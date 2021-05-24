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
COPY src /src

WORKDIR /src

# Compile project
RUN make

# Copy installed files to second ubuntu stage to save space
FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
        librdkafka-dev \
    && rm -rf /var/lib/apt/lists/*

# Librdkafka library
# COPY --from=build /usr/lib/x86_64-linux-gnu/librdkafka.a /usr/lib/x86_64-linux-gnu/librdkafka.a
# COPY --from=build /usr/lib/x86_64-linux-gnu/librdkafka.so /usr/lib/x86_64-linux-gnu/librdkafka.so
# COPY --from=build /usr/lib/x86_64-linux-gnu/librdkafka.so.1 /usr/lib/x86_64-linux-gnu/librdkafka.so.1
# COPY --from=build /usr/lib/x86_64-linux-gnu/librdkafka++.a /usr/lib/x86_64-linux-gnu/librdkafka++.a
# COPY --from=build /usr/lib/x86_64-linux-gnu/librdkafka++.so /usr/lib/x86_64-linux-gnu/librdkafka++.so
# COPY --from=build /usr/lib/x86_64-linux-gnu/librdkafka++.so.1 /usr/lib/x86_64-linux-gnu/librdkafka++.so.1
# COPY --from=build /usr/include/librdkafka /usr/include/librdkafka

# Libcppkafka library
COPY --from=build /usr/local/lib/libcppkafka.so /usr/local/lib/libcppkafka.so
COPY --from=build /usr/local/lib/libcppkafka.so.0.3.1 /usr/local/lib/libcppkafka.so.0.3.1
COPY --from=build /usr/local/include/cppkafka /usr/local/include/cppkafka

# Kdsync
COPY --from=build /build /build

# Cppkafka path search workaround
ENV LD_LIBRARY_PATH=/usr/local/lib

CMD ["/build/kdsync"]