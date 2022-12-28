#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define LOGIN_REQUEST "LOGIN"
#define RESTRICT_PARAS_CHAR " " // Actually space character

int server_fd, new_socket, valread, port = 8013;
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

int main()
{

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
    char *token;
    token = strtok(buffer, RESTRICT_PARAS_CHAR);

    if (strcmp(token, LOGIN_REQUEST) == 0)
    {
        token = strtok(NULL, RESTRICT_PARAS_CHAR);
        insert_user(token);
    }

}

/*
Insert the new user
*/
void insert_user(char *username)
{
    usersPtr newUser;
    newUser = malloc(sizeof(Users));

    if (newUser != NULL) // is space available
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
    }
}