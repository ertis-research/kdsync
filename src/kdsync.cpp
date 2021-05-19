#include <iostream>
#include <cppkafka/cppkafka.h>
#include <librdkafka/rdkafka.h>

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
