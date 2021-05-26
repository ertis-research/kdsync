KAFKA_URL=https://ftp.cixug.es/apache/kafka/2.8.0/kafka_2.13-2.8.0.tgz

cd `dirname $0`
curl $KAFKA_URL -o kafka-dist.tgz
rm -rf kafka-dist
mkdir kafka-dist
cd kafka-dist
tar -xvzf ../kafka-dist.tgz --strip 1
rm -f ../kafka-dist.tgz