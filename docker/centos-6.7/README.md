Dockerfile for deploying to Centos 6.7

We could automate this but for now

1. Build the image
```shell
cd /path/to/here
docker build -t libaps2-centos-6.7 .
```
2. Run the image interactively
```shell
docker run --rm -it libaps2-centos-6.7
```
3. In the container clone, build and package
```shell
root@27d8bf239feb git clone --recursive https://github.com/BBN-Q/libaps2.git
root@27d8bf239feb cd libaps2
root@27d8bf239feb mkdir build
root@27d8bf239feb cd build
root@27d8bf239feb cmake ../src
root@27d8bf239feb make
root@27d8bf239feb make package
```
4. From outside the container copy the `tar.gz` package
```shell
docker cp 27d8bf239feb:/libaps2/build/libaps2-1.0.0-Linux.tar.gz .
```
