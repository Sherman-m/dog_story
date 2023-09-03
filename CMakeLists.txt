#Настроим конфигурацию cmake проекта
cmake_minimum_required(VERSION 3.11)
project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

#Подключим файл conanbuildinfo_multi.cmake
include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

#Просим BOOST_BEAST использовать std::string_view
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

#Найдем библиотеку Threads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#Добавим исходники модуля app
set(APP
  src/app/player.h
  src/app/player.cpp
  src/app/players_table.h
  src/app/players_table.cpp
  src/app/application.h
  src/app/application.cpp
  src/app/token.h
  src/app/token.cpp
  src/app/strand_storage.h
  src/app/strand_storage.cpp
  src/app/ticker.h
  src/app/ticker.cpp)

#Добавим исходники модуля http_handler
set(HTTP_HANDLER
  src/http_handler/request_handler.h
  src/http_handler/request_handler.cpp
  src/http_handler/api_handler.h
        src/http_handler/api_handler.cpp
  src/http_handler/response_generators.h
  src/http_handler/response_generators.cpp
  src/http_handler/api_serializer.h
  src/http_handler/api_serializer.cpp)

#Добавим исходники модуля http_server
set(HTTP_SERVER
  src/http_server/http_server.h
  src/http_server/http_server.cpp)

#Добавим исходники модуля json_loader
set(JSON_LOADER
  lib/json_loader/json_loader.h
  lib/json_loader/json_loader.cpp
  lib/json_loader/loot_types_storage.h
  lib/json_loader/loot_types_storage.cpp)

#Добавим исходники модуля logger
set(LOGGER
  src/logger/logger.h
  src/logger/logger.cpp)

#Добавляем исходники модуля model
set(MODEL
  lib/model/model.h
  lib/model/road.h
  lib/model/road.cpp
  lib/model/building.h
  lib/model/building.cpp
  lib/model/office.h
  lib/model/office.cpp
  lib/model/map.h
  lib/model/map.cpp
  lib/model/game.h
  lib/model/game.cpp
  lib/model/geometry.h
  lib/model/geometry.cpp
  lib/model/dog.h
  lib/model/dog.cpp
  lib/model/game_session.h
  lib/model/game_session.cpp
  lib/model/loot_generator.h
  lib/model/loot_generator.cpp
  lib/model/lost_object.h
  lib/model/lost_object.cpp
  lib/model/collision_detector.h
  lib/model/collision_detector.cpp)

#Добавим исходники модуля util
set(UTIL
  lib/util/sdk.h
  lib/util/tagged.h
  lib/util/boost_json.cpp
  lib/util/file_handler.h
  lib/util/file_handler.cpp
  lib/util/program_options_parser.h
  lib/util/program_options_parser.cpp)

#Создадим переменную с исходниками приложения
set(SRC
  src/main.cpp
  ${APP}
  ${HTTP_HANDLER}
  ${HTTP_SERVER}
  ${LOGGER})

#Создадим переменную с исходниками библиотеки
set(LIB
  ${JSON_LOADER}
  ${MODEL}
  ${UTIL})

#Добавим статическую библиотеку
add_library(game_lib STATIC ${LIB})

#Добавим сторонние библиотеки к статической библиотеке
target_include_directories(game_lib PUBLIC CONAN_PKG::boost)
target_link_libraries(game_lib PUBLIC
  CONAN_PKG::boost
  Threads::Threads)

#Добавим цель для приложения
add_executable(game_server ${SRC})

#Добавим зависимость цели для приложения от статической библиотеки.
#target_include_directories() добавится автоматически из зависимой библиотеки.
target_link_libraries(game_server PRIVATE game_lib)

#Если тип сборки-- Debug, то добавляем тесты.
if (CMAKE_BUILD_TYPE STREQUAL Debug)

  #Создадим переменную со всеми тестами
  set(TESTS
    tests/loot_generator_tests.cpp
    tests/collision-detector-tests.cpp)

  #Добавим цель для тестов
  add_executable(game_server_tests ${TESTS})

  #Добавим зависимость тестов от фреймворка Catch2 и статической библиотеки
  target_link_libraries(game_server_tests PRIVATE
    CONAN_PKG::catch2
    game_lib)

  #Интегрируем Catch2 с CTest
  include(CTest)
  include(${CONAN_BUILD_DIRS_CATCH2_DEBUG}/Catch.cmake)
  catch_discover_tests(game_server_tests)
endif()
