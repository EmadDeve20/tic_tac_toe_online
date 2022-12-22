#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void print_welcome_message();

int main()
{

    print_welcome_message();

}

void print_welcome_message()
{
    puts("TIC TAC TOE  SERVER IS UP!");
}