FROM xabylr/docker-derecho:2.1.0

# Install librdkafka
RUN apt-get update && apt-get install -y --no-install-recommends \
        librdkafka-dev \
        libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /

# Install cppkafka
RUN git clone -b v0.3.1 --depth 1 https://github.com/mfontanini/cppkafka.git

WORKDIR /cppkafka/build

RUN cmake .. && make && make install

# Copy kdsnyc sources
COPY src /src

WORKDIR /src

# Compile project
RUN make

# Cppkafka path search workaround
ENV LD_LIBRARY_PATH=/usr/local/lib

CMD ["/build/kdsync"]