# Start from the official GCC image (Debian-based)
FROM gcc:latest

# Install required packages and CMake
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        cmake \
        git \
        make \
        ninja-build \
        && rm -rf /var/lib/apt/lists/*

# Checkout the "duckdb-strict-cast" extension
WORKDIR /home
RUN git clone https://github.com/slvoinea/duckdb-strict-cast.git

# Update submodules
WORKDIR /home/duckdb-strict-cast
RUN git submodule update --init --recursive

# Build extension
RUN GEN=ninja make

# Run duckdb
WORKDIR /home
CMD ["./duckdb-strict-cast/build/release/duckdb"]