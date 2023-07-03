#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>

#define IS_EMPTY(X) ((X) == NULL)
#define FOREVER 1

#define DEFAULT_PORT 8013
#define BUFFER_SIZE 1024
#define LOGIN_REQUEST "LOGIN"
#define GET_PLAYGROUND_REQUEST "GET PLAYGROUND"
#define FIND_PLAYER_REQUEST "FIND"
#define RESTRICT_PARAS_CHAR " " // Actually space character
#define LOGIN_STATUS_OK "LOGIN OK"
#define LOGIN_STATUS_FAILED_MEMORY_SIZE "LOGIN NKM"
#define LOGIN_STATUS_FAILED_NOT_VALID_USERNAME "LOGIN NKU"
#define PLAYER_FOUND_RESPONSE "PLAYER FOUND"
#define PLAYER_FOUND_RESPONSE_SIZE strlen(PLAYER_FOUND_RESPONSE)
#define LOGIN_OK_SIZE strlen(LOGIN_STATUS_OK)
#define LOGIN_NOT_OK_SIZE strlen(LOGIN_STATUS_FAILED_MEMORY_SIZE)
#define USERNAME_LENGTH 20
#define GROUND_SIZE 10
#define FINAL_SCORE 5

#define LOG_OK_FORMAT               "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;32m%s\n\033[0m"
#define LOG_WARNING_FORMAT          "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;33m%s\n\033[0m"
#define LOG_ERROR_FORMAT            "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;31m%s\n\033[0m"
#define LOG_INFO_FORMAT             "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;34m%s\n\033[0m"
#define LOG_DEFAULT_FORMAT          "[%d-%02d-%02d  %02d:%02d] %s\n"

#define PLAYGROUND_FORMAT           "PLAYGROUND %s%s%s %d %d %c %c \r\n"
#define PLAYGROUND_RESPONSE_SIZE ((USERNAME_LENGTH*2) + GROUND_SIZE + 30) 

typedef enum LogMessageType
{
    OK,
    WARNING,
    ERROR,
    INFO,
} log_type;

typedef enum Player_status
{
    WAITING_FOR_A_PLAYER,
    PLAYING
} player_status;

int master_socket, new_socket, valread;
unsigned int port = DEFAULT_PORT;
char buffer[1024] = {0};
struct sockaddr_in socket_address;

typedef struct users {
    int ipAddress;
    int socketAddress;
    char *username;
    player_status p_status;
    struct users* nextUser;
    struct users* prevUser;
} Users;

typedef  Users* usersPtr;

//initialize the list of users as LinkedList
usersPtr list_of_users = NULL;

typedef struct playground {
    char ground[GROUND_SIZE];
    usersPtr player_one;
    usersPtr player_two;
    char player_one_char;
    char player_two_char;
    unsigned short first_player_points;
    unsigned short secound_player_points;
    struct playground *nextPlayGround;
    struct playground *prevPlayGround;
} playGround;

typedef playGround *playGroundPtr;

//initialize the playground as LinkedList
playGroundPtr mainGround = NULL;

//create a Queue for users' requests to find a player
struct queue_of_players{
    
    usersPtr player;
    struct queue_of_players* next;
};

typedef struct queue_of_players* QueueOfPlayers;

QueueOfPlayers  front = NULL;
QueueOfPlayers  rear = NULL;

void print_welcome_message();
void setup_server();
char** requests_parser();
void manage_requests(char** request_parsed, const int *sock);
void insert_user(char *username, const int *sock);
int delete_user(const int *socket_addr);
void find_a_player(const int *socket_address);
void handle_disconnected_user(const int *socket_address);
void create_a_playground(const usersPtr player1, const usersPtr player2);
int delete_playground(const int *socket_addr);
int new_username_is_valid(char *);
void chage_port(const char *port);
void log_print(const log_type *type, const char* message, ...);
void signal_handler(int EXIT_CODE);
void perform_player_selection(const char *username_1, const unsigned short select, const char *username_2);
int check_who_is_winner(playGroundPtr pg);
void send_playground_data(const playGroundPtr pg);
int reset_playground(playGroundPtr pg);
void enqueue(usersPtr player);
void dequeue(usersPtr *player_one, usersPtr *player_two);

int main(int argc, char **argv)
{

    srand(time(NULL));

    signal(SIGINT, signal_handler);

    // Check If We Have an Argument, Change the Default Port
    if (argc > 1) chage_port(argv[1]);

    print_welcome_message();
    setup_server();
}

