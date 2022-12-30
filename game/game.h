#ifndef GAME_GAME_H
#define GAME_GAME_H

#include <stdint-gcc.h>

enum map_object{
    wall,
    bush,
    campsite,
    one_coin,
    treasure,
    large_treasure,
    air,
    dropped_treasure,
    wild_beast,
    player_1,
    player_2,
    player_3,
    player_4

};


struct __attribute__((packed)) info_for_player{
    int position_x;
    int position_y;
    int deaths;
    int carried_coins;
    int saved_coins;
    uint16_t show_map[5][5];
    int round_number;
    int player_number;

};



#endif //GAME_GAME_H
