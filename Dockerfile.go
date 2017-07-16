FROM debian:stretch

RUN apt-get update && apt-get install -y \
    build-essential libc6-dev \
    libpng16-16 libpng-dev \
    golang-go git

RUN mkdir -p /go
ENV GOPATH=/go
RUN go get -v -d github.com/urfave/cli

ADD . /go/src/github.com/qmsk/pngtile
WORKDIR /go/src/github.com/qmsk/pngtile

RUN make
RUN go install -v ./go/...

ENV PATH=/go/bin:$PATH