void print_welcome_message()
{
    puts("TIC TAC TOE  SERVER IS UP!");
    //TODO: Print the Ip Address of the server
    printf("SERVER PORT IS: %d\n", port);
    fflush(stdout);
}

//setup server
void setup_server()
{
    int opt = 1;

    // Creating socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket Initialization Failed!");
        exit(EXIT_FAILURE);
    }

    // attaching socket to the port (defualt port is 8013)
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        perror("attaching port for the socket failed!");
        exit(EXIT_FAILURE);
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(port);

    if (bind(master_socket, (struct sockaddr*)&socket_address, sizeof(socket_address)) < 0)
    {
        perror("The binding Failed!");
        exit(EXIT_FAILURE);
    }

    if (listen(master_socket, 3) < 0)
    {
        perror("listen Failed!");
        exit(EXIT_FAILURE);
    }

    fd_set readfds;
    int adderlen = sizeof(socket_address);
    usersPtr *__users = &list_of_users;;
    int max_sd;
    int activity;
    int sd = 0;

    while (FOREVER)
    {

        FD_ZERO(&readfds);

        FD_SET(master_socket, &readfds);

        max_sd = master_socket;

        FOREACH_USERS_ONE:

            if (IS_EMPTY(*__users))
                goto END_ONE;

            sd = (*__users)->socketAddress;
            
            if (sd > 0)
                FD_SET(sd, &readfds);
            
            if (sd > max_sd)
                max_sd = sd;
            
            __users = &(*__users)->nextUser;
            goto FOREACH_USERS_ONE;

        END_ONE:
            __users = &list_of_users;



        
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0))
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // if something happend on the master socket
        if (FD_ISSET(master_socket, &readfds))
        {

            if ((new_socket = accept(master_socket, (struct sockaddr *)&socket_address, (socklen_t*)&adderlen))<0)  
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }

            valread = read(new_socket, buffer, BUFFER_SIZE);

            if (valread > 0)
            {
                char **request_parsed = requests_parser();
                manage_requests(request_parsed, &new_socket);
                memset(buffer, '\0', 1024); // clear the buffer
            }
        }

        // else its some IO operation on some other socket

        FOREACH_USERS_TWO:

            if (IS_EMPTY(*__users))
                goto END_TWO;

            sd = (*__users)->socketAddress;
            
            if (FD_ISSET(sd, &readfds))
            {   
                if ((valread = read(sd, buffer, 1024)) == 0)
                {
                    handle_disconnected_user(&sd);
                    if (IS_EMPTY(*__users))
                        goto END_TWO;
                }
                else
                {
                    char **request_parsed = requests_parser();
                    manage_requests(request_parsed, &sd);
                    memset(buffer, '\0', 1024); // clear the buffer
                }
            }
            
            __users = &(*__users)->nextUser;
            goto FOREACH_USERS_TWO;
            
        END_TWO:
            __users = &list_of_users;
    }

}

/*
this function parses all of the requests
*/
char** requests_parser()
{
    char *string_slice;
    string_slice = strtok(buffer, RESTRICT_PARAS_CHAR);
    char **sliced = malloc(10 * USERNAME_LENGTH);
    int sliced_idx = 0;

    while (string_slice != NULL)
    {   
        sliced[sliced_idx] = string_slice;
        string_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
        sliced_idx++;
    }

    return sliced;

}

/*
Check the parsed request and execute the commands
*/
void manage_requests(char** request_parsed, const int *sock)
{
    if (strcmp(request_parsed[0], LOGIN_REQUEST) == 0)
    {   
        char username[USERNAME_LENGTH] = ""; 
        for (int index = 1; strcmp(request_parsed[index], "\r\n")!=0; index++)
        {   
            if (strlen(username) + strlen(request_parsed[index]) >= USERNAME_LENGTH)
                break;

            strcat(username, request_parsed[index]);
            
            if (strcmp(request_parsed[index], "\r\n")!=0)
                strcat(username, " ");
        }
        insert_user(username, sock);
        return;
    }

    if (strcmp(request_parsed[0], FIND_PLAYER_REQUEST) == 0)
    {
        find_a_player(sock);
        return;
    }
}

