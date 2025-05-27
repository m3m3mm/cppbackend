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

EXPOSE 8080

CMD ["sh", "-c", "./build/server_logging & sleep 5"] 