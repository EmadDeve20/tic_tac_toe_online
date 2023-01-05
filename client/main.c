#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#define USERNAME_LENGTH 20
#define LOGIN_REQUEST "LOGIN "
#define LOGOUT_REQUEST "LOGOUT "
#define LOGIN_REQUEST_SIZE  9 + USERNAME_LENGTH
#define LOGOU_REQUEST_SIZE 10 +  USERNAME_LENGTH
#define LOGIN_STATUS_OK "LOGIN OK"
#define LOGIN_STATUS_SIZE 9
#define LOGIN_REQUEST_FORMAT "LOGIN %s \r\n" // LOGIN $username
#define LOGOUT_REQUEST_FORMAT "LOGOUT %s \r\n" // LOGOUT $username

static volatile sig_atomic_t keep_running = 1;

int port = 8013, sock = 0, client_fd, valread;
char server_address[1024];
char username[USERNAME_LENGTH];
struct sockaddr_in socket_address;
char buffer[1024];

void initial_settings();
void client_setup();
void game_controller();
int try_to_login();
void logout();
void close_end_of_string(char *text);
static void signal_handler(int _);


int main()
{
    signal(SIGINT, signal_handler);
    initial_settings();
    client_setup();
    game_controller();
    logout();

}

// this is a form for entering username and server details
void initial_settings()
{
    printf("Enter your name: ");
    fgets(username, USERNAME_LENGTH, stdin);
    close_end_of_string(username);
    
    printf("Enter Server IP: ");
    fgets(server_address, 1024, stdin);
    close_end_of_string(server_address);

    printf("Enter Port Server: ");
    scanf("%d", &port);
    

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

void game_controller()
{

    printf("%s\n", try_to_login() ? "LOGIN TO SERVER" : "CAN NOT LOGIN TO SERVER! Maybe Your username is Exist! Try With another name");

    while (keep_running)
    {
        // SEND and RECEIVE data BETWEEN server and client
    }

}

/*
This function is for trying to login with a new Account!
actually the account with the name parameter
*/
int try_to_login()
{   
    char login_request[LOGIN_REQUEST_SIZE];
    sprintf(login_request, LOGIN_REQUEST_FORMAT, username);
    send(sock, login_request, strlen(login_request), 0);

    valread = read(sock, buffer, LOGIN_STATUS_SIZE);
    if (valread > 0)
    {
        if (strcmp(buffer, LOGIN_STATUS_OK) == 0)
            return 1;
    }

    return 0;
}

/*
send logout request 
*/
void logout()
{
    char logout_request[LOGOU_REQUEST_SIZE];
    sprintf(logout_request, LOGOUT_REQUEST_FORMAT, username);
    puts(logout_request);
    send(sock, logout_request, strlen(logout_request), 0);
}

/*
this function is for closing the end of text with a '\0' character
*/
void close_end_of_string(char *text)
{   

    if ((strlen(text) > 0) && (text[strlen (text) - 1] == '\n'))
        text[strlen (text) - 1] = '\0';
}

void signal_handler(int _)
{
    (void)_;
    keep_running = 0; 
}