/*
Insert the new user
*/
void insert_user(char *username, const int *sock)
{
    usersPtr newUser;
    newUser = malloc(sizeof(Users));
    newUser->username =  malloc(USERNAME_LENGTH);
    log_type log_t;
    // is space available and the username is valid
    if ((newUser != NULL && newUser->username != NULL) && new_username_is_valid(username)) 
    {   
        memset(newUser->username, '\0', USERNAME_LENGTH); // clear piece of memory for string variable
        newUser->username = strncat(newUser->username, username, strlen(username));
        newUser->socketAddress = *sock;
        newUser->p_status = WAITING_FOR_A_PLAYER;
        newUser->nextUser = NULL;
        newUser->prevUser = NULL;

        if (IS_EMPTY(list_of_users))
        {
            list_of_users = newUser;
        }
        else
        {
            newUser->nextUser = list_of_users;
            list_of_users->prevUser = newUser;
            list_of_users = newUser;
        }

        send(new_socket, LOGIN_STATUS_OK, LOGIN_OK_SIZE, 0);

        log_t = OK;
        log_print(&log_t, "The user %s successfully created", username);
    }
    else
    {
        log_t = ERROR;

        if (IS_EMPTY(newUser) && IS_EMPTY(newUser->username))
        {
            send(new_socket, LOGIN_STATUS_FAILED_MEMORY_SIZE, LOGIN_NOT_OK_SIZE, 0);
            log_print(&log_t, "There is not enough server memory to create a new user");
        }
        else
        {
            send(new_socket, LOGIN_STATUS_FAILED_NOT_VALID_USERNAME, LOGIN_NOT_OK_SIZE, 0);
            log_print(&log_t, "The %s username exist so this is not valid!", username);
        }
    }
}

/*
delete a username using his name
return true if player status = playing
*/ 
int delete_user(const int *socket_addr)
{
    usersPtr *userLists = &list_of_users;
    usersPtr delUser = NULL;
    int socket_found = 0;
    char username[USERNAME_LENGTH];
    memset(username, 0, USERNAME_LENGTH);
    log_type log_t = INFO;
    int user_is_in_game = 0;
    
    if(!IS_EMPTY(*userLists))
    {
        while (!IS_EMPTY(*userLists))
        {
            if ((*userLists)->socketAddress == *socket_addr)
            {
                socket_found = 1;
                break;
            }
            userLists = &(*userLists)->nextUser;
        }

        if(!IS_EMPTY(*userLists) && socket_found)
        {   
            delUser = *userLists;
            if (delUser->p_status == PLAYING) user_is_in_game = 1;

            if (IS_EMPTY((*userLists)->nextUser) && IS_EMPTY((*userLists)->prevUser))
                *userLists = NULL;
            else
            {
                if (!IS_EMPTY((*userLists)->nextUser) && !IS_EMPTY((*userLists)->prevUser))
                {
                    usersPtr prev_user = (*userLists)->prevUser, next_user = (*userLists)->nextUser;

                    prev_user->nextUser = next_user;

                    next_user->prevUser = prev_user;

                    (*userLists)->prevUser =  prev_user;
                }

                else if (!IS_EMPTY((*userLists)->nextUser) && IS_EMPTY((*userLists)->prevUser))
                {
                    *userLists = (*userLists)->nextUser;
                    (*userLists)->prevUser = NULL;
                }
            }

            sprintf(username, "%s", delUser->username);
            close(delUser->socketAddress);
            free(delUser);
            delUser = NULL;
            return user_is_in_game;
            log_print(&log_t, "the %s user Delete", username);
        }

    }

    return user_is_in_game;
}

/*
This function try to find a player
*/
void find_a_player(const int *socket_address)
{
    usersPtr *__users = &list_of_users;
    usersPtr player1 = NULL, player2 = NULL;

    while (!IS_EMPTY(*__users))
    {
        if ((*__users)->socketAddress == *socket_address)
        {
            enqueue(*__users);
            break;
        }

        __users = &(*__users)->nextUser; 
    }

    dequeue(&player1, &player2);

    if (!IS_EMPTY(player1) && !IS_EMPTY(player2))
    {   
        create_a_playground(player1, player2);

        send(player1->socketAddress, PLAYER_FOUND_RESPONSE, strlen(PLAYER_FOUND_RESPONSE), 0);
        send(player2->socketAddress, PLAYER_FOUND_RESPONSE, strlen(PLAYER_FOUND_RESPONSE), 0);
    }
}

// TODO: Test this function 
void handle_disconnected_user(const int *socket_address)
{

    // if this user is in the playground, remove its competitor from the playground and change its competitor status 
    if (delete_user(socket_address))
        delete_playground(socket_address);

}


