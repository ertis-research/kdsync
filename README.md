# kdsync: Synchronize Kafka clusters using Derecho.
This application will alow synchronization between Apache Kafka clusters
using [Derecho](https://derecho-project.github.io/) library for synchronization 
with RDMA or TCP connections.

It is still in early development, but it is already working.

## How to use

### Option 1. Using Docker
This option depends on this [derecho Docker container](https://github.com/ertis-research/docker-derecho).

* Build the kdsync image 
  ```
  docker build -t kdsync .
  ```

* Run an instance from the image<br> 
  You can specify kdsync arguments as well as copying a derecho conf file in /etc/derecho/derecho.cfg
  ```
  docker run -dit --name kdsync --volume /abs/path/to/config/file.cfg:/etc/derecho/derecho.cfg kdsync <number-of-instances> <cluster-brokers> <topic-to-replicate>
  ```
  If kafka brokers or other kdsync instances are outside the same network:
  * If running Docker in Linux:<br>
   ```--network="host"``` option should be added to the run 
   option:
    ```
    docker run --network="host" -dit --name kdsync kdsync <number-of-instances> <cluster-brokers> <topic-to-replicate>
    ```
  * If running Docker for Windows or Mac:<br>
   Use ```host.docker.internal``` instead of ```localhost``` in broker addresses.


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
  ./kdsync <number-of-instances> <cluster-brokers> <topic-to-replicate>
  ```

