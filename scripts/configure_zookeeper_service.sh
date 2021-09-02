#!/bin/bash
# Run as superuser

set -e # Exit on eror
set -x # Show arguments executed

cat > /etc/systemd/system/zookeeper.service <<EOF
[Unit]
Requires=network.target remote-fs.target
After=network.target remote-fs.target

[Service]
Type=simple
User=root
ExecStart=/opt/kafka/bin/zookeeper-server-start.sh /opt/kafka/config/zookeeper.properties
ExecStop=/opt/kafka/bin/zookeeper-server-stop.sh
Restart=on-abnormal

[Install]
WantedBy=multi-user.target
EOF

systemctl enable zookeeper
systemctl start zookeeper

mkdir -p /etc/systemd/system/kafka.service.d
cat > /etc/systemd/system/kafka.service.d/zookeper <<EOF
[Unit]
Requires=zookeeper.service
After=zookeeper.service
EOF

systemctl restart kafka