void create_a_playground(const usersPtr player1, const usersPtr player2)
{
    playGroundPtr new_playground;
    new_playground = malloc(sizeof(playGround));

    log_type log_t = OK;

    if (new_playground != NULL)
    {   
        player1->p_status = PLAYING;
        player2->p_status = PLAYING;
        memset(new_playground->ground, '-', GROUND_SIZE-1);
        new_playground->ground[GROUND_SIZE-1] = '\0';
        new_playground->player_one = player1;
        new_playground->player_two = player2;
        new_playground->player_one_char =  rand() % 2 ? 'X' : 'O';
        new_playground->player_two_char =  new_playground->player_one_char == 'O' ? 'X' : 'O';
        new_playground->first_player_points = new_playground->secound_player_points = 0; 
        new_playground->nextPlayGround = mainGround;
        mainGround = new_playground;

        send_playground_data(new_playground);
        log_print(&log_t, "create a playground. players: %s and %s", new_playground->player_one->username, new_playground->player_two->username);
    }
    else
    {
        log_t = ERROR;
        log_print(&log_t, "the memory does not have  free space to create a playground");
    }

}

//TODO: Do test this function
// @return the socket address of the user not disconnected
int delete_playground(const int *socket_addr)
{
    playGroundPtr *pg = &mainGround;
    int sd = 0;

    while (!IS_EMPTY(*pg))
    {
        if (((*pg)->player_one->socketAddress == *socket_addr) || ((*pg)->player_two->socketAddress == *socket_addr))
        {
            playGroundPtr delete_pg = *pg;
            *pg = (*pg)->nextPlayGround;

            if (!IS_EMPTY(delete_pg->player_one))
            {
                delete_pg->player_one->p_status = WAITING_FOR_A_PLAYER;
                sd = delete_pg->player_one->socketAddress;
            }
            
            if (!IS_EMPTY(delete_pg->player_two))
            {
                delete_pg->player_two->p_status = WAITING_FOR_A_PLAYER;
                sd = delete_pg->player_one->socketAddress;
            }

            free(delete_pg);
            break;
        }
        else
            pg = &(*pg)->nextPlayGround;
    }

    return sd;
}

//TODO: do test this function
//TODO: if the user choice is not valid! send a good message to his user and send message.
void perform_player_selection(const char *username_1, const unsigned short select, const char *username_2)
{
    playGroundPtr* pg = &mainGround;

    while (!IS_EMPTY(*pg))
    {
        if ((strcmp((*pg)->player_one->username, username_1) == 0) && (strcmp((*pg)->player_two->username, username_2) == 0))
        {
            if ((*pg)->ground[select] == '-')
                (*pg)->ground[select] = (*pg)->player_one_char;
            break;
        }
        else if((strcmp((*pg)->player_one->username, username_2) == 0) && (strcmp((*pg)->player_two->username, username_1) == 0))
        {
            if ((*pg)->ground[select] == '-')
                (*pg)->ground[select] = (*pg)->player_two_char;
            break;
        }
        else
        {
            pg = &(*pg)->nextPlayGround;
        }
    }

    check_who_is_winner(*pg);
    reset_playground(*pg);
    send_playground_data(*pg);
}

