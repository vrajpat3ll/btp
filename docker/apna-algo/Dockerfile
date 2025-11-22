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
    iproute2 \ 
    tcpdump \
    vim \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Install kubectl
RUN curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl" && \
    install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl && \
    rm kubectl

# Install uv + Python + pip + packages
RUN curl -LsSf https://astral.sh/uv/install.sh | sh && \
    ln -s /root/.local/bin/uv /usr/local/bin/uv && \
    uv python install 3.12 && \
    uv venv /opt/venv && \
    . /opt/venv/bin/activate && \
    uv pip install psutil

ENV PATH="/opt/venv/bin:${PATH}"

COPY src/apna-algo lb

RUN cd lb && make

# Expose the port the load balancer will listen on
EXPOSE 38412

# The container will run indefinitely to allow exec commands
CMD ["sleep", "infinity"]
