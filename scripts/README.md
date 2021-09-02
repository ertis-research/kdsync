# Scripts instructions
These scripts have been tested under ubuntu 20.04.
They all need to be run with superuser permissions.

* `install_kafka_and_configure_service.sh`: It will download Apache Kafka from
  the Internet and configure a service named `kafka`. Before the service starts,
  the Kafka server configuration file can be customized.
* `configure_zookeeper_service.sh`: If run after previous script, it will configure
  Zookeeper as a new service named `zookeeper` and configure the existing Kafka
  service to start after this one.

* `install_derecho_and_kdsync.sh`: It will download Derecho from its GitHub
  repository, compile it and install it as an executable.

* `configure_kdsync_service.sh`: If run after previous script, it will create a
  Kdsync a service named `kdsync`. Before the service starts, the Derecho
  configuration file can be customized.