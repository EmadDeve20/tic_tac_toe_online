#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>

#define DEFAULT_PORT 8013
#define BUFFER_SIZE 1024
#define LOGIN_REQUEST "LOGIN"
#define RESTRICT_PARAS_CHAR " " // Actually space character
#define LOGIN_STATUS_OK "LOGIN OK"
#define LOGIN_STATUS_FAILED "LOGIN NK"
#define LOGIN_STATUS_SIZE 9

typedef enum LogMessageType
{
    OK,
    WARNING,
    ERROR,
    INFO,
} log_type;

int server_fd, new_socket, valread;
unsigned int port = DEFAULT_PORT;
char buffer[1024] = {0};
struct sockaddr_in socket_address;

typedef struct users {
    uint32_t ipAddress;
    char *username;
    struct users* nextUser;
} Users;

typedef  Users* usersPtr;

//initialize the list of users as LinkedList
usersPtr list_of_users;

void print_welcome_message();
void setup_server();
void requests_parser();
void insert_user(char *username);
int user_list_is_empty(const usersPtr users_list);
int new_username_valid(char *);
void chage_port(const char *port);
void log(const char* message, log_type message_type);

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

    if ((new_socket = accept(server_fd, (struct sockaddr*)&socket_address, (socklen_t*)&adderlen)) < 0)
    {
        perror("Accept Failed!");
        exit(EXIT_FAILURE);
    }

    // printf("%d\n", socket_address.sin_addr.s_addr);
    valread = read(new_socket, buffer, BUFFER_SIZE);

    if (valread >= 0)
    {
        printf("Data:%s\n", buffer);
        requests_parser();
    }

}

/*
this function parses all of the requests
*/
void requests_parser()
{
    char *string_slice;
    string_slice = strtok(buffer, RESTRICT_PARAS_CHAR);

    if (strcmp(string_slice, LOGIN_REQUEST) == 0)
    {
        string_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
        insert_user(string_slice);
    }

}

/*
Insert the new user
*/
void insert_user(char *username)
{
    usersPtr newUser;
    newUser = malloc(sizeof(Users));

    // is space available and the username is valid
    if (newUser != NULL && new_username_valid(username)) 
    {
        newUser->username = username;
        newUser->ipAddress = socket_address.sin_addr.s_addr;

        if (list_of_users == NULL)
        {
            newUser->nextUser = list_of_users;
            list_of_users = newUser;
        }
        else
        {
            list_of_users->nextUser = newUser;
            newUser->nextUser = NULL;
        }

        printf("\n\nUser %s added", username);
        send(new_socket, LOGIN_STATUS_OK, LOGIN_STATUS_SIZE, 0);
    }
    else
    {
        send(new_socket, LOGIN_STATUS_OK, LOGIN_STATUS_SIZE, 0);
    }
}

// list_of_users is Empty??
int user_list_is_empty(const usersPtr users_list)
{
    return users_list == NULL;
}

// the new username is valid??
int new_username_valid(char *new_username)
{
    usersPtr __users = list_of_users;

    while (!user_list_is_empty(__users))
    {
        if (strcmp((__users)->username, new_username) == 0)
            return 0; // This username Exists! So new username is not Valid! it is must be unique
        __users = (__users)->nextUser;
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