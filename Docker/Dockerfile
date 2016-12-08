FROM alpine:3.4
MAINTAINER Michael McVady <femtonaut@gmail.com>

RUN apk update && \
    apk upgrade && \
    echo 'add runtime dependencies' && \
    apk add --no-cache \
    boost \
    snappy && \
    echo 'add build dependencies' && \
    apk add --no-cache --virtual build-deps \
    boost-dev \
    g++ \
    git \
    scons \
    snappy-dev && \
    echo 'build and install dory' && \
    cd /root/ && \
    git clone https://github.com/team-telnyx/dory && \
    cd dory && \
    git checkout alpine_dockerize && \
    cd src/dory && \
    scons -Q --up --release dory && \
    mkdir -p /opt/dory/bin/ && \
    cp /root/dory/out/release/dory/dory /opt/dory/bin/ && \
    echo 'clean up and remove build dependencies' && \
    cd /root && \
    rm -rf dory && \
    apk del --purge build-deps

RUN mkdir -p /etc/dory
ADD dory_conf.xml /etc/dory/
ADD start.sh /etc/dory/

EXPOSE 9090

CMD sh /etc/dory/start.sh
