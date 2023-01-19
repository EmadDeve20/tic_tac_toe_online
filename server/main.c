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

#define IS_EMPTY(X) ((X) == NULL)
#define FOREVER 1

#define DEFAULT_PORT 8013
#define BUFFER_SIZE 1024
#define LOGIN_REQUEST "LOGIN"
#define LOGOUT_REQUEST "LOGOUT"
#define FIND_PLAYER_REQUEST "FIND"
#define RESTRICT_PARAS_CHAR " " // Actually space character
#define LOGIN_STATUS_OK "LOGIN OK"
#define LOGIN_STATUS_FAILED "LOGIN NK"
#define LOGIN_STATUS_SIZE 9
#define USERNAME_LENGTH 20
#define GROUND_SIZE 9
#define FINAL_SCORE 5

#define LOG_OK_FORMAT               "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;32m%s\n\033[0m"
#define LOG_WARNING_FORMAT          "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;33m%s\n\033[0m"
#define LOG_ERROR_FORMAT            "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;31m%s\n\033[0m"
#define LOG_INFO_FORMAT             "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;34m%s\n\033[0m"

#define PLAYGROUND_FORMAT           "PLAYGROUND %s %s %s %d %d \r\n"
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
    int socketAddress;
    char *username;
    player_status p_status;
    struct users* nextUser;
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
} playGround;

typedef playGround *playGroundPtr;

//initialize the playground as LinkedList
playGroundPtr mainGround = NULL;

void print_welcome_message();
void setup_server();
char** requests_parser();
void manage_requests(char** request_parsed, const int *sock);
void insert_user(char *username, const int *sock);
int delete_user(const int socket_addr);
char* find_a_player(const char *us_req);
void handle_disconnected_user(const int *socket_address);
void create_a_playground(const usersPtr player1, const usersPtr player2);
int delete_playground(const int socket_addr);
int new_username_is_valid(char *);
void chage_port(const char *port);
void log_print(const log_type *type, const char* message, ...);
void signal_handler(int EXIT_CODE);
void perform_player_selection(const char *username_1, const unsigned short select, const char *username_2);
int check_who_is_winner(playGroundPtr pg);
void send_playground_data(const playGroundPtr pg);
int reset_playground(playGroundPtr pg);

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
    usersPtr *__users;
    int max_sd;
    int activity;

    while (FOREVER)
    {

        FD_ZERO(&readfds);

        FD_SET(master_socket, &readfds);

        int max_sd = master_socket;

        __users = &list_of_users;

        while (!IS_EMPTY(*__users))
        {
            int sd = (*__users)->socketAddress;
            
            if (sd > 0)
                FD_SET(sd, &readfds);
            
            __users = &(*__users)->nextUser;

        }
        
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

         __users = &list_of_users;

        while (!IS_EMPTY(*__users))
        {
            int sd = (*__users)->socketAddress;
            
            if (FD_ISSET(sd, &readfds))
            {   
                if ((valread = read(sd, buffer, 1024)) == 0)
                {
                    handle_disconnected_user(&sd);
                }
                else
                {
                    char **request_parsed = requests_parser();
                    manage_requests(request_parsed, &sd);
                    memset(buffer, '\0', 1024); // clear the buffer
                }
            }
            
            __users = &(*__users)->nextUser;
        }
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

    if (strcmp(request_parsed[0], LOGOUT_REQUEST) == 0)
    {
        // delete_user(request_parsed[1]);
        return;
    }
    if (strcmp(request_parsed[0], FIND_PLAYER_REQUEST) == 0)
    {
        // TODO: do something
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

        if (IS_EMPTY(list_of_users))
        {
            list_of_users = newUser;
        }
        else
        {
            usersPtr *users = &list_of_users; 
            while (!IS_EMPTY((*users)->nextUser)) 
                users = &(*users)->nextUser;

            (*users)->nextUser = newUser;
        }

        log_t = OK;
        send(new_socket, LOGIN_STATUS_OK, LOGIN_STATUS_SIZE, 0);
    }
    else
    {
        log_t = ERROR;
        send(new_socket, LOGIN_STATUS_FAILED, LOGIN_STATUS_SIZE, 0);
    }

    log_t == OK ? 
      log_print(&log_t, username, "user successfully created") : 
      log_print(&log_t, "Memory is not available for new user creation or the ", username, " username is exist"); 
}

