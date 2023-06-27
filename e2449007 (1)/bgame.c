#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "logging.h"
#include "message.h"
#include <sys/poll.h>
#include <sys/select.h>

typedef struct obstacle {
    coordinate position;
    int durability;
} obstacle;

typedef struct bomber {
    pid_t pid;
    coordinate position;
    int arg_count;
    char **args;
    int dead_or_alive; //dead -1
} bomber;

typedef struct bomb {
    coordinate position;
    unsigned int radius;
    long interval;
    char **args;
    pid_t pid;
    int dead_or_alive; //dead -1
} bomb;

int current_bomb_count=0;

void pipe_and_fork_bomb(bomb *bombs, long interval, struct pollfd *pollfds_bomb, int bomb_sockets[][2], int index){
    //printf("pipe_and_fork_bomb OKAY \n");
    char * interval_str;
    interval_str = malloc(sizeof(char) * 20);
    sprintf(interval_str, "%ld", interval);
    //printf("%s\n", interval_str);

    //printf("LOOP OKAY \n");
    pid_t pid;
    if(PIPE(bomb_sockets[index]) == -1) perror("pipe bomb error\n");
    //else printf("PIPE BOMB OKAY\n");

    //printf("SOCKETS helper %d, %d\n",bomb_sockets[index][0],bomb_sockets[index][1]);

    pollfds_bomb[index].fd = bomb_sockets[index][0];
    pollfds_bomb[index].events = POLLIN;
    pollfds_bomb[index].revents = 0;

    pid = fork();

    if(pid < 0){
        perror("ERROR : Cannot fork \n");
    }

    else if(pid == 0){//CHILD
        //printf("CHILD \n");
        char * bomb_args[] = {"./bomb", interval_str, NULL};  //BOMB OLCAK

        close(bomb_sockets[index][0]);
        //printf("BEFORE DUP \n");
        //printf("dup2 ret %d\n", dup2(bomb_sockets[index][1], STDOUT_FILENO));

        dup2(bomb_sockets[index][1], STDOUT_FILENO);
        dup2(bomb_sockets[index][1], STDIN_FILENO);
        //printf("AFTER DUP \n");

        // bomb_args = bombs[i].args;
        //printf("EXEC BEFORE \n");
        if (execv(bomb_args[0], bomb_args) == -1) {
            perror("execv failed bomb");
            exit(1);
        }
        //else{printf("EXEC OKAY \n");}
    } 

    else{ //PARENT
        //printf("PARENT \n");
        close(bomb_sockets[index][1]);
        bombs[index].pid = pid;
    }

    //printf("SOCKETS helper end %d, %d\n",bomb_sockets[index][0],bomb_sockets[index][1]);

    //printf("pipe_and_fork_bomb RETURNS \n");
}


