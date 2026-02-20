FROM alpine AS build

RUN apk add --no-cache \
    make \
    build-base \ 
    openssl-dev \
    openssl-libs-static \
    ncurses-dev \
    ncurses-static \
    gettext-dev \
    gettext-static \
    fftw-dev \
    fftw-static \
    fftw-double-libs \
    fftw-long-double-libs

WORKDIR /source

COPY source .

# make a static binary, and link in gettext
ENV LDFLAGS="-static -lintl"


RUN ./configure --with-tfo --with-ncurses --with-openssl --with-fftw3 && make

FROM alpine AS release

RUN apk add --no-cache \
    ncurses

COPY --from=build /source/httping /usr/local/bin/

ENV TERM=xterm-256color

# add -Y for nice color output!
ENTRYPOINT ["httping", "-Y"]

CMD ["--version"]