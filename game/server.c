#include "server.h"
#include "game.h"
#include <stdio.h>
#include <pthread.h>
#include <ncurses.h>
#include <stdint.h>
#include <locale.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define map_height 25
#define map_width 51
#define max_number_of_beasts 100

pthread_mutex_t mutex_player[4];
pthread_mutex_t mutex_beast[100];
pthread_mutex_t mutex_map;


int rounds=0;

uint16_t map[map_height][map_width];
//x-wysokosc od gory ekranu
//y-szerokosc od lewej

struct player players[4];
struct beast beasts[max_number_of_beasts];
struct dropped_treasure droppedTreasure[100];

int number_of_beast = 0;

pthread_t pthread_player_tab[4];
pthread_t pthread_beast_tab[100];

int main()
{
    sigaction(SIGPIPE, &(struct sigaction){SIG_IGN}, NULL);
    time_t t;
    srand((unsigned) time(&t));
    setlocale(LC_ALL, "");
    initscr();            /* Start curses mode           */
    init_map();

    for (int i = 0; i < 4; ++i) {
        pthread_mutex_init(&mutex_player[i],NULL);
    }
    for (int i = 0; i < 100; ++i) {
        pthread_mutex_init(&mutex_beast[i],NULL);
    }
    pthread_mutex_init(&mutex_map,NULL);

    for (int i = 0; i < 100; ++i) {
        droppedTreasure[i].dropped_coins = -1;
    }

    for (int i = 0; i < 4; ++i) {
        players[i].free_slot = 0;
    }

    pthread_t pthread_2;
    pthread_create(&pthread_2, NULL, waiting, NULL);

    pthread_t pthread_1;
    pthread_create(&pthread_1, NULL, main_loop, NULL);


    keyboard();
    getchar();

    for (int i = 0; i < 4; ++i) {
        pthread_mutex_destroy(&mutex_player[i]);
    }
    for (int i = 0; i < 100; ++i) {
        pthread_mutex_destroy(&mutex_beast[i]);
    }
    pthread_mutex_destroy(&mutex_map);

    endwin();            /* End curses mode          */
    return 0;
}

