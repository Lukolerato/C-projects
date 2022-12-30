#ifndef UNTITLED_SERVER_H
#define UNTITLED_SERVER_H


#include <stdint-gcc.h>

#define up 1
#define down  2
#define right  3
#define left  4
#define no_move 5
#define quit  6

void init_map();
void show_map();
void* keyboard(); //dodawanie itemów na mape i tworzenie wątku bestii
void add_one_coin();
void add_treasure();
void add_large_treasure();
void* main_loop(); //1 wątek gry
void spawn_beast();
void starting_point(int slot_id);
void* waiting(); //1 odpala player_receiver
int find_free_slot();
void* player_receiver(void* id); //4
void make_move(int id, int direction);
enum map_object get_player_object(int id);
void send_info_to_client(int id);
void* choose_beast_move(void* id); //100
void make_beast_move(int id, int direction);
void send_info_to_beast(int id);
void print_stats();

struct player{
    int position_x;
    int position_y;
    int start_x;
    int start_y;
    int deaths;
    int carried_coins;
    int saved_coins;
    int free_slot; //0-free, 1-taken
    int socket;
    int player_in_bush; //0-out, 1-in
    int place_on_map;
    int next_move;
};

struct beast{
    int position_x;
    int position_y;
    int next_move;
    int place_on_map;
    int beast_in_bush; //0-out, 1-in
    int show_map[5][5];
};

struct dropped_treasure{
    int position_x;
    int position_y;
    int dropped_coins; //-1-free, 0<-used
};


#endif //UNTITLED_SERVER_H
