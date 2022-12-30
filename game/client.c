#include <unistd.h>
#include <locale.h>
#include <stdlib.h>
#include <pthread.h>
#include "client.h"
#include "game.h"

int sockfd;

int main(){
    setlocale(LC_ALL, "");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd == -1){
        printf("creating socket err");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int connection = connect(sockfd,(struct sockaddr*)&server_addr,sizeof(server_addr));

    if(connection ==-1){
        printf("connecting to server err");
        return 1;
    }

    int info_from_server = -1; //server pełny = 0, pusty =1
    int response = recv(sockfd,&info_from_server,sizeof(info_from_server),0);

    if(response ==-1){
        printf("response err");
        return 1;
    }

    if(info_from_server ==0){
        printf("Server is full. Can not join game.");
        close(sockfd);
        return 1;
    }

    initscr();
    noecho();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    clear();


    pthread_t pthread_1;
    pthread_create(&pthread_1, NULL, client_view, NULL);

    keyboard(sockfd);
    endwin();
    return 0;
}

void keyboard(int sockfd){

    static int up = 1;
    static int down = 2;
    static int right = 3;
    static int left = 4;
    static int quit = 5;

    for (;;) {
        int input = getch();
        switch(input){

            case KEY_UP:
                send(sockfd,&up,sizeof (up),0);
                break;
            case KEY_DOWN:
                send(sockfd,&down,sizeof (down),0);
                break;
            case KEY_RIGHT:
                send(sockfd,&right,sizeof (right),0);
                break;
            case KEY_LEFT:
                send(sockfd,&left,sizeof (down),0);
                break;
            case 'q':
            case 'Q':
                send(sockfd,&quit,sizeof (quit),0);
                endwin();
                exit(0);
                break;
            default:
                break;
        }
        //clear();
        //refresh();
    }
}

void* client_view(){
    for(;;){
        struct info_for_player infoForPlayer;
        recv(sockfd,&infoForPlayer,sizeof (infoForPlayer),0);
        clear();
        for (int i = 0; i < 5; ++i) {
            for (int j = 0; j < 5; ++j) {

                int x = infoForPlayer.position_x-2;
                int y = infoForPlayer.position_y-2;

                if(x<0){
                    x=0;
                }
                if(y<0){
                    y=0;
                }

                switch(infoForPlayer.show_map[i][j]){
                    case wall:
                        mvprintw(x+i,y+j,"█");
                        break;
                    case bush:
                        mvprintw(x+i,y+j,"#");
                        break;
                    case wild_beast:
                        mvprintw(x+i,y+j,"*");
                        break;
                    case one_coin:
                        mvprintw(x+i,y+j,"c");
                        break;
                    case treasure:
                        mvprintw(x+i,y+j,"t");
                        break;
                    case large_treasure:
                        mvprintw(x+i,y+j,"T");
                        break;
                    case campsite:
                        mvprintw(x+i,y+j,"A");
                        break;
                    case dropped_treasure:
                        mvprintw(x+i,y+j,"D");
                        break;
                    case air:
                        mvprintw(x+i,y+j," ");
                        break;
                    case player_1:
                        mvprintw(x+i,y+j,"1");
                        break;
                    case player_2:
                        mvprintw(x+i,y+j,"2");
                        break;
                    case player_3:
                        mvprintw(x+i,y+j,"3");
                        break;
                    case player_4:
                        mvprintw(x+i,y+j,"4");
                        break;
                }
            }
        }
        print_stats(infoForPlayer);
        refresh();
    }
}
void print_stats( struct info_for_player infoForPlayer){
    mvprintw(1,60,"Campsite X/Y: 23/11");
    mvprintw(2,60,"Round number: %d",infoForPlayer.round_number);

    mvprintw(6,60,"Number: %d", infoForPlayer.player_number+1);
    mvprintw(7,60,"Type: HUMAN");
    mvprintw(8,60,"Curr X/Y: %d/%d",infoForPlayer.position_x,infoForPlayer.position_y);
    mvprintw(9,60,"Deaths: %d",infoForPlayer.deaths);
    mvprintw(12,60,"Coins found: %d",infoForPlayer.carried_coins);
    mvprintw(13,60,"Coins brought: %d",infoForPlayer.saved_coins);

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