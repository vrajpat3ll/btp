# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Update package lists and install necessary packages including GCC, SCTP development headers, and networking tools
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates \
    gcc \
    libc-dev \
    build-essential \
    libsctp-dev \
    net-tools \
    iputils-ping \
    tcpdump \
    vim \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install kubectl
RUN curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
    install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl && \
    rm kubectl


COPY . lb

RUN cd lb && make

# Expose the port the load balancer will listen on
EXPOSE 38412

# The container will run indefinitely to allow exec commands
CMD ["sleep", "infinity"]
