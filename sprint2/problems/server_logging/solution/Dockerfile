FROM ubuntu:22.04

RUN apt-get update && \
    apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .
COPY data ./data

RUN mkdir build && \
    cd build && \
    cmake .. && \
    make

RUN echo '--- /app/build contents ---' && ls -l /app/build && echo '--- Try running server_logging manually ---' && (cd /app/build && ./server_logging || echo 'server_logging failed to run')

EXPOSE 8080

CMD ["./build/server_logging"] 