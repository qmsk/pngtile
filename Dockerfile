FROM debian:stretch

RUN apt-get update && apt-get install -y \
  curl apt-transport-https gnupg

RUN \
     curl -s https://deb.nodesource.com/gpgkey/nodesource.gpg.key | apt-key add - \
  && echo 'deb https://deb.nodesource.com/node_8.x stretch main' > /etc/apt/sources.list.d/nodesource.list

RUN apt-get update && apt-get install -y \
    build-essential libc6-dev \
    libpng16-16 libpng-dev \
    golang-go git \
    nodejs

RUN mkdir -p /go /go/src/github.com/qmsk/pngtile/web
ENV GOPATH=/go
RUN go get -v -d \
  github.com/urfave/cli \
  github.com/gorilla/schema \
  github.com/jessevdk/go-flags \
  github.com/qmsk/go-web

ADD web/package.json /go/src/github.com/qmsk/pngtile/web/
WORKDIR /go/src/github.com/qmsk/pngtile/web
RUN npm install

ADD . /go/src/github.com/qmsk/pngtile
WORKDIR /go/src/github.com/qmsk/pngtile
RUN make
RUN go install -v ./go/cmd/...

ENV PATH=/go/bin:$PATH

RUN adduser --system --uid 1000 --home /srv/pngtile --group pngtile

VOLUME /srv/pngtile/images
USER pngtile
CMD pngtile --recursive /srv/pngtile/images; pngtile-server --pngtile-path /srv/pngtile/images --http-static ./web --pngtile-templates ./web/templates --http-listen :9090
EXPOSE 9090
