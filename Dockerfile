# Сборка сервера
## Создание образа build
FROM gcc:11.3 as build
LABEL stage=builder

## Установка необходимых утилит
RUN apt update \
 && apt install -y python3-pip \
                   cmake \
 && pip3 install conan==1.55 \
 && rm -rf /var/lib/apt/lists/*

## Копирование исходников в образ
COPY ./lib /app/lib
COPY ./src /app/src
COPY CMakeLists.txt conanfile.txt /app/

## Сборка исходников сервера
RUN mkdir /app/build  \
 && cd /app/build  \
 && conan install .. --build=missing -s build_type=Release \
                                     -s compiler.libcxx=libstdc++11 \
 && cmake -DCMAKE_BUILD_TYPE=Release .. \
 && cmake --build . --target game_server

# Запуск сервера
## Создание образа run
FROM ubuntu:22.04 as run
LABEL stage=runner

## Установка необходимых утилит
RUN apt update  \
 && apt install -y rsyslog \
                   net-tools

## Копирование конфигурационных и статических данных в образ 
COPY --from=build app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

## Создание директории для сохранений
RUN mkdir /app/save_files

## Создание нового пользователя www и выдача ему прав на логгирование
RUN groupadd -r www \
 && useradd -r -g www www \
 && usermod -a -G syslog www

## Изменение прав владения директорией /app
RUN chown www /app

## Переключение на пользователя www
USER www

## Копирование скрипта для запуска сервера
COPY server_starter.sh /

## Открытие портов
EXPOSE 8080

## Параметры для запуска сервера 
CMD ["-c", "/app/data/config.json", "-w", "/app/static/", \ 
     "-t", "30", "--randomize-spawn-points", \
     "--state-file", "/app/save_files/save.txt", "--save-state-period", "3000"]

## Запуск сервера
ENTRYPOINT ["./server_starter.sh"]
