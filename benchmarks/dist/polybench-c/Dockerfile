FROM ubuntu:18.04

# Baseline components
RUN apt-get update && \
    apt-get install -y \
    build-essential \
    clang-10 \
    make \
    perl \
    ca-certificates \
    wget \
    && rm -rf /var/lib/apt/lists/*

# Create symbolic links for clang
RUN ln -s /usr/bin/clang-10 /usr/bin/clang
RUN ln -s /usr/bin/clang++-10 /usr/bin/clang++

ENTRYPOINT [ "/polybench/build.sh" ]