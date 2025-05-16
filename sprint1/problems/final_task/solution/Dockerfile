# Build stage
FROM gcc:11.3 as build

# Install dependencies
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# Configure conan for faster builds
RUN conan config init --force && \
    conan config set general.revisions_enabled=1 && \
    conan config set general.parallel_download=8 && \
    conan config set general.parallel_build=8

# Copy conanfile first to leverage Docker cache
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s build_type=Release

# Copy source files
COPY ./src /app/src
COPY CMakeLists.txt /app/

# Build the application with parallel compilation
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . -- -j$(nproc)

# Run stage
FROM ubuntu:22.04 as run

# Create non-root user
RUN groupadd -r www && useradd -r -g www www
USER www

# Copy built application and data
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data

# Set entrypoint
ENTRYPOINT ["/app/game_server", "/app/data/config.json"] 