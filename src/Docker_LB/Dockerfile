# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Update package lists and install necessary packages including GCC, SCTP development headers, and networking tools
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    ca-certificates \
    gcc \
    libc-dev \
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


COPY load_balancer.c /load_balancer.c

COPY consistent_hashing.c /consistent_hashing.c

# COPY ud.c /ud.c

COPY reactive_lb.c /reactive_lb.c

COPY threshold_lb.c /threshold_lb.c

# Compile the load_balancer.c source file with SCTP support
RUN gcc -o /load_balancer /load_balancer.c -lsctp

RUN gcc -o /consistent_hashing /consistent_hashing.c -lsctp

# RUN gcc -o /ud /ud.c -lsctp

RUN gcc -o /reactive_lb /reactive_lb.c -lsctp

RUN gcc -o /threshold_lb /threshold_lb.c -lsctp

# Expose the port the load balancer will listen on
EXPOSE 38412

# The container will run indefinitely to allow exec commands
CMD ["sleep", "infinity"]
