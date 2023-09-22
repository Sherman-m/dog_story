#pragma once

// В модуле serialization хранятся все классы и функции, позволяющие
// сериализовать объекты игры:
//  - Простые объекты, такие как точки, прямоугольники и т. д.;
//  - Дороги;
//  - Потерянные предметы;
//  - Собаки;
//  - Игровые сессии;
//  - Игроки.

#include "serialized_dog.h"
#include "serialized_game_session.h"
#include "serialized_geometry.h"
#include "serialized_lost_object.h"
#include "serialized_player.h"
#include "serialized_road.h"