void* main_loop(){
    for(;;){
        for (int i = 0; i < number_of_beast; ++i) {
            pthread_mutex_lock(&mutex_beast[i]);
            pthread_mutex_lock(&mutex_map);
            send_info_to_beast(i);
            make_beast_move(i,beasts[i].next_move);
            beasts[i].next_move = no_move;
            pthread_mutex_unlock(&mutex_map);
            pthread_mutex_unlock(&mutex_beast[i]);
        }
        for (int i = 0; i < 4; ++i) {
            if(players[i].free_slot == 1){
                pthread_mutex_lock(&mutex_player[i]);
                pthread_mutex_lock(&mutex_map);
                make_move(i,players[i].next_move);
                players[i].next_move = no_move;
                send_info_to_client(i);
                pthread_mutex_unlock(&mutex_map);
                pthread_mutex_unlock(&mutex_player[i]);
            }
        }
        show_map();
        print_stats();
        refresh();
        sleep(1);
        rounds++;
    }
}
void init_map(){
    FILE *f = fopen("map.txt","r");
    for (int i = 0; i <map_height; ++i) {
        for (int j = 0; j < map_width; ++j) {
            int c = fgetc(f);
            if(c==10 || c==13){
                --j;
                continue;
            }

            if(c==32){
                map[i][j]=6;
            } else{
                map[i][j]=c-48;
            }
        }
    }
    fclose(f);
}
void show_map(){
    for (int i = 0; i < map_height; ++i) {
        for (int j = 0; j < map_width; ++j) {
            switch(map[i][j]){
                case wall:
                    mvprintw(i,j,"█");
                    break;
                case bush:
                    mvprintw(i,j,"#");
                    break;
                case wild_beast:
                    mvprintw(i,j,"*");
                    break;
                case one_coin:
                    mvprintw(i,j,"c");
                    break;
                case treasure:
                    mvprintw(i,j,"t");
                    break;
                case large_treasure:
                    mvprintw(i,j,"T");
                    break;
                case campsite:
                    mvprintw(i,j,"A");
                    break;
                case dropped_treasure:
                    mvprintw(i,j,"D");
                    break;
                case air:
                    mvprintw(i,j," ");
                    break;
                case player_1:
                    mvprintw(i,j,"1");
                    break;
                case player_2:
                    mvprintw(i,j,"2");
                    break;
                case player_3:
                    mvprintw(i,j,"3");
                    break;
                case player_4:
                    mvprintw(i,j,"4");
                    break;
            }
        }
    }
    refresh();            /* Print it on to the real screen */
}
void* keyboard(){
    for (;;) {
        int input = getchar();
        int nob;
        switch(input){
            case 'b':
            case 'B':
                pthread_mutex_lock(&mutex_map);
                spawn_beast();
                pthread_mutex_unlock(&mutex_map);
                nob = number_of_beast-1;
                pthread_create(&pthread_beast_tab[nob], NULL, choose_beast_move, &nob);

                break;
            case 'c':
                //mvprintw(60,10,"nacisnieto c");
                refresh();
                pthread_mutex_lock(&mutex_map);
                add_one_coin();
                pthread_mutex_unlock(&mutex_map);
                break;
            case 't':
                pthread_mutex_lock(&mutex_map);
                add_treasure();
                pthread_mutex_unlock(&mutex_map);
                break;
            case 'T':
                pthread_mutex_lock(&mutex_map);
                add_large_treasure();
                pthread_mutex_unlock(&mutex_map);
                break;
            case 'q':
            case 'Q':
                //quit
                break;
            default:
                break;
        }
    }
    return NULL;
}
void add_one_coin(){
    for (;;) {
        int x = rand() % map_height;
        int y = rand() % map_width;

        if (map[x][y] == air) {
            map[x][y] = one_coin;
            return;
        }
    }
}
void add_treasure(){
    for (int i = 0; i < 1000; ++i) {
        int x = rand() % map_height;
        int y = rand() % map_width;

        if (map[x][y] == air) {
            map[x][y] = treasure;
            return;
        }
    }
}
void add_large_treasure(){
    for (int i = 0; i < 1000; ++i) {
        int x = rand() % map_height;
        int y = rand() % map_width;

        if (map[x][y] == air) {
            map[x][y] = large_treasure;
            return;
        }
    }
}
int find_free_slot(){
    for (int i = 0; i < 4; ++i) {
        pthread_mutex_lock(&mutex_player[i]);
        if(players[i].free_slot == 0){
            pthread_mutex_unlock(&mutex_player[i]);
            return i;
        }
        pthread_mutex_unlock(&mutex_player[i]);
    }
    return -1; //all seats are taken
}
enum map_object get_player_object(int id){

