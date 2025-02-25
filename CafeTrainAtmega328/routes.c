#include "routes.h"

// Таблица управления стрелками
const uint8_t SWITCH_MASKS[16] = {
	0b00000000, // C0  RailroadSwitch 1 Left
	0b00010000, // C1  RailroadSwitch 1 Right
	0b00001000, // C2  RailroadSwitch 2 Left
	0b00011000, // C3  RailroadSwitch 2 Right
	0b00000100, // C4  RailroadSwitch 3 Left
	0b00010100, // C5  RailroadSwitch 3 Right
	0b00001100, // C6  RailroadSwitch 4 Left
	0b00011100, // C7  RailroadSwitch 4 Right
	0b00000001, // C8  RailroadSwitch 5 Left
	0b00010001, // C9  RailroadSwitch 5 Right
	0b00001001, // C10 RailroadSwitch 6 Left
	0b00011001, // C11 RailroadSwitch 6 Right
	0b00000101, // C12 RailroadSwitch 7 Left
	0b00010101, // C13 RailroadSwitch 7 Left
	0b00001101, // C14 Move Backward
	0b00011101  // C15 Move Forward
};

// Определение маршрутов
Route routes[] = {
	{"Table 1", {2}, 1},
	{"Table 2", {1, 4}, 2},
	{"Table 3", {1, 3, 6}, 3},
	{"Table 4", {1, 3, 5, 8}, 4},
	{"Table 5", {1, 3, 5, 7, 10}, 5},
	{"Table 6", {1, 3, 5, 7, 9, 12}, 6},
	{"Table 7", {1, 3, 5, 7, 9, 11, 14}, 7},
	{"Table 8", {1, 3, 5, 7, 9, 11, 13}, 7}
};

// Вычисляем количество маршрутов (теперь это возможно!)
const uint8_t NUM_ROUTES = sizeof(routes) / sizeof(routes[0]);
