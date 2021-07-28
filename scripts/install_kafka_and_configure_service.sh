#!/bin/bash
# Run as superuser

set -e # Exit on eror
set -x # Show arguments executed

apt-get install -y --no-install-recommends openjdk-11-jdk

TMPFILE=`mktemp`
curl "https://ftp.cixug.es/apache/kafka/2.8.0/kafka_2.13-2.8.0.tgz" -o $TMPFILE
mkdir -p /opt/kafka
cd /opt/kafka
tar -xzf $TMPFILE --strip 1

cat > /etc/systemd/system/kafka.service <<EOF
[Service]
Type=simple
User=root
ExecStart=/bin/sh -c '/opt/kafka/bin/kafka-server-start.sh /opt/kafka/config/server.properties > /opt/kafka/kafka.log 2>&1'
ExecStop=/opt/kafka/bin/kafka-server-stop.sh
Restart=on-abnormal

[Install]
WantedBy=multi-user.target
EOF

nano /opt/kafka/config/server.properties

systemctl enable kafka
systemctl start kafka