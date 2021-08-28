#!/bin/bash
# Run as superuser

#https://www.digitalocean.com/community/tutorials/how-to-install-apache-kafka-on-ubuntu-20-04

set -e # Exit on eror
set -x # Show arguments executed

#Install packages
apt-get update

apt-get install -y --no-install-recommends \
    ca-certificates \
    wget \
    git \
    build-essential \
    cmake \
    libtool \
    m4 \
    automake \
    libssl-dev \
    librdmacm-dev \
    libibverbs-dev \
    librdkafka-dev \
    libboost-all-dev \
    libfmt-dev 

cd /opt # Workdir for installation

#####################
## Install Derecho ##
#####################

git clone -b v2.2.1 --depth 1 https://github.com/Derecho-Project/derecho.git

(
    export LC_ALL=C # Use default language (presumably English)

    # Install libfabric
    ./derecho/scripts/prerequisites/install-libfabric.sh

    # Install json
    ./derecho/scripts/prerequisites/install-json.sh

    # Instal mutils
    ./derecho/scripts/prerequisites/install-mutils.sh

    # Instal mutils containers
    ./derecho/scripts/prerequisites/install-mutils-containers.sh

    # Instal mutils tasks
    ./derecho/scripts/prerequisites/install-mutils-tasks.sh
)


# Instal libspdlog 1.3 (1.5 will not work with Derecho)
wget http://old-releases.ubuntu.com/ubuntu/pool/universe/s/spdlog/libspdlog-dev_1.3.1-1_amd64.deb
dpkg -i libspdlog-dev_1.3.1-1_amd64.deb
rm libspdlog-dev_1.3.1-1_amd64.deb

(
    mkdir -p derecho/Release
    cd derecho/Release
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
    make -j $(nproc)
    make install
)

####################
## Install Kdsync ##
####################

git clone -b v0.3.1 --depth 1 https://github.com/mfontanini/cppkafka.git
(mkdir -p cppkafka/build && cd cppkafka/build && cmake .. && make && make install)


git clone -b master --depth 1 https://github.com/ertis-research/kdsync.git
(cd kdsync/src && make && make install)

ldconfig # Create links of libraries installed