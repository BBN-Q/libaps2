FROM centos:6.7

MAINTAINER Colm Ryan <cryan@bbn.com>

#install gcc toolchain
RUN yum update -y
RUN yum install -y centos-release-scl-rh
RUN yum install -y devtoolset-3-gcc devtoolset-3-gcc-c++

#a few other tools
RUN yum install -y git cmake wget tar bzip2

#default to the devtools toolchain
RUN echo "source /opt/rh/devtoolset-3/enable" >> /root/.bashrc

#install hdf5
RUN source /opt/rh/devtoolset-3/enable && mkdir hdf5 && cd /hdf5 && \
  wget http://www.hdfgroup.org/ftp/HDF5/current/src/hdf5-1.8.17.tar.bz2 && \
  tar xjf hdf5-1.8.17.tar.bz2 && \
  cd hdf5-1.8.17 && \
  ./configure --prefix=/usr/local/hdf5 --enable-cxx && \
  make install