    if(id==0){
        return player_1;
    }
    if(id==1){
        return player_2;
    }
    if(id==2){
        return player_3;
    }
    if(id==3){
        return player_4;
    }
}
void starting_point(int slot_id){

    for (;;) {
        int x = rand() % map_height;
        int y = rand() % map_width;

        if (map[x][y] == air) {
            map[x][y]= get_player_object(slot_id);

            players[slot_id].position_x=x;
            players[slot_id].position_y=y;
            players[slot_id].start_x=x;
            players[slot_id].start_y=y;

            return;
        }
    }
}
void* waiting(){

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        printf("creating socket err");
        return NULL;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int t = 1;
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&t,sizeof(int));

    int bind_return = bind(sockfd,(struct sockaddr*)&server_addr,sizeof (server_addr));

    if(bind_return==-1){
        printf("binding socket err");
        return NULL;
    }

    int listen_return = listen(sockfd,100);
    if(listen_return==-1){
        printf("listening socket err");
        return NULL;
    }

    for (;;) {
        int client_socket = accept(sockfd,NULL,NULL);
        int free_slot = find_free_slot();
        if(free_slot==-1){
            int response_info = 0;
            send(client_socket,&response_info,sizeof(response_info),0);
            continue;
        }
        else{
            int response_info = 1;
            send(client_socket,&response_info,sizeof(response_info),0);

            pthread_mutex_lock(&mutex_player[free_slot]);
            starting_point(free_slot);
            players[free_slot].free_slot = 1;
            players[free_slot].deaths = 0;
            players[free_slot].carried_coins = 0;
            players[free_slot].saved_coins = 0;
            players[free_slot].socket = client_socket;
            players[free_slot].place_on_map = air;
            players[free_slot].player_in_bush = 0;
            pthread_create(&pthread_player_tab[free_slot], NULL, player_receiver, &free_slot);
            pthread_mutex_unlock(&mutex_player[free_slot]);
        }
    }
}
void spawn_beast(){
    for (;;) {
        int x = rand() % map_height;
        int y = rand() % map_width;

        if (map[x][y] == air) {
            map[x][y] = wild_beast;
            beasts[number_of_beast].place_on_map = air;
            beasts[number_of_beast].beast_in_bush = 0;
            beasts[number_of_beast].position_x = x;
            beasts[number_of_beast].position_y = y;
            beasts[number_of_beast].next_move = no_move;
            number_of_beast++;
            return;
        }
    }
}
void* choose_beast_move(void* id){
    int beast_id = *(int*)id;
    int player_direction = -1;

    for (;;) {
        //checking up from beast pionowo
        if(beasts[beast_id].show_map[1][2] == player_1 ||beasts[beast_id].show_map[1][2] == player_2 ||beasts[beast_id].show_map[1][2] == player_3 ||beasts[beast_id].show_map[1][2] == player_4){
            player_direction = up;
        }
        else if(beasts[beast_id].show_map[1][2] != wall){
            if(beasts[beast_id].show_map[0][2] == player_1 ||beasts[beast_id].show_map[0][2] == player_2 ||beasts[beast_id].show_map[0][2] == player_3 ||beasts[beast_id].show_map[0][2] == player_4){
                player_direction = up;
            }
        }
        //checking down from beast pionowo
        if(beasts[beast_id].show_map[3][2] == player_1 ||beasts[beast_id].show_map[3][2] == player_2 ||beasts[beast_id].show_map[3][2] == player_3 ||beasts[beast_id].show_map[3][2] == player_4){
            player_direction = down;
        }
        else if(beasts[beast_id].show_map[3][2] != wall){
            if(beasts[beast_id].show_map[4][2] == player_1 ||beasts[beast_id].show_map[4][2] == player_2 ||beasts[beast_id].show_map[4][2] == player_3 ||beasts[beast_id].show_map[4][2] == player_4){
                player_direction = down;
            }
        }
        //checking left from beast poziomo
        if(beasts[beast_id].show_map[2][1] == player_1 ||beasts[beast_id].show_map[2][1] == player_2 ||beasts[beast_id].show_map[2][1] == player_3 ||beasts[beast_id].show_map[2][1] == player_4){
            player_direction = left;
        }
        else if(beasts[beast_id].show_map[2][1] != wall){
            if(beasts[beast_id].show_map[2][0] == player_1 ||beasts[beast_id].show_map[2][0] == player_2 ||beasts[beast_id].show_map[2][0] == player_3 ||beasts[beast_id].show_map[2][0] == player_4){
                player_direction = left;
            }
        }
        //checking right from beast poziomo
        if(beasts[beast_id].show_map[2][3] == player_1 ||beasts[beast_id].show_map[2][3] == player_2 ||beasts[beast_id].show_map[2][3] == player_3 ||beasts[beast_id].show_map[2][3] == player_4){
            player_direction = right;
        }
        else if(beasts[beast_id].show_map[2][3] != wall){
            if(beasts[beast_id].show_map[2][4] == player_1 ||beasts[beast_id].show_map[2][4] == player_2 ||beasts[beast_id].show_map[2][4] == player_3 ||beasts[beast_id].show_map[2][4] == player_4){
                player_direction = right;
            }
        }

        if(player_direction !=-1){
            pthread_mutex_lock(&mutex_beast[beast_id]);
            beasts[beast_id].next_move = player_direction;
            pthread_mutex_unlock(&mutex_beast[beast_id]);
            continue;
        }

        int rand_move = rand()%5;
        pthread_mutex_lock(&mutex_beast[beast_id]);
        beasts[beast_id].next_move = rand_move;
        pthread_mutex_unlock(&mutex_beast[beast_id]);

    }
    
    return NULL;
}
void make_beast_move(int id, int direction){

    if(direction==no_move){
        return;
    }
    if(beasts[id].beast_in_bush == 1){
        beasts[id].beast_in_bush = 0;
        return;
    }

    int new_place = -1;

    switch(direction){
        case up:
            new_place = map[beasts[id].position_x-1][beasts[id].position_y];
            break;
        case down:
            new_place = map[beasts[id].position_x+1][beasts[id].position_y];
            break;
        case right:
            new_place = map[beasts[id].position_x][beasts[id].position_y+1];
            break;
        case left:
            new_place = map[beasts[id].position_x][beasts[id].position_y-1];
            break;
    }
    if(new_place == -1){
        return;
    }
    if(new_place == wall){
        return;
    }

    map[beasts[id].position_x][beasts[id].position_y] = beasts[id].place_on_map;
    int player_id;

    switch (new_place) {
        case bush:
            beasts[id].place_on_map = bush;
            beasts[id].beast_in_bush = 1;
            break;
        case campsite:
            beasts[id].place_on_map = campsite;
            break;
        case one_coin:
            beasts[id].place_on_map = one_coin;
            break;
        case treasure:
            beasts[id].place_on_map = treasure;
            break;
        case large_treasure:
            beasts[id].place_on_map = large_treasure;
            break;
        case air:
            beasts[id].place_on_map = air;
            break;
        case dropped_treasure:
            beasts[id].place_on_map = dropped_treasure;
            break;
        case wild_beast:
            beasts[id].place_on_map = wild_beast;
            break;
        case player_1:
        case player_2:
        case player_3:
        case player_4:

            for (int i = 0; i < 4; ++i) {
                if(players[i].free_slot == 1){
                    if(new_place==player_1){
                        player_id = 0;
                    }
                    if(new_place==player_2){
                        player_id = 1;
                    }
                    if(new_place==player_3){
                        player_id = 2;
                    }
                    if(new_place==player_4){
                        player_id = 3;
                    }
                }
            }

            for (int i = 0; i < 100; ++i) {
                if(droppedTreasure[i].dropped_coins == -1){
                    if(players[player_id].carried_coins==0){
                        break;
                    }
                    droppedTreasure[i].position_x = players[player_id].position_x;
                    droppedTreasure[i].position_y = players[player_id].position_y;
                    droppedTreasure[i].dropped_coins = players[player_id].carried_coins;
                }
            }

            if(players[player_id].carried_coins==0){
                beasts[id].place_on_map = players[player_id].place_on_map;
            }
            else{
                beasts[id].place_on_map = dropped_treasure;
            }


            pthread_mutex_lock(&mutex_player[player_id]);
            players[player_id].deaths +=1;
            players[player_id].place_on_map = air;
            players[player_id].carried_coins = 0;
            players[player_id].position_x = players[player_id].start_x;
            players[player_id].position_y = players[player_id].start_y;
            pthread_mutex_unlock(&mutex_player[player_id]);

            map[players[player_id].position_x][players[player_id].position_y] = get_player_object(player_id);
    }

    switch (direction) {
        case up:
            beasts[id].position_x -=1;
            break;
        case down:
            beasts[id].position_x +=1;
            break;
        case right:
            beasts[id].position_y +=1;
            break;
        case left:
            beasts[id].position_y -=1;
            break;
    }
    map[beasts[id].position_x][beasts[id].position_y] = wild_beast;

}
void send_info_to_beast(int id){
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            beasts[id].show_map[i][j] = air;
        }
    }

    int i = beasts[id].position_x-2;
    int j = beasts[id].position_y-2;

    if(i < 0){
        i=0;
    }
    if(j<0){
        j=0;
    }

    int position_down = beasts[id].position_x+2;
    int position_right = beasts[id].position_y+2;

    if(position_down>map_height-1){
        position_down = map_height-1;
    }
    if(position_right>map_width-1){
        position_right = map_width-1;
    }

    int j_copy =j;
    for (int x = 0; i <= position_down; ++i,++x) {
        j = j_copy;
        for (int y=0; j <= position_right; ++j, ++y) {
            beasts[id].show_map[x][y]=map[i][j];
        }
    }
    
}


