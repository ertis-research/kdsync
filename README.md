# kdsync: Synchronize Kafka clusters using Derecho.
This application will alow synchronization between Apache Kafka clusters
using [Derecho](https://derecho-project.github.io/) library for synchronization 
with RDMA or TCP connections.

It is still in early development, so features are missing.


## Requirements
* [Derecho library](https://derecho-project.github.io/)
* [librdkafka](https://github.com/edenhill/librdkafka)
* [cppkafka](https://github.com/mfontanini/cppkafka)

## How to build
```
cd src
make
```

## How to run
```
../build/kdsync
```
