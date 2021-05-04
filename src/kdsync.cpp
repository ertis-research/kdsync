#include <iostream>
#include <cppkafka/cppkafka.h>

using namespace std;
using namespace cppkafka;

const string KAFKA_TOPIC = "test";
const string KAFKA_BROKERS = "127.0.0.1:9092";
const string KAFKA_GROUP_ID = "kdsync";

int main()
{
	Configuration config = {
		{"metadata.broker.list", KAFKA_BROKERS},
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
	consumer.subscribe({KAFKA_TOPIC});
	
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