void* player_receiver(void* id){

    int player_id = *(int*)id;
    int information_from_client;

    for(;;){

        int receive = recv(players[player_id].socket,&information_from_client,sizeof (information_from_client),0);
        if(receive == -1 || receive ==0){
            players[player_id].free_slot = 0;
            map[players[player_id].position_x][players[player_id].position_y] = air;
            return NULL;
        }

        switch (information_from_client) {
            case up:
                pthread_mutex_lock(&mutex_player[player_id]);
                players[player_id].next_move = up;
                pthread_mutex_unlock(&mutex_player[player_id]);
                break;
            case down:
                pthread_mutex_lock(&mutex_player[player_id]);
                players[player_id].next_move = down;
                pthread_mutex_unlock(&mutex_player[player_id]);
                break;
            case right:
                pthread_mutex_lock(&mutex_player[player_id]);
                players[player_id].next_move = right;
                pthread_mutex_unlock(&mutex_player[player_id]);
                break;
            case left:
                pthread_mutex_lock(&mutex_player[player_id]);
                players[player_id].next_move = left;
                pthread_mutex_unlock(&mutex_player[player_id]);
                break;
            case quit:
                //
                break;
            default:
                break;

        }

    }
}
void make_move(int id, int direction){
    if(direction==no_move){
        return;
    }
    if(players[id].player_in_bush == 1){
        players[id].player_in_bush = 0;
        return;
    }

    int new_place = -1;

    switch(direction){
        case up:
            new_place = map[players[id].position_x-1][players[id].position_y];
            break;
        case down:
            new_place = map[players[id].position_x+1][players[id].position_y];
            break;
        case right:
            new_place = map[players[id].position_x][players[id].position_y+1];
            break;
        case left:
            new_place = map[players[id].position_x][players[id].position_y-1];
            break;
    }
    if(new_place == wall){
        return;
    }
    map[players[id].position_x][players[id].position_y] = players[id].place_on_map;
    int id_of_sec_player;
    switch (new_place) {
        case bush:
            players[id].place_on_map = bush;
            players[id].player_in_bush = 1;
            break;
        case campsite:
            players[id].place_on_map = campsite;
            players[id].saved_coins += players[id].carried_coins;
            players[id].carried_coins = 0;
            break;
        case one_coin:
            players[id].place_on_map = air;
            players[id].carried_coins +=1;
            break;
        case treasure:
            players[id].place_on_map = air;
            players[id].carried_coins +=10;
            break;
        case large_treasure:
            players[id].place_on_map = air;
            players[id].carried_coins +=50;
            break;
        case air:
            players[id].place_on_map = air;
            break;
        case dropped_treasure:
            players[id].place_on_map = air;
            for (int i = 0; i < 100; ++i) {
                if(players[id].position_x == droppedTreasure[i].position_x && players[id].position_y == droppedTreasure[i].position_y){
                    players[id].carried_coins += droppedTreasure[i].dropped_coins;
                    droppedTreasure[i].dropped_coins=-1;
                }
            }
            break;
        case wild_beast:

            for (int i = 0; i < 100; ++i) {
                if(players[id].carried_coins ==0){
                    break;
                }
                if(droppedTreasure[i].dropped_coins == -1){
                    droppedTreasure[i].position_x = players[id].position_x;
                    droppedTreasure[i].position_y = players[id].position_y;
                    droppedTreasure[i].dropped_coins = players[id].carried_coins;
                }
            }
            if(players[id].carried_coins ==0){
                map[players[id].position_x][players[id].position_y] = players[id].place_on_map;
            }
            else{
                map[players[id].position_x][players[id].position_y] = dropped_treasure;
            }
            players[id].deaths +=1;
            players[id].place_on_map = air;
            players[id].carried_coins = 0;

            players[id].position_x = players[id].start_x;
            players[id].position_y = players[id].start_y;
            map[players[id].position_x][players[id].position_y] = get_player_object(id);
            return;
        case player_1:
        case player_2:
        case player_3:
        case player_4:

            for (int i = 0; i < 4; ++i) {
                if(players[i].free_slot == 1){
                    if(new_place==player_1){
                        id_of_sec_player = 0;
                    }
                    if(new_place==player_2){
                        id_of_sec_player = 1;
                    }
                    if(new_place==player_3){
                        id_of_sec_player = 2;
                    }
                    if(new_place==player_4){
                        id_of_sec_player = 3;
                    }
                }
            }

            for (int i = 0; i < 100; ++i) {
                if(droppedTreasure[i].dropped_coins == -1){
                    droppedTreasure[i].position_x = players[id_of_sec_player].position_x;
                    droppedTreasure[i].position_y = players[id_of_sec_player].position_y;
                    droppedTreasure[i].dropped_coins = players[id].carried_coins+players[id_of_sec_player].carried_coins;
                }
            }
            players[id].deaths +=1;
            players[id].place_on_map = air;
            players[id].carried_coins = 0;
            players[id].position_x = players[id].start_x;
            players[id].position_y = players[id].start_y;

            players[id_of_sec_player].deaths +=1;
            players[id_of_sec_player].place_on_map = air;
            players[id_of_sec_player].carried_coins = 0;
            map[players[id_of_sec_player].position_x][players[id_of_sec_player].position_y] = dropped_treasure;
            players[id_of_sec_player].position_x = players[id_of_sec_player].start_x;
            players[id_of_sec_player].position_y = players[id_of_sec_player].start_y;

            map[players[id].position_x][players[id].position_y] = get_player_object(id);
            map[players[id_of_sec_player].position_x][players[id_of_sec_player].position_y] = get_player_object(id_of_sec_player);
            return;
    }

    switch (direction) {
        case up:
            players[id].position_x -=1;
            break;
        case down:
            players[id].position_x +=1;
            break;
        case right:
            players[id].position_y +=1;
            break;
        case left:
            players[id].position_y -=1;
            break;
    }
    if(id == 0){
        map[players[id].position_x][players[id].position_y] = player_1;
    }
    else if(id == 1){
        map[players[id].position_x][players[id].position_y] = player_2;
    }
    else if(id == 2){
        map[players[id].position_x][players[id].position_y] = player_3;
    }
    else if(id == 3){
        map[players[id].position_x][players[id].position_y] = player_4;
    }

}
void send_info_to_client(int id) {
    struct info_for_player infoForPlayer;
    infoForPlayer.position_x = players[id].position_x;
    infoForPlayer.position_y = players[id].position_y;
    infoForPlayer.deaths = players[id].deaths;
    infoForPlayer.carried_coins = players[id].carried_coins;
    infoForPlayer.saved_coins = players[id].saved_coins;

    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < 5; ++j) {
            infoForPlayer.show_map[i][j] = air;
        }
    }

    int i = players[id].position_x-2;
    int j = players[id].position_y-2;

    if(i < 0){
        i=0;
    }
    if(j<0){
        j=0;
    }

    int position_down = players[id].position_x+2;
    int position_right = players[id].position_y+2;

    if(position_down>map_height-1){
        position_down = map_height-1;
    }
    if(position_right>map_width-1){
        position_right = map_width-1;
    }


    int j_copy =j;
    for (int x = 0; i <= position_down; ++i,++x) {
        j = j_copy;
        for (int y=0; j <= position_right; ++j, ++y) {
            infoForPlayer.show_map[x][y]=map[i][j];
        }
    }
    infoForPlayer.player_number = id;
    infoForPlayer.round_number = rounds;
    int send_return;
    send_return = send(players[id].socket,&infoForPlayer,sizeof (infoForPlayer),0);
    if(send_return == -1 || send_return == 0){
        players[id].free_slot = 0;
    }
}

