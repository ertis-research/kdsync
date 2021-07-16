#pragma once

#include <cppkafka/cppkafka.h>
#include <librdkafka/rdkafka.h>

#include <derecho/core/derecho.hpp>
#include <derecho/mutils-serialization/SerializationSupport.hpp>
#include <forward_list>

#include "spdlog/spdlog.h"

using namespace std;


struct ReplicatedEvent : public mutils::ByteRepresentable {
  string topic;
  int32_t partition;
  vector<unsigned char> payload;
  vector<unsigned char> key;
  int64_t offset;

  set<uint32_t> replicas;  // Contains the instances where the event is
                           // successfully replicated

  ReplicatedEvent(const string& topic, int32_t partition,
                  vector<unsigned char>& payload, vector<unsigned char>& key,
                  int64_t offset, set<uint32_t> replicas = {})
      : topic(topic),
        partition(partition),
        payload(payload),
        key(key),
        offset(offset),
        replicas(replicas){};

  bool operator==(const ReplicatedEvent &event){
    return this->topic == event.topic &&
     this->partition == event.partition &&
     this->payload == event.payload &&
     this->key == event.key &&
     this->offset == event.offset;
  }


  DEFAULT_SERIALIZATION_SUPPORT(ReplicatedEvent, topic, partition, payload, key,
                                offset, replicas);
};


class EventList : public mutils::ByteRepresentable {
  unsigned int n_replicas;
  list<ReplicatedEvent> events;

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
    Mark all events as produced until the specified id and topic is reached
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
    spdlog::info("Deleting completed events");
    // Delete completed nodes
    events.erase(events.begin(), last_completed);
  }

  /**
   * @returns a deepy copy of the complete list of events at the moment.
   */
  list<ReplicatedEvent> get_events() { return events; }

  EventList(unsigned int n_replicas) : n_replicas(n_replicas) {}
  EventList(unsigned int n_replicas, const list<ReplicatedEvent>& events)
      : n_replicas(n_replicas), events(events) {}

  DEFAULT_SERIALIZATION_SUPPORT(EventList, n_replicas, events);
  REGISTER_RPC_FUNCTIONS(EventList, add_event, mark_produced, get_events);
};