int check_who_is_winner(playGroundPtr pg)
{
    // Check lines
    if ((pg->ground[0] == pg->ground[1]) && pg->ground[2] == pg->ground[0])
    {
        if (pg->ground[0] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }

    if ((pg->ground[3] == pg->ground[4]) && pg->ground[5] == pg->ground[3])
    {
        if (pg->ground[3] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }

    if ((pg->ground[6] == pg->ground[7]) && pg->ground[8] == pg->ground[6])
    {
        if (pg->ground[6] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }

    //Check columns

    if ((pg->ground[0] == pg->ground[3]) && pg->ground[6] == pg->ground[0])
    {
        if (pg->ground[6] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }

    if ((pg->ground[1] == pg->ground[4]) && pg->ground[7] == pg->ground[1])
    {
        if (pg->ground[1] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }

    if ((pg->ground[2] == pg->ground[5]) && pg->ground[8] == pg->ground[2])
    {
        if (pg->ground[2] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }
    
    // Check diameters

    if ((pg->ground[2] == pg->ground[4]) && pg->ground[6] == pg->ground[2])
    {
        if (pg->ground[2] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }

    if ((pg->ground[0] == pg->ground[4]) && pg->ground[8] == pg->ground[0])
    {
        if (pg->ground[0] == 'X')
        {
            if (pg->player_one_char == 'X')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
        else
        {
            if (pg->player_one_char == 'O')
                pg->first_player_points++;
            else
                pg->secound_player_points++;
        }
    }

    int winner = 0;
    
    if (pg->first_player_points >= FINAL_SCORE)
        winner = 1;
    if (pg->secound_player_points >= FINAL_SCORE)
        winner = 2;

    return 0;
}

void send_playground_data(const playGroundPtr pg)
{

    char playground_data_one[PLAYGROUND_RESPONSE_SIZE] = {0};
    char playground_data_two[PLAYGROUND_RESPONSE_SIZE] = {0};

    sprintf(playground_data_one, PLAYGROUND_FORMAT, pg->player_one->username, pg->player_two->username, 
        pg->ground, pg->first_player_points, pg->secound_player_points, pg->player_one_char,
        pg->player_two_char);
    
    sprintf(playground_data_two, PLAYGROUND_FORMAT, pg->player_two->username, pg->player_one->username, 
        pg->ground, pg->secound_player_points, pg->first_player_points, pg->player_two_char,
        pg->player_one_char);
    
    send(pg->player_one->socketAddress, playground_data_one, PLAYGROUND_RESPONSE_SIZE, 0);
    send(pg->player_two->socketAddress, playground_data_two, PLAYGROUND_RESPONSE_SIZE, 0);
}

int reset_playground(playGroundPtr pg)
{
    short number_of_games_played = 0; 

    for (int i = 0; i < GROUND_SIZE; i++)
    {
        if (pg->ground[i] != '-')
            number_of_games_played++;
    }

    if (number_of_games_played > GROUND_SIZE)
    {
        memset(pg->ground, '-', GROUND_SIZE);
        return 1;
    }

    return 0;
}


void enqueue(usersPtr player)
{
    QueueOfPlayers q = malloc(sizeof(struct queue_of_players));
    if (!IS_EMPTY(q))
    {
        q->player = player;
        q->next = NULL;

        if (rear == NULL)
        {
            front = q;
            rear = q;
        }
        else
        {
            rear->next = q;
            rear = rear->next;
        }
    }
}

// get a pointer for each player (player number one and number two)
// if there are two players waiting in the queue, it will value both of them 
// Otherwise, set both of them to NULL
void dequeue(usersPtr *player_one, usersPtr *player_two)
{
    if ((!IS_EMPTY(front) && !IS_EMPTY(rear)) && (front->player->socketAddress != rear->player->socketAddress))
    {   
        QueueOfPlayers delete_queue = front;
        *player_one = front->player;
        front = front->next;
        free(delete_queue);

        delete_queue = front;
        *player_two = front->player;
        front = front->next;
        if (delete_queue->player->socketAddress == rear->player->socketAddress)
            rear = NULL;
        free(delete_queue);

    }

    else
    {
        *player_one = NULL;
        *player_two = NULL;
    }
}


// the new username is valid??
int new_username_is_valid(char *new_username)
{
    usersPtr *__users = &list_of_users;

    while (!IS_EMPTY(*__users))
    {
        if (strcmp((*__users)->username, new_username) == 0)
            return 0; // This username Exists! So new username is not Valid! it is must be unique
        __users = &(*__users)->nextUser;
    }

    return 1;
}

// Change the port of the server. If Can not change, Default Port is 8013
void chage_port(const char  *port_string)
{

    for (size_t i = 0; i < strlen(port_string); i++)
        if (!isdigit(port_string[0]))
            return;

    port = atoi(port_string);
}

void log_print(const log_type *type, const char* message, ...)
{   
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char new_message[1024] = {0};
    va_list varg;
    va_start(varg, message);
    

    switch (*type)
    {
    case OK:
        sprintf(new_message, LOG_OK_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, message);
        vfprintf(stdout, new_message,  varg);
        break;
    
    case WARNING:
        sprintf(new_message, LOG_WARNING_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, message);
        vfprintf(stdout, new_message, varg);
        break;
    
    case ERROR:
        sprintf(new_message, LOG_ERROR_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, message);
        vfprintf(stdout, new_message, varg);
        break;
    
    case INFO:
        sprintf(new_message, LOG_INFO_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, message);
        vfprintf(stdout, new_message, varg);
        break;

    default:
        sprintf(new_message, LOG_DEFAULT_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, message);
        vfprintf(stdout, new_message, varg);
        break;
    }

    va_end(varg);
    fflush(stdout);
}

void signal_handler(int EXIT_CODE)
{
    
    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    shutdown(master_socket, SHUT_RDWR);

    puts("\nSERVER DOWN");
    fflush(stdout);

    exit(EXIT_CODE);
}