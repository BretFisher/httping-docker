## Docker slim image of httping, built from source

This container image is stored on docker hub: https://hub.docker.com/r/bretfisher/httping/

It builds from source repo at https://github.com/flok99/httping including all optional features
like the ncurses GUI `-K`, TCP fast open `-F`, and SSL `-l`.

### Basic example at docker CLI

run a HTTPS ping against google every second until you hit ctrl-c or stop/kill the container

`docker run --rm bretfisher/httping https://www.google.com`

### Better example

ping ever 100ms, use GET not HEAD, show status codes, use pretty colors

`docker run --rm bretfisher/httping -i .1 -G -s -Y https://www.google.com`

### Use the shell GUI

add a `-it` to run command and a `-K` to httping

`docker run -it --rm bretfisher/httping -i .1 -GsYK https://www.google.com`