int main(){
    //all variables are here,,
    int map_width, map_height, obstacle_count, bomber_count;
    int * map;
    int i, j;
    char **bomber_args;
    int bombers_left;
    struct pollfd *pollfds ;
    bomb *bombs;
    struct pollfd *pollfds_bomb ;
    int winner_pid=-2;
    int winner_index=-2;
    int stat;
    int exploded_bombs=0;
    //all variables are here,,


    scanf("%d %d %d %d", &map_width, &map_height, &obstacle_count, &bomber_count);

    //map = malloc(map_height*map_width*sizeof(int));
    int map_size = map_height*map_width;
    int obstacle_left = obstacle_count;
    bombs = malloc(map_size*sizeof(bomb*));
    obstacle obstacles[obstacle_count];
    bomber bombers[bomber_count];
    int bomber_sockets[bomber_count][2];
    int bomb_sockets[map_size][2];
    pollfds=malloc(bomber_count*sizeof(struct pollfd)*2);
    pollfds_bomb = malloc(map_size*sizeof(struct pollfd)*2);


    // input obstacles
    for (int i = 0; i < obstacle_count; i++) {
        scanf("%d %d %d", &obstacles[i].position.x, &obstacles[i].position.y, &obstacles[i].durability);
    }

    // input bombers
    for (int i = 0; i < bomber_count; i++) {
        scanf("%d %d %d", &bombers[i].position.x, &bombers[i].position.y, &bombers[i].arg_count);
        bombers[i].args = (char**)malloc(bombers[i].arg_count * sizeof(char *));
        for (int j = 0; j < bombers[i].arg_count; j++) {
            bombers[i].args[j] = (char*)malloc(1024 * sizeof(char));
            scanf("%s", bombers[i].args[j]);
        }
    }

    // Create bidirectional pipe, fork, execv

    for (int i = 0; i < bomber_count; i++) {

        pid_t pid;
        PIPE(bomber_sockets[i]);    // should be PIPE to make BIDIRECTIONAL
        //printf("%d %d\n",bomber_sockets[i][0], bomber_sockets[i][1]);
        //printf("SOCKETS BOMBER %d %d \n",bomber_sockets[i][0], bomber_sockets[i][1]);
        pollfds[i].fd = bomber_sockets[i][0];
        pollfds[i].events = POLLIN;
        pollfds[i].revents = 0;

        pid = fork();

        if(pid < 0){
            perror("ERROR : Cannot fork \n");
            continue;
        }

        else if(pid == 0){//CHILD
            //printf("child created \n");
           /* pollfds[i].fd = bomber_sockets[i][0];
            pollfds[i].events = POLLIN;
            pollfds[i].revents = 0;*/
            close(bomber_sockets[i][0]);

            dup2(bomber_sockets[i][1], STDOUT_FILENO);
            dup2(bomber_sockets[i][1], STDIN_FILENO);
            
            
            bombers[i].args[bombers[i].arg_count] = NULL; //DEGISEBILIR
            bomber_args = bombers[i].args;
            execv(bomber_args[0], bomber_args);
            perror("ERROR : Cannot execv \n");
        } 

        else{ //PARENT
            /*pollfds[i].fd = bomber_sockets[i][0];
            pollfds[i].events = POLLIN;
            pollfds[i].revents = 0;*/
            //waitpid(pid);
            close(bomber_sockets[i][1]);
            bombers[i].pid = pid;
            //printf("parenttayim \n");
        }
    }

    // check bombers left 
    // controller loop


    bombers_left = bomber_count;


    while(bombers_left>1){
               // select/poll on socket file descriptors of bombs
               // for all file descriptors ready (have data)
               //     read request
               //     serve request

        poll(pollfds, bomber_count, 0);
        poll(pollfds_bomb, current_bomb_count, 0);

        for(int j=0; j<current_bomb_count;j++){
            im message;
            if(pollfds_bomb[j].revents & POLLIN){
                if (read_data(bomb_sockets[j][0], &message) <= 0 || 
                    bombs[j].pid == -1) {
                    continue; // No data available from the bomber
                }
                if(bombs[j].dead_or_alive==-1) continue;

                if(message.type == BOMB_EXPLODE){
                    imp incoming_message_print;
                    omd outgoing_data;
                    om outgoing_message;
                    omp outgoing_message_print;

                    unsigned int radius = bombs[j].radius;
                    long interval = bombs[j].interval;

                    int x = bombs[j].position.x;
                    int y = bombs[j].position.y;

                    int x_up = x+radius;
                    int x_down = x-radius;
                    int y_up = y+radius;
                    int y_down = y-radius;
                    int bomber_index=-1;

                    int died_index=-1;
                    int flag0 = 1; //clear
                    int flag1 = 1; //clear
                    int flag2 = 1; //clear
                    int flag3 = 1; //clear

                    int first_died_index=-2;
                    int flag_someone_already_died=0;
                    

                    incoming_message_print.pid = bombs[j].pid;
                    incoming_message_print.m = &message;
                    print_output(&incoming_message_print, NULL, NULL, NULL);

                    bombs[j].dead_or_alive=-1; //explode yani
                    //exploded_bombs+=1;

                    //printf("RADIUS %d\n",radius);
                    //printf("interval %d\n",interval);
                    //printf("X Y %d %d\n",x,y);
                    
                    for (int i = 0; i < bomber_count; i++) {
                        if(bombers[i].pid==-1 || bombers[i].dead_or_alive==-1)continue;
                        if (bombers[i].position.x == bombs[j].position.x && bombers[i].position.y == bombs[j].position.y) {
                            //die
                            //outgoing_message_print.pid=bombers[i].pid;
                            bombers[i].dead_or_alive = -1; //died yani 
                            bombers_left -= 1;
                            //outgoing_message.type = BOMBER_DIE;
                            //outgoing_message_print.m = &outgoing_message;
                            //send_message(bomber_sockets[i][0], &outgoing_message); //????
                            //print_output(NULL, &outgoing_message_print, NULL, NULL);
                            //waitpid(bombers[i].pid,&stat,0);
                        }
                    }
                    if(bombers_left==1){
                        for(int i=0; i<bomber_count; i++){
                            if(bombers[i].dead_or_alive!=-1){
                                winner_pid = bombers[i].pid;
                                winner_index=i;
                            }
                        }
                        break;
                    }

                    for (int direction = 0; direction < 4; direction++) {
                        for (int step = 1; step <= radius; step++) {
                            if(direction==0 && flag0==0) continue;
                            if(direction==1 && flag1==0) continue;
                            if(direction==2 && flag2==0) continue;
                            if(direction==3 && flag3==0) continue;

                            coordinate current_position = bombs[j].position;
                            
                            if(direction==0){current_position.x += step; } // Right
                            if(direction==1) {current_position.y += step;} // Up
                            if(direction==2){current_position.x -= step; } // Left
                            if(direction==3) {current_position.y -= step;} // Down

                            // Check for obstacles
                            
                            for (int i = 0; i < obstacle_count; i++) {
                                if (current_position.x == obstacles[i].position.x && current_position.y == obstacles[i].position.y && obstacles[i].durability>0) {
                                    obsd obstacle_type;
                                    obstacle_type.position = obstacles[i].position;
                                    obstacle_type.remaining_durability =obstacles[i].durability-1;
                                    if(direction==0){
                                        if(flag0==0) break;
                                        flag0=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;
                                        
                                    }
                                    else if(direction==1){
                                        if(flag1==0) break;
                                        flag1=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;
                                    }
                                    else if(direction==2){
                                        if(flag2==0) break;
                                        flag2=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;
                                    
                                    }
                                    else if(direction==3){
                                        if(flag3==0) break;
                                        flag3=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;                                      
                                    }
                                } 
                            }

                            for(int i=0; i<bomber_count; i++){
                                if (current_position.x == bombers[i].position.x && current_position.y == bombers[i].position.y && bombers[i].pid != -1 && bombers[i].dead_or_alive!=-1) {
                                    if(direction==0 && flag0==0) break;
                                    if(direction==1 && flag1==0) break;
                                    if(direction==2 && flag2==0) break;
                                    if(direction==3 && flag3==0) break;
                                    if(flag_someone_already_died==0){
                                        first_died_index = i;
                                        flag_someone_already_died=1;
                                    }
                                    if(bombers_left==1){
                                        for(int i=0; i<bomber_count; i++){
                                            if(bombers[i].dead_or_alive!=-1){
                                                winner_pid = bombers[i].pid;
                                                winner_index=i;
                                            }
                                        }
                                        break;
                                }
                                    //outgoing_message_print.pid=bombers[i].pid;
                                    bombers[i].dead_or_alive = -1; //died yani 
                                    bombers_left -= 1;
                                    //outgoing_message.type = BOMBER_DIE;
                                    //outgoing_message_print.m = &outgoing_message;
                                    //send_message(bomber_sockets[i][0], &outgoing_message); //????
                                    //print_output(NULL, &outgoing_message_print, NULL, NULL);
                                    //waitpid(bombers[i].pid,&stat,0);
                                    break;
                                }
                            }
                            if(bombers_left==1){
                                        for(int i=0; i<bomber_count; i++){
                                            if(bombers[i].dead_or_alive!=-1){
                                                winner_pid = bombers[i].pid;
                                                winner_index=i;
                                            }
                                        }
                                        break;
                                }

                        }       

                    }
                    if(bombers_left==1){
                        for(int i=0; i<bomber_count; i++){
                            if(bombers[i].pid!=-1){
                                winner_pid = bombers[i].pid;
                                winner_index=i;
                            }
                        }
                        break;
                    }

                    waitpid(bombs[j].pid,&stat,0);
                }
                
            }
        }

        for (int i = 0; i < bomber_count; i++) {
            im message;
            if(bombers_left==1&&bombers[i].pid==winner_pid){
                read_data(bomber_sockets[i][0], &message);
                if(message.type==BOMBER_SEE){
                        om outgoing_message;
                        omp outgoing_message_print;
                        imp in_message_print;
                        in_message_print.pid = bombers[i].pid;
                        in_message_print.m = &message;
                        outgoing_message.type = BOMBER_WIN;
                        outgoing_message_print.m = &outgoing_message;
                        outgoing_message_print.pid = winner_pid;
                        print_output(&in_message_print, NULL, NULL, NULL);
                        print_output(NULL, &outgoing_message_print, NULL, NULL);
                        send_message(bomber_sockets[i][0], &outgoing_message);
                }
                if(message.type==BOMBER_MOVE){
                    imp incoming_message_print;
                    imd incoming_message_data;
                    om outgoing_message;
                    omp outgoing_message_print;
                    incoming_message_print.pid = bombers[i].pid;
                    incoming_message_print.m = &message;
                    incoming_message_data = message.data;
                    outgoing_message.type = BOMBER_WIN;
                    outgoing_message_print.m = &outgoing_message;
                    outgoing_message_print.pid = winner_pid;
                    print_output(&incoming_message_print, NULL, NULL, NULL);
                    print_output(NULL, &outgoing_message_print, NULL, NULL);
                    send_message(bomber_sockets[i][0], &outgoing_message);
                }
                if(message.type==BOMBER_PLANT){
                    imp incoming_message_print;
                    omp outgoing_message_print;
                    om outgoing_message;

                    incoming_message_print.pid = bombers[i].pid;
                    incoming_message_print.m = &message;

                    print_output(&incoming_message_print, NULL, NULL, NULL);

                    outgoing_message.type = BOMBER_WIN;
                    outgoing_message_print.m = &outgoing_message;
                    outgoing_message_print.pid = winner_pid;
                    send_message(bomber_sockets[i][0], &outgoing_message);
                    print_output(NULL, &outgoing_message_print, NULL, NULL);

                }
            }
            else if(bombers[i].dead_or_alive==-1){
                read_data(bomber_sockets[i][0], &message);
                if(message.type==BOMBER_SEE){
                        om outgoing_message;
                        omp outgoing_message_print;
                        imp in_message_print;
                        in_message_print.pid = bombers[i].pid;
                        in_message_print.m = &message;
                        outgoing_message.type = BOMBER_DIE;
                        outgoing_message_print.m = &outgoing_message;
                        outgoing_message_print.pid=bombers[i].pid;
                        print_output(&in_message_print, NULL, NULL, NULL);
                        print_output(NULL, &outgoing_message_print, NULL, NULL);
                        send_message(bomber_sockets[i][0], &outgoing_message); //????
                        waitpid(bombers[i].pid,&stat,0);
                }
                if(message.type==BOMBER_MOVE){
                    imp incoming_message_print;
                    imd incoming_message_data;
                    om outgoing_message;
                    omp outgoing_message_print;
                    incoming_message_print.pid = bombers[i].pid;
                    incoming_message_print.m = &message;
                    incoming_message_data = message.data;
                    outgoing_message.type = BOMBER_DIE;
                    outgoing_message_print.m = &outgoing_message;
                    outgoing_message_print.pid=bombers[i].pid;
                    print_output(&incoming_message_print, NULL, NULL, NULL);
                    print_output(NULL, &outgoing_message_print, NULL, NULL);
                    send_message(bomber_sockets[i][0], &outgoing_message);
                    waitpid(bombers[i].pid,&stat,0);
                }
                if(message.type==BOMBER_PLANT){
                    imp incoming_message_print;
                    omp outgoing_message_print;
                    om outgoing_message;

                    incoming_message_print.pid = bombers[i].pid;
                    incoming_message_print.m = &message;

                    print_output(&incoming_message_print, NULL, NULL, NULL);

                    outgoing_message.type = BOMBER_DIE;
                    outgoing_message_print.m = &outgoing_message;
                    outgoing_message_print.pid = bombers[i].pid;
                    send_message(bomber_sockets[i][0], &outgoing_message);
                    print_output(NULL, &outgoing_message_print, NULL, NULL);
                    waitpid(bombers[i].pid,&stat,0);

                }
            }

            if(pollfds[i].revents & POLLIN){
                if(bombers[i].dead_or_alive==-1) continue;
                
                if (read_data(bomber_sockets[i][0], &message) <= 0 || 
                     bombers[i].pid == -1) {
                     continue; // No data available from the bomber
                }

                //read_data(bomber_sockets[i][0], &message);
                if(message.type == BOMBER_START){
                    
                    int r;
                    imp incoming_message_print;
                    omd outgoing_data = {.new_position = bombers[i].position};
                    // printf("out data: %d\n", outgoing_data.new_position.x);
                    om outgoing_message = {BOMBER_LOCATION, outgoing_data};
                    omp outgoing_message_print = {bombers[i].pid, &outgoing_message};
                    incoming_message_print.pid = bombers[i].pid;
                    incoming_message_print.m = &message;
                    print_output(&incoming_message_print, NULL, NULL, NULL);

                    bombers[i].dead_or_alive=0;

                    outgoing_message_print.pid = bombers[i].pid;
                    outgoing_message_print.m = &outgoing_message;

                    
                    r = send_message(bomber_sockets[i][0], &outgoing_message);
                    //printf("%d\n",r);
                    print_output(NULL, &outgoing_message_print, NULL, NULL);
                    //continue;

                }
                
                if(message.type==BOMBER_SEE){
                    int r;
    
                    om outgoing_message;
                    imp in_message_print;
                    omp out_message_print;
                    od visible_objects[25];
                    int object_count = 0;
                    int flag0 = 1; //clear
                    int flag1 = 1; //clear
                    int flag2 = 1; //clear
                    int flag3 = 1; //clear
                    in_message_print.pid = bombers[i].pid;
                    in_message_print.m = &message;
                    print_output(&in_message_print, NULL, NULL, NULL);

                    for (int j = 0; j < current_bomb_count; j++) {
                        if(bombs[j].pid==-1)continue;
                        if(bombers[i].dead_or_alive==-1)continue;
                        if (bombers[i].position.x == bombs[j].position.x && bombers[i].position.y == bombs[j].position.y) {
                            visible_objects[object_count].position = bombs[j].position;
                            visible_objects[object_count].type = BOMB;
                            object_count++;
                            break;
                        }
                    }


                    for (int direction = 0; direction < 4; direction++) {
                        for (int step = 1; step <= 3; step++) {
                            if(direction==0 && flag0==0) continue;
                            if(direction==1 && flag1==0) continue;
                            if(direction==2 && flag2==0) continue;
                            if(direction==3 && flag3==0) continue;

                            coordinate current_position = bombers[i].position;
                            
                            if(direction==0){current_position.x += step; } // Right
                            if(direction==1) {current_position.y += step;} // Up
                            if(direction==2){current_position.x -= step; } // Left
                            if(direction==3) {current_position.y -= step;} // Down

                            // Check for obstacles
                            
                            for (int j = 0; j < obstacle_count; j++) {
                                if (current_position.x == obstacles[j].position.x && current_position.y == obstacles[j].position.y && obstacles[j].durability>0) {
                                    if(direction==0){
                                        if(flag0==0) break;
                                        flag0=0;
                                        visible_objects[object_count].position = obstacles[j].position;
                                        visible_objects[object_count].type = OBSTACLE;
                                        object_count++;
                                        break;
                                        
                                    }
                                    else if(direction==1){
                                        if(flag1==0) break;
                                        flag1=0;
                                        visible_objects[object_count].position = obstacles[j].position;
                                        visible_objects[object_count].type = OBSTACLE;
                                        object_count++;
                                        break;
                                    }
                                    else if(direction==2){
                                        if(flag2==0) break;
                                        flag2=0;
                                        visible_objects[object_count].position = obstacles[j].position;
                                        visible_objects[object_count].type = OBSTACLE;
                                        object_count++;
                                        break;
                                    
                                    }
                                    else if(direction==3){
                                        if(flag3==0) break;
                                        flag3=0;
                                        visible_objects[object_count].position = obstacles[j].position;
                                        visible_objects[object_count].type = OBSTACLE;
                                        object_count++;
                                        break;                                      
                                    }
                                }  // BU DIGER GORUSLERI DE ETKILEYECEK + BOMB EKLE
                            }

                            for(int j=0; j<bomber_count; j++){
                                if (bombers[i].pid!=bombers[j].pid && current_position.x == bombers[j].position.x && current_position.y == bombers[j].position.y && bombers[j].dead_or_alive != -1) {
                                    if(direction==0 && flag0==0) break;
                                    if(direction==1 && flag1==0) break;
                                    if(direction==2 && flag2==0) break;
                                    if(direction==3 && flag3==0) break;
                                    visible_objects[object_count].position = bombers[j].position;
                                    visible_objects[object_count].type = BOMBER;
                                    object_count++;
                                    break;
                                }
                            }
                            
                            for (int j = 0; j < current_bomb_count; j++) {
                                if (current_position.x == bombs[j].position.x && current_position.y == bombs[j].position.y && bombs[j].dead_or_alive != -1) {
                                    if(direction==0 && flag0==0) break;
                                    if(direction==1 && flag1==0) break;
                                    if(direction==2 && flag2==0) break;
                                    if(direction==3 && flag3==0) break;
                                    visible_objects[object_count].position = bombs[j].position;
                                    visible_objects[object_count].type = BOMB;
                                    object_count++;
                                    break;
                                }
                            }

                        }       

                    }
                    
                    outgoing_message.type = BOMBER_VISION;
                    outgoing_message.data.object_count = object_count;
                    out_message_print.pid = bombers[i].pid;
                    out_message_print.m = &outgoing_message;
                    r = send_message(bomber_sockets[i][0], &outgoing_message);
                   // printf("%d\n",r);

                    send_object_data(bomber_sockets[i][0], object_count, visible_objects); 
                    print_output(NULL, &out_message_print, NULL, visible_objects);

                    // N OBJEYI PRINTLICEN
                }  

                if(message.type==BOMBER_MOVE){
                    imp incoming_message_print;
                    imd incoming_message_data;
                    incoming_message_print.pid = bombers[i].pid;
                    incoming_message_print.m = &message;
                    incoming_message_data = message.data;
                    print_output(&incoming_message_print, NULL, NULL, NULL);

                    coordinate target_position = incoming_message_data.target_position;
                    coordinate current_position = bombers[i].position;

                    // Check if target position is one step away from current position
                    if (!((abs(target_position.x - current_position.x) == 1 && target_position.y == current_position.y) ||
                        (abs(target_position.y - current_position.y) == 1 && target_position.x == current_position.x))) {
                        // Target position is not one step away, send old location back
                        om move_response;
                        move_response.type = BOMBER_LOCATION;
                        move_response.data.new_position = bombers[i].position;
                        send_message(bomber_sockets[i][0], &move_response);
                    } 
                    else if (target_position.x == current_position.x || target_position.y == current_position.y) {
                        // Check if move is horizontal or vertical
                        int blocked = 0;

                        // Check for obstacles or other bombers in the target position
                        for (int j = 0; j < bomber_count; j++) {
                            if (j != i && bombers[j].position.x == target_position.x && bombers[j].position.y == target_position.y) {
                                blocked = 1;
                                break;
                            }
                        }

                        for (int j = 0; j < obstacle_count; j++) {
                            if (obstacles[j].position.x == target_position.x && obstacles[j].position.y == target_position.y) {
                                blocked = 1;
                                break;
                            }
                        }

                        for (int j = 0; j < current_bomb_count; j++) {
                            if (bombs[j].position.x == target_position.x && bombs[j].position.y == target_position.y) {
                                blocked = 1;
                                break;
                            }
                        }

                        if (blocked) {
                            // Target position is blocked, send old location back
                            om move_response;
                            move_response.type = BOMBER_LOCATION;
                            move_response.data.new_position = bombers[i].position;
                            send_message(bomber_sockets[i][0], &move_response);
                        } 
                        else {
                            // Move is valid, update bomber position
                            bombers[i].position = target_position;

                            // Send BOMBER_LOCATION message with new position
                            om move_response;
                            move_response.type = BOMBER_LOCATION;
                            move_response.data.new_position = bombers[i].position;
                            send_message(bomber_sockets[i][0], &move_response);

                            // Log outgoing message
                            omp outgoing_message_print;
                            outgoing_message_print.pid = bombers[i].pid;
                            outgoing_message_print.m = &move_response;
                            print_output(NULL, &outgoing_message_print, NULL, NULL);
                        }
                    } 
                    
                    else {
                        // Diagonal move is not accepted, send old location back
                        om move_response;
                        move_response.type = BOMBER_LOCATION;
                        move_response.data.new_position = bombers[i].position;
                        send_message(bomber_sockets[i][0], &move_response);
                    }
                }

                if(message.type==BOMBER_PLANT) {
                    imp incoming_message_print;
                    long interval;
                    unsigned int radius;
                    unsigned int x;
                    unsigned int y;
                    int flag_bomb = 1;
                    om plant_response;
                    omp outgoing_message_print;
                    om outgoing_message;

                    incoming_message_print.pid = bombers[i].pid;
                    incoming_message_print.m = &message;

                    print_output(&incoming_message_print, NULL, NULL, NULL);

                    interval = message.data.bomb_info.interval;   // EXECUTABLE GONDER
                    radius = message.data.bomb_info.radius;

                    x = bombers[i].position.x;
                    y = bombers[i].position.y;

                    for(int i=0; i<current_bomb_count;i++) {
                        if(bombs[i].position.x == x && bombs[i].position.y == y){
                            flag_bomb = 0;
                            plant_response.type = BOMBER_PLANT_RESULT;
                            plant_response.data.planted = 0;
                            break;
                        }
                    }

                    if(flag_bomb == 1){
                        char * path= "./bomb";
                        //char * bomb_args[] = {"./bomb_inek", interval, NULL};
                        //execv(path, bomb_args);
                        int index = current_bomb_count;

                        bombs[current_bomb_count].interval = interval;    
                        bombs[current_bomb_count].radius = radius;   
                        bombs[current_bomb_count].position.x = x;
                        bombs[current_bomb_count].position.y = y;  
                        bombs[current_bomb_count].dead_or_alive=0;
                        current_bomb_count++;   
                        plant_response.type = BOMBER_PLANT_RESULT;
                        plant_response.data.planted = 1;

                        //printf("pipe içine giriyor\n");

                        pipe_and_fork_bomb(bombs, interval, pollfds_bomb, bomb_sockets, index);
                        //printf("SOCKETS %d %d \n", bomb_sockets[i][0], bomb_sockets[i][1]);
                        
                    }
                    send_message(bomber_sockets[i][0], &plant_response);
                    outgoing_message_print.m = &plant_response;
                    print_output(NULL, &outgoing_message_print, NULL, NULL);
                    
                }

            }

        }
        usleep(1000);
    }


    for(int j=0; j<current_bomb_count;j++){
        if(bombs[j].dead_or_alive==-1){
            exploded_bombs+=1;
            continue;
        } 
    }


    while(1){
        poll(pollfds_bomb, current_bomb_count, 0);
        for(int j=0; j<current_bomb_count;j++){
            im message;
            if(pollfds_bomb[j].revents & POLLIN){
                if (read_data(bomb_sockets[j][0], &message) <= 0 || 
                    bombs[j].pid == -1 && bombs[j].dead_or_alive==-1) {
                    continue; // No data available from the bomber
                }

                if(message.type == BOMB_EXPLODE){
                    imp incoming_message_print;
                    omd outgoing_data;
                    om outgoing_message;
                    omp outgoing_message_print;

                    unsigned int radius = bombs[j].radius;
                    long interval = bombs[j].interval;

                    int x = bombs[j].position.x;
                    int y = bombs[j].position.y;

                    int x_up = x+radius;
                    int x_down = x-radius;
                    int y_up = y+radius;
                    int y_down = y-radius;
                    int bomber_index=-1;

                    int died_index=-1;
                    int flag0 = 1; //clear
                    int flag1 = 1; //clear
                    int flag2 = 1; //clear
                    int flag3 = 1; //clear

                    int first_died_index=-2;
                    int flag_someone_already_died=0;

                    incoming_message_print.pid = bombs[j].pid;
                    incoming_message_print.m = &message;
                    print_output(&incoming_message_print, NULL, NULL, NULL);

                    bombs[j].dead_or_alive=-1; //explode yani
                    exploded_bombs+=1;
                    
                    for (int direction = 0; direction < 4; direction++) {
                        for (int step = 1; step <= radius; step++) {
                            if(direction==0 && flag0==0) continue;
                            if(direction==1 && flag1==0) continue;
                            if(direction==2 && flag2==0) continue;
                            if(direction==3 && flag3==0) continue;

                            coordinate current_position = bombs[j].position;
                            
                            if(direction==0){current_position.x += step; } // Right
                            if(direction==1) {current_position.y += step;} // Up
                            if(direction==2){current_position.x -= step; } // Left
                            if(direction==3) {current_position.y -= step;} // Down

                            // Check for obstacles
                            
                            for (int i = 0; i < obstacle_count; i++) {
                                if (current_position.x == obstacles[i].position.x && current_position.y == obstacles[i].position.y && obstacles[i].durability>0) {
                                    obsd obstacle_type;
                                    obstacle_type.position = obstacles[i].position;
                                    obstacle_type.remaining_durability =obstacles[i].durability-1;
                                    if(direction==0){
                                        if(flag0==0) break;
                                        flag0=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;
                                        
                                    }
                                    else if(direction==1){
                                        if(flag1==0) break;
                                        flag1=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;
                                    }
                                    else if(direction==2){
                                        if(flag2==0) break;
                                        flag2=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;
                                    
                                    }
                                    else if(direction==3){
                                        if(flag3==0) break;
                                        flag3=0;
                                        obstacles[i].durability-=1;
                                        print_output(NULL,NULL,&obstacle_type,NULL);
                                        break;                                      
                                    }
                                } 
                            }

                        }       

                    }

                    waitpid(bombs[j].pid,&stat,0);
                }
                
            }
        }
        if(exploded_bombs>=current_bomb_count) break;
        //printf("hfjdhfs\n");
    }


    for (int i=0; i<bomber_count; i++){
        waitpid(bombers[i].pid, &stat, 0);
    }


    exit(1);
    // reap children

}
