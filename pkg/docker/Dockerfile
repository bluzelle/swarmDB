FROM ubuntu:16.04
MAINTAINER support@bluzelle.com

RUN set -x \
    && apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates \
        gettext-base \
        libjsoncpp-dev

ARG SWARM_BUILD_DIR
ARG SWARM_NAME

COPY swarmdb__amd64.deb /tmp
RUN dpkg -i /tmp/swarmdb__amd64.deb && apt-get install -f

COPY config.json.template /tmp

RUN mkdir -p /tmp/swarm_home && cd /tmp/swarm_home

WORKDIR /tmp/swarm_home

ENTRYPOINT mkdir -p ${SWARM_NAME} \
    && cd ${SWARM_NAME} \
    && envsubst < /tmp/config.json.template > config-${SWARM_NAME}.json \
    && swarm -c config-${SWARM_NAME}.json
