# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    wget \
    tar \
    git \
    build-essential \
    tcpdump \
    vim \
    iputils-ping \
    net-tools

# Install Go
RUN wget https://dl.google.com/go/go1.14.4.linux-amd64.tar.gz && \
    tar -C /usr/local -zxvf go1.14.4.linux-amd64.tar.gz && \
    rm go1.14.4.linux-amd64.tar.gz

# Set Go environment variables
ENV GOPATH=/root/go
ENV GOROOT=/usr/local/go
ENV PATH=$PATH:$GOPATH/bin:$GOROOT/bin

# Create Go directories
RUN mkdir -p $GOPATH/{bin,pkg,src}

# Clone the my5G-RANTester repository and build
COPY my5G-RANTester $GOPATH/src/my5G-RANTester
WORKDIR $GOPATH/src/my5G-RANTester/cmd

RUN go mod download && go build app.go

# Expose ports (adjust as necessary)
EXPOSE 9487
EXPOSE 2152

# Set entrypoint to run the binary
CMD ["sleep", "infinity"]

