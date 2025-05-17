# строим
FROM gcc:11.3 as build

# установка зависимостей
RUN apt update && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

# копируем для докера
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan install .. --build=missing -s build_type=Release

COPY ./src /app/src
COPY CMakeLists.txt /app/

# билдим
RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . -- -j$(nproc)

# раним
FROM ubuntu:22.04 as run

# создание юзера
RUN groupadd -r www && useradd -r -g www www
USER www

# копируем билд
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data

# ставим точку входа
ENTRYPOINT ["/app/game_server", "/app/data/config.json"] 