void print_stats(){
    mvprintw(1,60,"Campsite X/Y: 23/11");
    mvprintw(2,60,"Round number: %d",rounds);
    mvprintw(5,60,"Parameter:    Player1  Player2  Player3  Player4");
    mvprintw(6,60,"Curr X/Y:");
    int y=74;
    for (int i = 0; i < 4; ++i) {

        if(players[i].free_slot == 1){
            mvprintw(6,y,"%02d/%02d",players[i].position_x,players[i].position_y);
            mvprintw(7,y,"%02d",players[i].deaths);

            mvprintw(9,y,"%02d",players[i].carried_coins);
            mvprintw(10,y,"%02d",players[i].saved_coins);

        }
        else{
            mvprintw(6,y,"--/--");
            mvprintw(7,y,"-");
            mvprintw(9,y,"-");
            mvprintw(10,y,"-");
        }
        y+=10;

    }
    mvprintw(7,60,"Deaths:");
    mvprintw(8,60,"Coins:");
    mvprintw(9,60," Carried:");
    mvprintw(10,60," Brought:");

    mvprintw(20,60,"Legend:");
    mvprintw(21,60,"1234 - players");
    mvprintw(22,60,"█    - wall");
    mvprintw(23,60,"#    - bushes (slow down)");
    mvprintw(24,60,"*    - enemy");
    mvprintw(25,60,"c    - one coin");
    mvprintw(26,60,"D    – dropped treasure ");
    mvprintw(27,60,"C    - treasure (10 coins)");
    mvprintw(28,60,"T    - large treasure (50 coins)");
    mvprintw(29,60, "A    - campsite");


}








