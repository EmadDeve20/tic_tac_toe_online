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

#define FOREVER 1

#define DEFAULT_PORT 8013
#define BUFFER_SIZE 1024
#define LOGIN_REQUEST "LOGIN"
#define RESTRICT_PARAS_CHAR " " // Actually space character
#define LOGIN_STATUS_OK "LOGIN OK"
#define LOGIN_STATUS_FAILED "LOGIN NK"
#define LOGIN_STATUS_SIZE 9
#define USERNAME_LENGTH 20

#define LOG_OK_FORMAT               "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;32m%s\n\033[0m"
#define LOG_WARNING_FORMAT          "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;33m%s\n\033[0m"
#define LOG_ERROR_FORMAT            "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;31m%s\n\033[0m"
#define LOG_INFO_FORMAT             "\033[0;37m[%d-%02d-%02d  %02d:%02d] \033[0;34m%s\n\033[0m"

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

int server_fd, new_socket, valread;
unsigned int port = DEFAULT_PORT;
char buffer[1024] = {0};
struct sockaddr_in socket_address;

typedef struct users {
    uint32_t ipAddress;
    char *username;
    player_status p_status;
    struct users* nextUser;
} Users;

typedef  Users* usersPtr;

//initialize the list of users as LinkedList
usersPtr list_of_users = NULL;

typedef struct playground {
    char ground[9];
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
void insert_user(char *username);
int user_list_is_empty(const usersPtr users_list);
int new_username_is_valid(char *);
void chage_port(const char *port);
void log_print(const log_type *type, const char* message, ...);

int main(int argc, char **argv)
{

    // Check If We Have an Argument, Change the Default Port
    if (argc > 1) chage_port(argv[1]);

    print_welcome_message();
    setup_server();
    

    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);

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
    int adderlen = sizeof(socket_address);
    int opt = 1;

    // Creating socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket Initialization Failed!");
        exit(EXIT_FAILURE);
    }

    // attaching socket to the port (defualt port is 8013)
    if (setsockopt(server_fd, SOL_SOCKET, (SO_REUSEADDR | SO_REUSEADDR),
    &opt, sizeof(opt)) == -1)
    {
        perror("attaching port for the socket failed!");
        exit(EXIT_FAILURE);
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = INADDR_ANY;
    socket_address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&socket_address, sizeof(socket_address)) < 0)
    {
        perror("The binding Failed!");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen Failed!");
        exit(EXIT_FAILURE);
    }

    while (FOREVER)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&socket_address, (socklen_t*)&adderlen)) < 0)
        {
            perror("Accept Failed!");
            exit(EXIT_FAILURE);
        }
        valread = read(new_socket, buffer, BUFFER_SIZE);

        if (valread >= 0)
        {
            char **request_parsed = requests_parser();
            memset(buffer, '\0', 1024); // clear the buffer
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
    char **sliced = malloc(10 * sizeof(char*));
    int sliced_idx = 0;

    while (string_slice != NULL)
    {
        sliced[sliced_idx] = string_slice;
        string_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
    }

    return sliced;

}

/*
Insert the new user
*/
void insert_user(char *username)
{
    usersPtr newUser;
    newUser = malloc(sizeof(Users));
    newUser->username =  malloc(USERNAME_LENGTH+1);
    log_type log_t;
    // is space available and the username is valid
    if ((newUser != NULL && newUser->username != NULL) && new_username_is_valid(username)) 
    {   
        memset(newUser->username, '\0', USERNAME_LENGTH+1); // clear piece of memory for string variable
        newUser->username = strncat(newUser->username, username, USERNAME_LENGTH);
        newUser->ipAddress = socket_address.sin_addr.s_addr;
        newUser->p_status = WAITING_FOR_A_PLAYER;
        newUser->nextUser = NULL;

        if (user_list_is_empty(list_of_users))
        {
            list_of_users = newUser;
        }
        else
        {
            usersPtr *users = &list_of_users; 
            while (!user_list_is_empty((*users)->nextUser)) 
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

// list_of_users is Empty??
int user_list_is_empty(const usersPtr users_list)
{
    return users_list == NULL;
}

// the new username is valid??
int new_username_is_valid(char *new_username)
{
    usersPtr *__users = &list_of_users;

    while (!user_list_is_empty(*__users))
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