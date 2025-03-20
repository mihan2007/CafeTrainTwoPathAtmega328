#include "routes.h"

// Таблица управления стрелками
const uint8_t SWITCH_MASKS[16] = {
	0b10000000, // RailroadSwitch 1 Left
	0b01000000, // RailroadSwitch 1 Right
	0b00100000, // RailroadSwitch 2 Left
	0b00010000, // RailroadSwitch 2 Right
	0b00001000, // RailroadSwitch 3 Left
	0b00000100, // RailroadSwitch 3 Right
	0b00000010, // RailroadSwitch 4 Left
	0b00000001, // RailroadSwitch 4 Right
	0b10000000, // RailroadSwitch 5 Left
	0b01000000, // RailroadSwitch 5 Right
	0b00100000, // RailroadSwitch 6 Left
	0b00010000, // RailroadSwitch 6 Right
	0b00001000, // RailroadSwitch 7 Left
	0b00000100, // RailroadSwitch 7 Right
	0b00000010, // RailroadSwitch 8 Left
	0b00000001, // RailroadSwitch 8 Right
};
// Table1 = 0b01
// Table2 = 0b101
// Определение маршрутов
Route routes[] = {
	{"Table 1", {1}, 1},
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
