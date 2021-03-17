FROM  ubuntu:focal

ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Europe/Prague

# Install essentials
RUN apt-get -y update && apt-get install -y cmake build-essential git

# Install NLOPT
RUN git clone git://github.com/stevengj/nlopt && cd nlopt && mkdir build && cd build && cmake .. && make && make install && cd ../../

# Install Eigen
RUN git clone https://gitlab.com/libeigen/eigen && cp -r ./eigen ./src

# Copy and compile SEIR
COPY ./ /
RUN cd ./src && cmake CMakeLists.txt && make

# This command runs your application, comment out this line to compile only
CMD ["./src/seirfilter"]
