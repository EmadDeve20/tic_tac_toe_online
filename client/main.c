#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>

#define LOGIN_REQUEST "LOGIN "
#define LOGIN_REQUEST_SIZE 26

int port = 8013, sock = 0, client_fd;
char server_address[1024];
char username[20];
struct sockaddr_in socket_address;

void initial_settings();
void client_setup();
void try_to_login();


int main()
{

    initial_settings();
    client_setup();
    try_to_login();

}

// this is a form for entering username and server details
void initial_settings()
{
    
    printf("Enter Server IP: ");
    scanf("%1023s", server_address);

    printf("Enter Port Server: ");
    scanf("%d", &port);
    
    printf("Enter your name: ");
    scanf("%19s", username);

}

/*
    setup client means connecting to the server
    and 
    start game!
*/
void client_setup()
{
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket Connection Error!\n");
        exit(EXIT_FAILURE);
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);

    // Convert IPv4 from text to binary
    if (inet_pton(AF_INET, server_address, &socket_address.sin_addr) <= 0)
    {
        perror("Invalid address or Address not supported\n");
        exit(EXIT_FAILURE);
    }

    if ((client_fd = connect(sock, (struct sockaddr*)&socket_address, sizeof(socket_address))) < 0)
    {
        perror("Connection Faild!\n");
        exit(EXIT_FAILURE);
    }
}

/*
This function is for trying to login with a new Account!
actually the account with the name parameter
*/
void try_to_login()
{   
    // char *login_request = strncat("LOGIN ", username, strlen(username));
    char login_request[LOGIN_REQUEST_SIZE] = LOGIN_REQUEST;
    strncat(login_request, username, strlen(username));
    printf("REQUEST TO SEND %s\n", login_request);
    send(sock, login_request, strlen(login_request), 0);
}