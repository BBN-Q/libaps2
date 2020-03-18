FROM centos:6.7

MAINTAINER Colm Ryan <cryan@bbn.com>

# install gcc toolchain
RUN yum update -y
RUN yum install -y centos-release-scl-rh
RUN yum install -y devtoolset-3-gcc devtoolset-3-gcc-c++

# a few other tools
RUN yum install -y git cmake wget tar bzip2

# default to the devtools toolchain
RUN echo "source /opt/rh/devtoolset-3/enable" >> /root/.bashrc

# install a newer cmake
RUN source /opt/rh/devtoolset-3/enable && mkdir cmake && cd /cmake && \
  wget https://cmake.org/files/v3.6/cmake-3.6.3.tar.gz && \
  tar xf cmake-3.6.3.tar.gz && \
  cd cmake-3.6.3 && \
  cmake . && \
  make && \
  make install

ENTRYPOINT ["/bin/bash"]
