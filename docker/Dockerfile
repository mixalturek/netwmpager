# Dirty, there is no need to be nice
FROM debian:buster

ENV NETWMPAGER_VERSION=1.11.2

RUN apt-get update
RUN apt-get install -y libxft-dev pkg-config wget mc
RUN apt-get install -y build-essential devscripts lintian

RUN mkdir /build \
&& cd /build \
&& wget https://github.com/mixalturek/netwmpager/archive/${NETWMPAGER_VERSION}.tar.gz \
&& mv ${NETWMPAGER_VERSION}.tar.gz netwmpager_${NETWMPAGER_VERSION}.orig.tar.gz \
&& tar -xf netwmpager_${NETWMPAGER_VERSION}.orig.tar.gz

RUN cd /build/netwmpager-${NETWMPAGER_VERSION} && debuild -us -uc
