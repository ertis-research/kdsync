# Kdsync: Synchronize Kafka clusters using Derecho.
Kdsync is a tool that allows ordered synchronization between Apache Kafka
clusters. It internally uses [Derecho](https://derecho-project.github.io/)
library in order to achieve it. 

This tool acts as an Apache Kafka client designed to work in conjuntion with
multiples instances of itself located in different clusters. Each instance
should connect to a different cluster of Apache Kafka, joining both as a
consumer and a producer. These instances will also be connected to each other
through TCP or preferably RDMA.

Synchronization is expected to work with RDMA or TCP connections but deeper
experimentation should be done.

Although it is still in early development and there could be a some bugs, it is
already functional.

## How it works
Kdsync will wait to consume events from one configured topic. When an event is
received, it will be inserted to a single common shared list of events to
replicate. Each tool, including the sender one, will constantly produce these
shared messages but in another topic with the same name as previous but with
suffixed "-global".

This new topic exists because if two events reach concurrently to the same topic
in two different clusters before they are synchronized, the order would not be
the same for all clusters. A Kafka client couln't and shouldn't do anything to
modify existing events from a topic. However, other approaches could improve
this situation, but they would require modifying or proxying Kafka brokers.

As messages are marked after they are sent by each instance, it is guaranteed
that every message will be sent **at least once**. Is important to take this in
mind in order to adjust delivery semantics of Kafka as needed. For greater
reliability,
[EoS](https://www.confluent.io/blog/exactly-once-semantics-are-possible-heres-how-apache-kafka-does-it/)
should be used.

## How to use

### Option 1. Building from source
* Install required libraries
  * [Derecho library](https://derecho-project.github.io/)
  * [librdkafka](https://github.com/edenhill/librdkafka)
  * [cppkafka](https://github.com/mfontanini/cppkafka)

* Clone this project
  ```
  git clone https://github.com/ertis-research/kdsync
  ```

* Open kdsync folder
  ```
  cd kdsync
  ```

* Build using makefile
  ```
  make
  ```

* Run the program with desired configuration
  ```
  ./kdsync <number-of-instances> <cluster-brokers> <topic-to-replicate>
  ```

### Option 2. Using Docker
This option depends on this [derecho Docker container](https://github.com/ertis-research/docker-derecho).

This option is not intented for use with RDMA connections.

* Clone this project
  ```
  git clone https://github.com/ertis-research/kdsync
  ```

* Open kdsync folder
  ```
  cd kdsync
  ```

* Build the kdsync image 
  ```
  docker build -t kdsync .
  ```

* Run an instance from the image.<br> 
  You can specify kdsync arguments as well as copying a derecho conf file in `/etc/derecho/derecho.cfg`.
  ```
  docker run -dit --name kdsync --volume /abs/path/to/config/file.cfg:/etc/derecho/derecho.cfg kdsync <number-of-instances> <cluster-brokers> <topic-to-replicate>
  ```
  If kafka brokers or other kdsync instances are outside the same Docker network:
  * If running Docker in Linux:<br>
   ```--network="host"``` option should be added to the run 
   option:
    ```
    docker run --network="host" -dit --name kdsync kdsync <number-of-instances> <cluster-brokers> <topic-to-replicate>
    ```
  * If running Docker for Windows or Mac:<br>
   Use ```host.docker.internal``` instead of ```localhost``` in broker addresses.
   It could be required to open Derecho ports if other kdsync instances are outside
   this Docker network.

## Configuration
In order to communicate with each other, each instance needs a 
[Derecho config file](https://derecho-project.github.io/userguide.html#configuring-core-derecho). Some examples are provided in this project.

This file could be specified by placing a file named `derecho.cfg` in the same
directory or by setting the location path in environment variable
`DERECHO_CONF_FILE`.

After that, the application can be run directly or being added as a service.
The following commands are used:
| Command order | Short name          | Default        | Description                                                                                                                           |
|---------------|---------------------|----------------|---------------------------------------------------------------------------------------------------------------------------------------|
| 1             | Number of instances | 2              | Number of all instances. Kdsync will not work until all instances are running                                     |
| 2             | Cluster brokers     | 127.0.0.1:9092 | One or more kafka broker addresses to locate the cluster in the format host1:port1,host2:port2.  It could be a subset of the cluster. |
| 3             | Topic to replicate  | test           | Name of the topic that Kdsync will replicate                                                                                          |