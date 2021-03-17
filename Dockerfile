FROM debian:buster as build
# Ping with HTTP requests, see http://www.vanheusden.com/httping/
# built directly from master

LABEL maintainer="Bret Fisher <bret@bretfisher.com>"

RUN set -x \
	&& apt-get update \
	&& apt-get install -y \
    build-essential gcc make cmake gettext git \
    libncurses5-dev libncursesw5-dev libssl-dev libfftw3-dev libfftw3-bin \
    libfftw3-double3 libgomp1 libssl1.1 openssl \
    apt-transport-https ca-certificates wget

RUN git clone https://github.com/flok99/httping.git \
    && cd httping \
    && make \
    && chmod +x httping

# to prevent image bloat, lets do a multi-stage build and only copy the binary needed
FROM debian:buster-slim as release

RUN set -x \
    && apt-get update \
    && apt-get install -y \
    libfftw3-double3 libgomp1 libssl1.1 openssl \
    libfftw3-bin libfftw3-dev curl ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /httping/httping /usr/local/bin/

ENV TERM=xterm-256color

ENTRYPOINT ["httping"]

CMD ["--version"]
