#!/bin/bash

clang-format --style=Google -i lib/json_loader/*
clang-format --style=Google -i lib/model/*
clang-format --style=Google -i lib/util/*
clang-format --style=Google -i src/app/*
clang-format --style=Google -i src/db/*
clang-format --style=Google -i src/http_handler/*
clang-format --style=Google -i src/http_server/*
clang-format --style=Google -i src/logger/*
clang-format --style=Google -i src/serialization/*
clang-format --style=Google -i src/main.cpp
clang-format --style=Google -i tests/*
