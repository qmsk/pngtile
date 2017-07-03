FROM debian:stretch

RUN apt-get update && apt-get install -y \
    build-essential \
    libc6-dev libpng-dev \
    python python-dev python-pip virtualenv

ADD . /src/pngtile

WORKDIR /src/pngtile

RUN make

RUN virtualenv /opt/pngtile

RUN make -B install PREFIX=/opt/pngtile
RUN /opt/pngtile/bin/pip install -r requirements.txt
RUN /opt/pngtile/bin/python setup.py build_ext
RUN /opt/pngtile/bin/python setup.py install

WORKDIR /opt/pngtile
