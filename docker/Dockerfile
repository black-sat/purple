FROM black:ubuntu20.04

ARG BLACK_VERSION=0.10.3
ENV BLACK_VERSION $BLACK_VERSION
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y software-properties-common && \
    add-apt-repository ppa:deadsnakes/ppa
RUN apt-get update && apt-get install -y pybind11-dev \
    python3-pip python-is-python3 \
    python3.9-dev python3.9-distutils 

ENV Python3_EXECUTABLE=/usr/bin/python3.9
ENV PYTHON_EXECUTABLE=/usr/bin/python3.9

WORKDIR /downloads
RUN wget https://github.com/black-sat/black/releases/download/v$BLACK_VERSION/black-sat-$BLACK_VERSION.ubuntu20.04.x86_64.deb
RUN apt-get install -y ./black-sat-$BLACK_VERSION.ubuntu20.04.x86_64.deb 

WORKDIR /purple/build

ENTRYPOINT ["/usr/bin/env", "--"]
