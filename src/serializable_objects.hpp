#pragma once

#include <cppkafka/cppkafka.h>
#include <librdkafka/rdkafka.h>

#include <derecho/core/derecho.hpp>
#include <derecho/mutils-serialization/SerializationSupport.hpp>
#include <forward_list>

#include "spdlog/spdlog.h"

using namespace std;


struct ReplicatedHeader : public mutils::ByteRepresentable {
  string key;
  vector<unsigned char> value;

  ReplicatedHeader(const string& key, vector<unsigned char>& value)
    : key(key),
      value(value){};

  bool operator==(const ReplicatedHeader &header) const{
    return this->key == header.key &&
     this->value == header.value;
  }

  DEFAULT_SERIALIZATION_SUPPORT(ReplicatedHeader, key, value);
};

struct ReplicatedEvent : public mutils::ByteRepresentable {
  string topic;
  int32_t partition;
  vector<unsigned char> payload;
  vector<unsigned char> key;
  int64_t offset;
  vector<ReplicatedHeader> headers;

  set<uint32_t> replicas;  // Contains the instances where the event is
                           // successfully replicated

  ReplicatedEvent(const string& topic, int32_t partition,
                  vector<unsigned char>& payload, vector<unsigned char>& key,
                  int64_t offset, const vector<ReplicatedHeader>& headers,
                  set<uint32_t> replicas = {})
      : topic(topic),
        partition(partition),
        payload(payload),
        key(key),
        offset(offset),
        headers(headers),
        replicas(replicas){};

  bool operator==(const ReplicatedEvent &event) const{
    return this->topic == event.topic &&
     this->partition == event.partition &&
     this->payload == event.payload &&
     this->key == event.key &&
     this->offset == event.offset &&
     this->headers == event.headers;
  }


  DEFAULT_SERIALIZATION_SUPPORT(ReplicatedEvent, topic, partition, payload, key,
                                offset, headers, replicas);
};


class EventList : public mutils::ByteRepresentable {
  unsigned int n_replicas;
  int event_chunk; //number of events to retrieve at the same time
  vector<ReplicatedEvent> events;

 public:
  void add_event(const ReplicatedEvent& event) {
    events.push_back(event);  // Put new events at the end of the list
  }

  /**
   * Called by an instance that has successfully produced all events
   * until the specified message id included.
   * All the previous events from the same topic will be marked as well.
   * When the final instance marks as produced an event. It will be removed
   * from the list. This is simmilar to the concept of an event commited to all
   * replicas in a kafka cluster, but between multiple clusters.
   *
   * @param[in] derecho_id derecho id of the caller
   * @param[in] last_topic topic of the last produced message
   * @param[in] last_partition partition of the last produced message
   * @param[in] last_offset kafka offset of the last message produced
   *
   */
  void mark_produced(uint32_t derecho_id, const string& last_topic,
                     int32_t last_partition, int64_t last_offset) {
    /*
    Mark all events as produced until the specified id and topic is reached.
    Safer option could be to traverse the list in reversed order. If event is
    not found, it doesn't mark anything. However, if the event is not found
    it should mean that was deleted before (though also this situation
    shouldn't be possible)
    */
    auto ptr = events.begin();
    auto last_completed = ptr;  // Last element to remove because has been
                                // produced by all instances
    bool found = false;
    while (!found && ptr != events.end()) {
      found = ptr->topic == last_topic && ptr->partition == last_partition &&
              ptr->offset == last_offset;
      ptr->replicas.insert(derecho_id);
      // spdlog::info("marking derecho id {:d}", derecho_id);

      if (ptr->replicas.size() == n_replicas) {
        // spdlog::info("Event Completed");

        // Mark this node to be deleted
        last_completed = next(ptr);
      } else {
        // spdlog::info("Event not completed");
      }

      ptr++;
    }

    //spdlog::info("begin: {:x}, completed: {:x}", (void*)&*events.begin(), (void*)&*last_completed);
    //spdlog::info("Deleting completed events");
    // Delete completed nodes
    events.erase(events.begin(), last_completed);
  }

  /**
   * @returns list of some events that aren't marked as replicated by provided
   * derecho id.
   */
  vector<ReplicatedEvent> get_events(uint32_t derecho_id) const { 
    auto first_not_replicated = find_if_not(
      events.begin(), 
      events.end(),
      [derecho_id](const ReplicatedEvent& event) {
        return event.replicas.find(derecho_id) != event.replicas.end();
      }
    );

    int remaining = distance(first_not_replicated, events.end());

    vector<ReplicatedEvent> result(
      first_not_replicated,
      next(first_not_replicated, min(event_chunk, remaining))
      );

    return result;
  }

  EventList(unsigned int n_replicas, unsigned int event_chunk) : n_replicas(n_replicas), event_chunk(event_chunk) {}
  EventList(unsigned int n_replicas, unsigned int event_chunk, const vector<ReplicatedEvent>& events)
      : n_replicas(n_replicas), event_chunk(event_chunk), events(events) {}

  DEFAULT_SERIALIZATION_SUPPORT(EventList, n_replicas, event_chunk, events);
  REGISTER_RPC_FUNCTIONS(EventList, ORDERED_TARGETS(add_event, mark_produced, get_events));
};