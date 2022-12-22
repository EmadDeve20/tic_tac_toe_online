#include <stdio.h>

int port = 8013;
char server_address[1024];
char username[1024];

void initial_settings();

int main()
{

    initial_settings();

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