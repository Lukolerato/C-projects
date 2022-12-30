#ifndef GAME_CLIENT_H
#define GAME_CLIENT_H
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <ncurses.h>
#include "game.h"

void keyboard(int sockfd);
void* client_view();
void print_stats(struct info_for_player infoForPlayer);

#endif //GAME_CLIENT_H
