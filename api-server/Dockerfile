# Build stage
FROM gcc:12 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Install hiredis
RUN git clone https://github.com/redis/hiredis.git && \
    cd hiredis && \
    make && \
    make install && \
    cd .. && \
    rm -rf hiredis

# Install redis-plus-plus
RUN git clone https://github.com/sewenew/redis-plus-plus.git && \
    cd redis-plus-plus && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    make install && \
    cd ../.. && \
    rm -rf redis-plus-plus

# Install cpp-httplib
RUN git clone https://github.com/yhirose/cpp-httplib.git && \
    cd cpp-httplib && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make install && \
    cd ../.. && \
    rm -rf cpp-httplib

# Install nlohmann/json
RUN git clone https://github.com/nlohmann/json.git && \
    cd json && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make install && \
    cd ../.. && \
    rm -rf json

# Copy source code
WORKDIR /app
COPY . .

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j$(nproc)

# Runtime stage
FROM debian:bullseye-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl1.1 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary and libraries
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /app/build/api_server /app/api_server

# Update library cache
RUN ldconfig

WORKDIR /app

EXPOSE 8080

CMD ["./api_server", "config/api.json"]
