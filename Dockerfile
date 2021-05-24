# Execution environment stage to save space
FROM ubuntu:20.04 as execution

# Install dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
        librdkafka-dev \
    && rm -rf /var/lib/apt/lists/*


FROM xabylr/derecho:2.1.0 as build

# Avoid timezone issue
ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        git \
        build-essential \
        cmake \
        librdkafka-dev \
        libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

# Install cppkafka
RUN git clone -b v0.3.1 --depth 1 https://github.com/mfontanini/cppkafka.git

WORKDIR /cppkafka/build

RUN cmake .. && make && make install

# Copy kdsnyc sources
COPY src /kdsync/src

WORKDIR /kdsync/src

# Compile project
RUN make

FROM execution

# Libcppkafka library
COPY --from=build /usr/local/lib/libcppkafka.so /usr/local/lib/libcppkafka.so
COPY --from=build /usr/local/lib/libcppkafka.so.0.3.1 /usr/local/lib/libcppkafka.so.0.3.1
COPY --from=build /usr/local/include/cppkafka /usr/local/include/cppkafka

# Default Derecho configuration file
COPY --from=build /usr/local/share/derecho/derecho-sample.cfg /etc/derecho/derecho.cfg

# Kdsync binary
COPY --from=build /kdsync/bin/kdsync /usr/bin/kdsync

# Derecho configuration file location
ENV DERECHO_CONF_FILE=/etc/derecho/derecho.cfg

# Cppkafka path search workaround
ENV LD_LIBRARY_PATH=/usr/local/lib

ENTRYPOINT ["kdsync"]