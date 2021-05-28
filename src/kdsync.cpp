#include "spdlog/spdlog.h"
#include <iostream>
#include <cppkafka/cppkafka.h>
#include <librdkafka/rdkafka.h>
#include <derecho/conf/conf.hpp>
#include <derecho/core/derecho.hpp>
#include "serializable_message.hpp"

using namespace std;
using namespace cppkafka;

const string KAFKA_TOPIC = "test";
const string KAFKA_BROKERS = "127.0.0.1:9092";
const string KAFKA_GROUP_ID = "kdsync";
const string KAFKA_KDSYNC_HEADER = "kdsync-processed";

bool is_already_processed(const Message &message){
	Message::HeaderListType header_list = message.get_header_list();

	Message::HeaderType *found_header = NULL;
	auto it = header_list.begin();
	while(it!= header_list.end() && it->get_name() != KAFKA_KDSYNC_HEADER ){
		it++;
	}

	if(it != header_list.end()){
		if (it -> get_value() == "true"){
			return true;

		}else{
			return false;
		}
	}

	return false;
}


int main(int argc, char *argv[])
{
	spdlog::info("Initialising Derecho");
	// Read configurations from the command line options as well as the default config file
    derecho::Conf::initialize(argc, argv);

	//Define subgroup membership using the default subgroup allocator function
    derecho::SubgroupInfo subgroup_function {derecho::DefaultSubgroupAllocator({
        {std::type_index(typeid(SerializableMessage)), derecho::one_subgroup_policy(derecho::fixed_even_shards(1,2))}
    })};

	//Each replicated type needs a factory; this can be used to supply constructor arguments
    //for the subgroup's initial state. These must take a PersistentRegistry* argument, but
    //in this case we ignore it because the replicated objects aren't persistent.
    auto serializable_message_factory = [](persistent::PersistentRegistry*,derecho::subgroup_id_t) { return std::make_unique<SerializableMessage>(); };

	spdlog::info("Initialising group construction/joining");

	derecho::Group<SerializableMessage> group(derecho::CallbackSet{}, subgroup_function, {},
										std::vector<derecho::view_upcall_t>{},
										serializable_message_factory);

	spdlog::info("Finished group construction/joining");

	uint32_t my_id = derecho::getConfUInt32(CONF_DERECHO_LOCAL_ID);
	
	spdlog::info("Local id: {}", my_id);
	

	//Get subgroup 0 (SerializableMessage). Subgroup index is in order declaration
	std::vector<node_id_t> message_members = group.get_subgroup_members<SerializableMessage>(0)[0];

	uint32_t rank_in_message = derecho::index_of(message_members, my_id);

	spdlog::info("Rank: {}", rank_in_message);

	derecho::Replicated<SerializableMessage>& message_rpc_handle = group.get_subgroup<SerializableMessage>();

	string kafka_brokers = KAFKA_BROKERS;
	string kafka_topic = KAFKA_TOPIC;
	

	if(argc>1){
		kafka_brokers = argv[1];
		if(argc>2){
			kafka_topic = argv[2];
		}
	}

	Configuration config = {
		{"metadata.broker.list", kafka_brokers},
		{"group.id", KAFKA_GROUP_ID}};

	// Kafka consumer initialization
	Consumer consumer(config);

	// Set the assignment callback
	consumer.set_assignment_callback([&](TopicPartitionList &topic_partitions) {
		// Here you could fetch offsets and do something, altering the offsets on the
		// topic_partitions vector if needed
		cout << "Got assigned " << topic_partitions.size() << " partitions!" << endl;
	});

	// Set the revocation callback
	consumer.set_revocation_callback([&](const TopicPartitionList &topic_partitions) {
		cout << topic_partitions.size() << " partitions revoked!" << endl;
	});

	// Subscribe topics to sync
	consumer.subscribe({kafka_topic});

	// Kafka producer initialization
	Producer producer(config);
	
	while (true)
	{
		// Poll. This will optionally return a message. It's necessary to check if it's a valid
		// one before using it
		Message msg = consumer.poll();
		if (msg)
		{
			if (!msg.get_error())
			{
				// It's an actual message. Get the payload and print it to stdout
				cout << msg.get_payload() << endl;

				if(!is_already_processed(msg)){
					//Create new message
					MessageBuilder new_msg(msg);

					//Add sync header
					cppkafka::Message::HeaderType kafka_sync_header(KAFKA_KDSYNC_HEADER, "true");
					new_msg.header(kafka_sync_header);

					producer.produce(new_msg);
				}
			}
			else if (!msg.is_eof())
			{
				// Is it an error notification, handle it.
				// This is explicitly skipping EOF notifications as they're not actually errors,
				// but that's how rdkafka provides them
			}
		}
	}

	return 0;
}
