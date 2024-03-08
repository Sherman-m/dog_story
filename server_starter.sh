#!/bin/bash

# Экспорт переменных окруженяи
export DOCKER_HOST_IP=$(route -n | awk '/UG[ \t]/{print $2}')
export GAME_DB_URL="postgres://USERNAME:PASSWORD@$(printenv DOCKER_HOST_IP):30432/DB_NAME"

# Запуск сервера
./app/game_server $*