/*
delete a username using his name
@return if user status is PLAYING return is 1 else is 0
*/ 
int delete_user(const int socket_addr)
{
    usersPtr *userLists = &list_of_users;
    usersPtr prevUser;
    usersPtr curentUser;
    usersPtr delUser;
    char username[USERNAME_LENGTH];
    player_status user_status = WAITING_FOR_A_PLAYER;
    log_type log_t = INFO;
    
    if(!IS_EMPTY(*userLists))
    {
    
        if ((*userLists)->socketAddress == socket_addr)
        {
            strncat(username, (*userLists)->username, strlen((*userLists)->username));
            *userLists = (*userLists)->nextUser;
            delUser = (*userLists);
            user_status = delUser->p_status;
            free(delUser);
        }
        else
        {   
            curentUser = (*userLists)->nextUser;
            while (curentUser->socketAddress == socket_addr && !IS_EMPTY(curentUser))
            {
                prevUser = curentUser;
                curentUser = curentUser->nextUser;
            }
            
            if(!IS_EMPTY(curentUser))
            {   
                delUser = curentUser;
                prevUser->nextUser = delUser->nextUser;
                strncat(username, curentUser->username, strlen(curentUser->username));
                user_status = delUser->p_status;
                free(delUser);
            }
        }

        // TODO: Use this the log print
        // log_print(&log_t, "LOGOUT USER: ", username_tmp);
        printf("USER DELETED:%s\n", username);
    }

    return (user_status == PLAYING) ? 1 : 0;
}

/*
This function try to find a player
@return competitor's name
*/
char* find_a_player(const char *us_req)
{
    char *username = {0};
    usersPtr *__users = &list_of_users;
    usersPtr player1 = NULL, player2 = NULL;

    while (!IS_EMPTY(*__users))
    {
        if ((*__users)->p_status == WAITING_FOR_A_PLAYER && (strcmp((*__users)->username, us_req)))
        {
            player2 = *__users;
            player2->p_status = PLAYING;
        }
        if (strcmp((*__users)->username, us_req) == 0)
        {
            if ((*__users)->p_status != PLAYING)
            {
                player1 = *__users;
                player1->p_status = PLAYING;
            }
            else
            {
                if (!IS_EMPTY(player2))
                    player2->p_status = WAITING_FOR_A_PLAYER;
                break;
            }
        }
        if (!IS_EMPTY(player1) && !IS_EMPTY(player2) && (player1->p_status == PLAYING && player2->p_status == PLAYING))
            break;

        __users = &(*__users)->nextUser; 
    }

    if (!IS_EMPTY(player1) && !IS_EMPTY(player2) && (player1->p_status == PLAYING && player2->p_status == PLAYING))
    {
        username = player2->username;
        create_a_playground(player1, player2);
    }

    return username;
}

// TODO: Test this function 
void handle_disconnected_user(const int *socket_address)
{

    delete_playground(*socket_address);
    delete_user(*socket_address);

}

//TODO: test this function
//TODO: add logs for this
void create_a_playground(const usersPtr player1, const usersPtr player2)
{
    playGroundPtr new_playground;
    new_playground = malloc(sizeof(playGround));

    if (new_playground != NULL)
    {
        memset(new_playground->ground, '-', GROUND_SIZE);
        new_playground->player_one = player1;
        new_playground->player_two = player2;
        new_playground->player_one_char =  rand() % 2 ? 'X' : 'O';
        new_playground->player_two_char =  new_playground->player_one_char == 'O' ? 'X' : 'O';
        new_playground->first_player_points = new_playground->secound_player_points = 0; 
        new_playground->nextPlayGround = mainGround;
        mainGround = new_playground;
    }

}

//TODO: Do test this function
// @return the socket address of the user not disconnected
int delete_playground(const int socket_addr)
{
    playGroundPtr *pg = &mainGround;
    int sd = 0;

    while (!IS_EMPTY(*pg))
    {
        if (((*pg)->player_one->socketAddress == socket_addr) || ((*pg)->player_two->socketAddress == socket_addr))
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

//TODO: I must test this Function!
void send_playground_data(const playGroundPtr pg)
{

    char playground_data_one[PLAYGROUND_RESPONSE_SIZE];
    char playground_data_two[PLAYGROUND_RESPONSE_SIZE];

    sprintf(playground_data_one, PLAYGROUND_FORMAT, pg->player_one->username, pg->player_two->username, 
        pg->ground, pg->first_player_points, pg->secound_player_points);
    
    sprintf(playground_data_one, PLAYGROUND_FORMAT, pg->player_two->username, pg->player_one->username, 
        pg->ground, pg->secound_player_points, pg->first_player_points);
    
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

//TODO: this function has a big bug! try to solve it
void log_print(const log_type *type, const char* message, ...)
{   
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    char new_message[1024] = {0};
    va_list varg;
    va_start(varg, message);
    //printf("Messga %s\n", message);
    while (message != NULL && (strlen(message) + strlen(new_message)) < 1024)
    {   //printf("%s\n", message);
        strncat(new_message, message, strlen(message));
        strcat(new_message, " ");
        message = va_arg(varg, const char*);
    }

    va_end(varg);


    switch (*type)
    {
    case OK:
        printf(LOG_OK_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, new_message);
        break;
    
    case WARNING:
        printf(LOG_WARNING_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, new_message);
        break;
    
    case ERROR:
        printf(LOG_ERROR_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, new_message);
        break;
    
    case INFO:
        printf(LOG_INFO_FORMAT, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, new_message);
        break;

    default:
        printf("[%d-%02d-%02d  %02d:%02d] %s\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, new_message);
        break;
    }

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