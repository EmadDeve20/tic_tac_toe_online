#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int port = 8013, sock = 0, client_fd;
char server_address[1024];
char username[1024];
struct sockaddr_in socket_address;

void initial_settings();
void client_setup();


int main()
{

    initial_settings();
    client_setup();

}

// this is a form for entering username and server details
void initial_settings()
{
    
    printf("Enter Server IP: ");
    scanf("%1023s", server_address);

    printf("Enter Port Server: ");
    scanf("%d", &port);
    
    printf("Enter your name: ");
    scanf("%1023s", username);

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