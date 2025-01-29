# Use an official C++ base image
FROM ubuntu:20.04

# Set non-interactive frontend for apt
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    git \
    wget \
    curl \
    python3 \
    python3-pip \
    gdb \
    && apt-get clean

# Install Google Test
RUN apt-get update && apt-get install -y \
    libgtest-dev \
    && apt-get clean \
    && cd /usr/src/googletest \
    && cmake . \
    && make \
    && make install

# Working directory
WORKDIR /app

# Copy source code to the container
COPY . /app

# Create build directory, compile the project, and handle output binaries
RUN mkdir -p build \
    && cd build \
    && cmake .. \
    && make \
    && mkdir -p /app/bin \
    && cp ./bin/* /app/bin/ || true

# Ensure binaries exist in /app/bin
WORKDIR /app/bin

# Run tests first, then launch interactive main
CMD ["bash", "-c", "./TrafficMonitoringTests || echo 'Unit tests failed.'; ./interactive_main"]
