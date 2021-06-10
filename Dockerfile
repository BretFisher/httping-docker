FROM alpine as build

RUN apk add \
    make \
    build-base \ 
    openssl-dev \
    openssl-libs-static \
    ncurses-dev \
    ncurses-static \
    gettext-dev \
    gettext-static \
    fftw-dev \
    fftw-double-libs \
    fftw-long-double-libs

WORKDIR /source

COPY source .

# make a static binary, and link in gettext
ENV LDFLAGS="-static -lintl"


RUN ./configure --with-tfo --with-ncurses --with-openssl --with-fftw3 && make

FROM alpine as release

COPY --from=build /source/httping /usr/local/bin/

ENV TERM=xterm-256color

# add -Y for nice color output!
ENTRYPOINT ["httping", "-Y"]

CMD ["--version"]