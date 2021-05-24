# kdsync: Synchronize Kafka clusters using Derecho.
This application will alow synchronization between Apache Kafka clusters
using [Derecho](https://derecho-project.github.io/) library for synchronization 
with RDMA or TCP connections.

It is still in early development, so features are missing.


## How to use

### Option 1. Using Docker
This option depends on this [derecho Docker container](https://github.com/ertis-research/docker-derecho).

* Build kdsync image 
  ```
  docker build -t kdsync .
  ```

* Run an instance. 
  You can specify kdsync arguments as well as a derecho conf file in /etc/derecho/derecho.cfg
  ```
  docker run -dit --name kdsync kdsync <args>
  ```


### Option 2. Manual build

* Install required libraries
  * [Derecho library](https://derecho-project.github.io/)
  * [librdkafka](https://github.com/edenhill/librdkafka)
  * [cppkafka](https://github.com/mfontanini/cppkafka)

* ```
  cd kdsync
  ```

* ```
  make
  ```

* ```
  ./kdsync <kafka-brokers> <replicated-topic>
  ```

