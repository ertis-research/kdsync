#include <cppkafka/cppkafka.h>
#include <librdkafka/rdkafka.h>

#include <derecho/conf/conf.hpp>
#include <derecho/core/derecho.hpp>
#include <iostream>
#include <thread>

#include "serializable_objects.hpp"
#include "spdlog/spdlog.h"

//#include <chrono>

using namespace std;
using namespace cppkafka;

#define REPLICAS 2
#define EVENT_CHUNK 10
#define KAFKA_BROKERS "127.0.0.1:9092"
#define KAFKA_TOPIC "test"
#define KAFKA_GROUP_ID "kdsync"
#define KAFKA_KDSYNC_TOPIC_SUFFIX "-global"

unsigned int replicas = REPLICAS;
unsigned int event_chunk = EVENT_CHUNK;
string kafka_brokers = KAFKA_BROKERS;
string kafka_topic = KAFKA_TOPIC;
Producer* producer;


/*
  Produces an event to the local cluster
*/
void replicate_event(const ReplicatedEvent& event){
  string replication_topic =
            event.topic + KAFKA_KDSYNC_TOPIC_SUFFIX;

  
  if(event.headers.size()>0){
    HeaderList<Header<Buffer>> hl(event.headers.size());
    for(auto rep_header : event.headers){
      Header<Buffer> header(rep_header.key, rep_header.value);
      hl.add(header);
    }
    producer->produce(MessageBuilder(replication_topic)       // topic
                            .partition(event.partition)    // partition
                            .payload(event.payload)        // payload
                            .key(event.key)                // key
                            .headers(hl)                   // headers
                            // offset is automatic
    );
  }else {
    producer->produce(MessageBuilder(replication_topic)       // topic
                            .partition(event.partition)    // partition
                            .payload(event.payload)        // payload
                            .key(event.key)                // key
                            // offset is automatic
    );
  }

  producer->flush();  
}


int main(int argc, char *argv[]) {
    // Read environment
  if(getenv("KDSYNC_DEBUG") != NULL){
    spdlog::set_level(spdlog::level::debug);
  }else{
    spdlog::set_level(spdlog::level::warn);
  }

  const char* str_event_chunk = getenv("EVENT_CHUNK");
  if(str_event_chunk != NULL){
    event_chunk = atoi(str_event_chunk);
  }


  spdlog::info("Initialising Derecho");
  // Read configurations from the command line options as well as the default
  // config file
  derecho::Conf::initialize(argc, argv);

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

  // Define subgroup membership using the default subgroup allocator function
  derecho::SubgroupInfo subgroup_function{derecho::DefaultSubgroupAllocator(
      {{std::type_index(typeid(EventList)),
        derecho::one_subgroup_policy(
            derecho::fixed_even_shards(1, replicas))}})};

  // Each replicated type needs a factory; this can be used to supply
  // constructor arguments
  // for the subgroup's initial state. These must take a PersistentRegistry*
  // argument, but in this case we ignore it because the replicated objects
  // aren't persistent.
  auto event_list_factory = [](persistent::PersistentRegistry *,
                               derecho::subgroup_id_t) {
    return std::make_unique<EventList>(replicas, event_chunk);
  };

  spdlog::info("Initialising group construction/joining");

  derecho::Group<EventList> group(derecho::UserMessageCallbacks{}, subgroup_function, {},
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

  Configuration consumer_config = {{"metadata.broker.list", kafka_brokers},
                                  {"group.id", KAFKA_GROUP_ID}};

  Configuration producer_config = {{"metadata.broker.list", kafka_brokers}};


  // Kafka consumer initialization
  Consumer consumer(consumer_config);

  // Set the assignment callback
  consumer.set_assignment_callback([&](TopicPartitionList &topic_partitions) {
    // Here you could fetch offsets and do something, altering the offsets on
    // the topic_partitions vector if needed
    spdlog::info("Kafka consumer got assigned {:d} partitions", topic_partitions.size());
  });

  // Set the revocation callback
  consumer.set_revocation_callback(
      [&](const TopicPartitionList &topic_partitions) {
        spdlog::info("{:d} partitions revoked!", topic_partitions.size());
      });

  // Subscribe topics to sync
  consumer.subscribe({kafka_topic});

  // Kafka producer initialization
  producer = new Producer(producer_config);

  spdlog::info("Kdsync has started the main loop");
  while (true) {
    //std::this_thread::sleep_for(std::chrono::milliseconds(200));

    derecho::QueryResults<vector<ReplicatedEvent>> events_query =
      message_rpc_handle.ordered_send<RPC_NAME(get_events)>(my_id);

    derecho::QueryResults<vector<ReplicatedEvent>>::ReplyMap& events_results =
      events_query.get();
    

    // Get longest list from all replies that does not return error
    vector<ReplicatedEvent> events_to_replicate; //Empty list
    bool found = false;
    auto ptr = events_results.begin();
    int longest_list = 0;
    while(ptr != events_results.end()){
      try {
        vector<ReplicatedEvent> received_list = ptr->second.get();
        if (received_list.size()>longest_list){
          longest_list = received_list.size();
          events_to_replicate = received_list;
        }
      } catch(const std::exception& e) {
        spdlog::warn("Couldn't recover event list from instance {:d}", ptr->first);
      }

      ptr++;
    }

    //Replicate all events from list
    if(events_to_replicate.size()>0 ){
      spdlog::info("Received {:d} elements to replicate locally", events_to_replicate.size());

      for (auto event : events_to_replicate){
        replicate_event(event);
      }

      ReplicatedEvent last_event = events_to_replicate.back();

      spdlog::info("Marking events as replicated");
      message_rpc_handle.ordered_send<RPC_NAME(mark_produced)>(
              my_id, last_event.topic, last_event.partition,
              last_event.offset);
    }
      
    // Poll. This will optionally return a message. It's necessary to check if
    // it's a valid one before using it
    Message msg = consumer.poll();
    if (msg) {
      if (!msg.get_error()) {
        // It's an actual message. Get the payload and print it to stdout
        string str_payload((char *)msg.get_payload().get_data(),
                           msg.get_payload().get_size());

        spdlog::info("Received local event");

          //Construct a ReplicatedEvent using a cppkafka Message
          vector<unsigned char> payload_v(msg.get_payload().begin(),
                                          msg.get_payload().end());
          vector<unsigned char> key_v(msg.get_key().begin(),
                                      msg.get_key().end());
          
          vector<ReplicatedHeader> headers;

          for (auto & event : msg.get_header_list()){
            vector<unsigned char> header_value_v(event.get_value().begin(),
                                          event.get_value().end());
            ReplicatedHeader rh = ReplicatedHeader(event.get_name(), header_value_v);
            headers.push_back(rh);
          }
          
          ReplicatedEvent revent(msg.get_topic(), msg.get_partition(),
                                 payload_v, key_v, msg.get_offset(), headers);

          // Send message to the other kdsync instances
          message_rpc_handle.ordered_send<RPC_NAME(add_event)>(revent);
      } else if (!msg.is_eof()) {
        // Is it an error notification, handle it.
        // This is explicitly skipping EOF notifications as they're not actually
        // errors, but that's how rdkafka provides them
      }
    }
  }

  return 0;
}
