#include <cppkafka/cppkafka.h>
#include <librdkafka/rdkafka.h>

#include <derecho/conf/conf.hpp>
#include <derecho/core/derecho.hpp>
#include <iostream>
#include <thread>

#include "serializable_objects.hpp"
#include "spdlog/spdlog.h"

using namespace std;
using namespace cppkafka;

#define REPLICAS 2
#define KAFKA_BROKERS "127.0.0.1:9092"
#define KAFKA_TOPIC "test"
#define KAFKA_GROUP_ID "kdsync"
#define KAFKA_KDSYNC_TOPIC_SUFFIX "-global"
#define KAFKA_KDSYNC_HEADER "kdsync-processed"

unsigned int replicas = REPLICAS;
string kafka_brokers = KAFKA_BROKERS;
string kafka_topic = KAFKA_TOPIC;
Producer* producer;

bool is_already_processed(const Message &message) {
  Message::HeaderListType header_list = message.get_header_list();

  Message::HeaderType *found_header = NULL;
  auto it = header_list.begin();
  while (it != header_list.end() && it->get_name() != KAFKA_KDSYNC_HEADER) {
    it++;
  }

  if (it != header_list.end()) {
    if (it->get_value() == "true") {
      return true;

    } else {
      return false;
    }
  }

  return false;
}

/*
  Produces an event to the local cluster
*/
void replicate_event(const ReplicatedEvent& event){
  string replication_topic =
            event.topic + KAFKA_KDSYNC_TOPIC_SUFFIX;
  producer->produce(MessageBuilder(replication_topic)       // topic
                             .partition(event.partition)    // partition
                             .payload(event.payload)        // payload
                             .key(event.key)                // key
                         // offset is automatic
    );
  producer->flush();  
}


int main(int argc, char *argv[]) {
  spdlog::info("Initialising Derecho");
  // Read configurations from the command line options as well as the default
  // config file
  derecho::Conf::initialize(argc, argv);

  // Define subgroup membership using the default subgroup allocator function
  derecho::SubgroupInfo subgroup_function{derecho::DefaultSubgroupAllocator(
      {{std::type_index(typeid(EventList)),
        derecho::one_subgroup_policy(
            derecho::fixed_even_shards(1, REPLICAS))}})};

  // Each replicated type needs a factory; this can be used to supply
  // constructor arguments
  // for the subgroup's initial state. These must take a PersistentRegistry*
  // argument, but in this case we ignore it because the replicated objects
  // aren't persistent.
  auto event_list_factory = [](persistent::PersistentRegistry *,
                               derecho::subgroup_id_t) {
    return std::make_unique<EventList>(REPLICAS);
  };

  spdlog::info("Initialising group construction/joining");

  derecho::Group<EventList> group(derecho::CallbackSet{}, subgroup_function, {},
                                  std::vector<derecho::view_upcall_t>{},
                                  event_list_factory);

  spdlog::info("Finished group construction/joining");

  uint32_t my_id = derecho::getConfUInt32(CONF_DERECHO_LOCAL_ID);

  spdlog::info("Local id: {}", my_id);

  // Get first and only subgroup (SerializableMessage).
  // Subgroup index is in declaration order (0)
  std::vector<node_id_t> message_members =
      group.get_subgroup_members<EventList>(0)[0];

  uint32_t rank_in_event = derecho::index_of(message_members, my_id);

  spdlog::info("Rank: {}", rank_in_event);

  derecho::Replicated<EventList> &message_rpc_handle =
      group.get_subgroup<EventList>();

  // Parse arguments
  if (argc > 1) {
    replicas = atoi(argv[1]);
    if (argc > 2) {
      kafka_brokers = argv[2];
      if (argc > 3) {
        kafka_topic = argv[3];
      }
    }
  }

  Configuration kafka_config = {{"metadata.broker.list", kafka_brokers},
                                {"group.id", KAFKA_GROUP_ID}};

  // Kafka consumer initialization
  Consumer consumer(kafka_config);

  // Set the assignment callback
  consumer.set_assignment_callback([&](TopicPartitionList &topic_partitions) {
    // Here you could fetch offsets and do something, altering the offsets on
    // the topic_partitions vector if needed
    spdlog::info("Got assigned {:d} partitions!", topic_partitions.size());
  });

  // Set the revocation callback
  consumer.set_revocation_callback(
      [&](const TopicPartitionList &topic_partitions) {
        spdlog::info("{:d} partitions revoked!", topic_partitions.size());
      });

  // Subscribe topics to sync
  consumer.subscribe({kafka_topic});

  // Kafka producer initialization
  producer = new Producer(kafka_config);

  while (true) {
    derecho::QueryResults<list<ReplicatedEvent>> events_query =
      message_rpc_handle.ordered_send<RPC_NAME(get_events)>();

    derecho::QueryResults<list<ReplicatedEvent>>::ReplyMap& events_results =
      events_query.get();
    

    // Get one list from all results that does not return error
    list<ReplicatedEvent> local_events; //Empty list
    bool found = false;
    auto ptr = events_results.begin();
    while(!found && ptr != events_results.end()){
      try {
        local_events = ptr->second.get();
        spdlog::info("Using instance {:d} event list", ptr->first);
        found = true;
      } catch(const std::exception& e) {
        spdlog::warn("Couldn't get event list from instance {:d}", ptr->first);
      }

      ptr++;
    }

    //Replicate all local events that were not replicated
    spdlog::info("Local list has {:d} elements", local_events.size());

    //Find first NOT replicated event and continue replicating from this point
    auto replicate_iter = find_if_not(
		local_events.begin(),
		local_events.end(),
		[my_id] (const ReplicatedEvent& event) { 
			return event.replicas.find(my_id) != event.replicas.end();}
	);

    // Send and mark all local messages as sent (if any)
    if(replicate_iter != local_events.end()){

    spdlog::info("Local producing global events...");
    for (auto ptr = replicate_iter; ptr!=local_events.end(); ptr++){
      replicate_event(*ptr);
    }
    ReplicatedEvent last_event = local_events.back();

    spdlog::info("Telling other instances local replication status");
    message_rpc_handle.ordered_send<RPC_NAME(mark_produced)>(
            my_id, last_event.topic, last_event.partition,
            last_event.offset);
    }else{
      //spdlog::info("No messages to replicate");
    }
      
    // Poll. This will optionally return a message. It's necessary to check if
    // it's a valid one before using it
    Message msg = consumer.poll();
    if (msg) {
      if (!msg.get_error()) {
        // It's an actual message. Get the payload and print it to stdout
        string str_payload((char *)msg.get_payload().get_data(),
                           msg.get_payload().get_size());

        spdlog::info("Received local event:  {:s}", str_payload);

        if (!is_already_processed(msg)) {
          //Construct a ReplicatedEvent using a cppkafka Message
          vector<unsigned char> payload_v(msg.get_payload().begin(),
                                          msg.get_payload().end());
          vector<unsigned char> key_v(msg.get_key().begin(),
                                      msg.get_key().end());

          ReplicatedEvent revent(msg.get_topic(), msg.get_partition(),
                                 payload_v, key_v, msg.get_offset());

          // Send message to the other kdsync instances
          message_rpc_handle.ordered_send<RPC_NAME(add_event)>(revent);
        }
      } else if (!msg.is_eof()) {
        // Is it an error notification, handle it.
        // This is explicitly skipping EOF notifications as they're not actually
        // errors, but that's how rdkafka provides them
      }
    }
  }

  return 0;
}
