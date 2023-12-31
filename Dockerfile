FROM gcc:11.3 as build
LABEL stage=builder
RUN apt update \
 && apt install -y \
    python3-pip \
    cmake \
 && pip3 install conan==1.55 \
 && rm -rf /var/lib/apt/lists/*

COPY conanfile.txt /app/
RUN mkdir /app/build  \
 && cd /app/build  \
 && conan install .. --build=missing -s build_type=Release \
                                     -s compiler.libcxx=libstdc++11

COPY ./lib /app/lib
COPY ./src /app/src
COPY CMakeLists.txt /app/

RUN cd /app/build  \
 && cmake -DCMAKE_BUILD_TYPE=Release .. \
 && cmake --build . --target game_server

FROM ubuntu:22.04 as run
RUN apt update  \
 && apt install -y rsyslog

RUN groupadd -r www && useradd -r -g www www
RUN usermod -a -G syslog www

COPY --from=build app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

RUN chown www /app
USER www

ENV GAME_DB_URL="postgres://sherman:Postpasswgreordsql122104@84.201.137.248:30432/dog_story_db"

ENTRYPOINT ["/app/game_server", "-c" ,"/app/data/config.json", "-w", "/app/static/"]

EXPOSE 8080