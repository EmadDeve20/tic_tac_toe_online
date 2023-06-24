#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define USERNAME_LENGTH 20
#define RESTRICT_PARAS_CHAR " " // Actually space character
#define LOGIN_REQUEST "LOGIN"
#define PLAYGROUND_RESPONSE "PLAYGROUND"
#define LOGIN_REQUEST_SIZE  9 + USERNAME_LENGTH
#define LOGIN_STATUS_OK "LOGIN OK"
#define LOGIN_STATUS_FAILED_MEMORY_SIZE "LOGIN NKM"
#define LOGIN_STATUS_FAILED_NOT_VALID_USERNAME "LOGIN NKU"
#define LOGIN_OK_SIZE strlen(LOGIN_STATUS_OK)
#define LOGIN_NOT_OK_SIZE strlen(LOGIN_STATUS_FAILED_MEMORY_SIZE)
#define LOGIN_REQUEST_FORMAT "LOGIN %s \r\n" // LOGIN $username
#define LOGOUT_REQUEST_FORMAT "LOGOUT %s \r\n" // LOGOUT $username
#define FIND_PLAYER_REQUEST_FORMAT "FIND %s \r\n" // FIND $username 
#define FIND_PLAYER_REQUEST_LENGTH 29
#define SELECT_REQUEST_FORMAT "SELECT %s %c %s \r\n" // SELECT $username $select $competitor
#define SELECT_REQUEST_LENGTH ((USERNAME_LENGTH*2) + 20)
#define PLAYGROUND_SIZE 9
#define CLEAR_SCREEN clear_screen();
#define PLAYER_FOUND_RESPONSE "PLAYER FOUND"


static volatile sig_atomic_t keep_running = 1;

int port = 8013, sock = 0, client_fd, valread, game_is_start = 0;
char server_address[1024];
char username[USERNAME_LENGTH] = {0};
char competitor[USERNAME_LENGTH];
char playground[PLAYGROUND_SIZE];
struct sockaddr_in socket_address;
char buffer[BUFFER_SIZE];
unsigned short user_points, competitor_points; 

#define CLEAR_BUFFER memset(buffer, '\0', BUFFER_SIZE)

void initial_settings();
void client_setup();
void game_controller();
int try_to_login();
void request_to_find_a_player();
void close_end_of_string(char *text);
int select_player_request(const char select);
char selected_number();
char** response_parser();
void response_manager(char **response_parsed);
void save_playground_status(const char *player, const char *competitor_name, const char playground_cp[PLAYGROUND_SIZE],
    unsigned short user_pt, unsigned short competitor_ps);
void clear_screen();
void change_username();
int check_player_found();

void draw_playground();
static void signal_handler(int _);


int main()
{
    signal(SIGINT, signal_handler);
    initial_settings();
    client_setup();
    game_controller();

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

void change_username()
{   
    memset(username, '\0', USERNAME_LENGTH);
    printf("Enter your name: ");
    fflush(stdout);
    fgets(username, USERNAME_LENGTH, stdin);
    close_end_of_string(username);
}

/*
    setup client means connecting to the server
    and 
    start game!
*/
void client_setup()
{
    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);

    // Convert IPv4 from text to binary
    if (inet_pton(AF_INET, server_address, &socket_address.sin_addr) <= 0)
    {
        perror("Invalid address or Address not supported\n");
        exit(EXIT_FAILURE);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket Connection Error!\n");
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
    CLEAR_SCREEN;
    
    // printf("%s\n", try_to_login() ? "LOGIN TO SERVER" : "CAN NOT LOGIN TO SERVER! Maybe Your username is Exist! Try With another name");

    int login_status = try_to_login(); 

    // TODO: fix this! When the username is not valid this must happen!
    while (login_status == -1)
    {
        change_username();
        int login_status = try_to_login(); 
    }
    
    if (login_status == 0)
        exit(EXIT_FAILURE);
    else
        game_is_start = 1;


    request_to_find_a_player();
    puts("the Server Searching to find a player  ...");
    fflush(stdout);

    //TODO: this is like endless loop beacuse the recive function dont set timeout!
    WAIT_FOR_PLAYER:
    
    if (check_player_found())
        goto START_GAME;
    else
        goto WAIT_FOR_PLAYER;

    START_GAME:

    puts("a player found!");
    fflush(stdout);
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

    recv(sock, buffer, BUFFER_SIZE, 0);
    
    if ((strlen(buffer) == LOGIN_OK_SIZE) && (strcmp(buffer, LOGIN_STATUS_OK) == 0))
        return 1;

    else if (strlen(buffer) == LOGIN_NOT_OK_SIZE)
    {
        if (strcmp(buffer, LOGIN_STATUS_FAILED_MEMORY_SIZE) == 0)
        {
            printf("the server memory is not enough! please try next time.\n");
            fflush(stdout);
        }

        else if (strcmp(buffer, LOGIN_STATUS_FAILED_NOT_VALID_USERNAME) == 0)
        {
            printf("this name exists please choose another name! \n");
            fflush(stdout);
            return -1;
        }
    }

    CLEAR_BUFFER;
    return 0;
}


/*
send the find a player request
*/
void request_to_find_a_player()
{
    char find_request[FIND_PLAYER_REQUEST_LENGTH];

    sprintf(find_request, FIND_PLAYER_REQUEST_FORMAT, username);
    send(sock, find_request, strlen(find_request), 0);
}


int check_player_found()
{
    recv(sock, buffer, BUFFER_SIZE, 0);

    if ((strlen(buffer) == strlen(PLAYER_FOUND_RESPONSE)) && (strcmp(buffer, PLAYER_FOUND_RESPONSE) == 0))
        return 1;

    return 0;
    CLEAR_BUFFER;

}

int select_player_request(const char select)
{
    char select_request[SELECT_REQUEST_LENGTH];
    sprintf(select_request, SELECT_REQUEST_FORMAT, username, select, competitor);
    
    return (send(sock, select_request, SELECT_REQUEST_LENGTH, 0) == -1) ? 0 : 1;
}

char selected_number()
{
    char s;

    while (!isdigit((s = getc(stdin))));

    return s;
}

char** response_parser()
{
    char *response_slice; 
    char **parsed = malloc(10 * sizeof(char*));

    response_slice = strtok(buffer, RESTRICT_PARAS_CHAR);

    int parsed_idx = 0;

    while ((response_slice != NULL) && parsed_idx < 10)
    {
        parsed[parsed_idx] = response_slice;
        response_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
        parsed_idx++;
    }

    return parsed; 
}

void response_manager(char **response_parsed)
{
    if (strcmp(PLAYGROUND_RESPONSE, response_parsed[0]) == 0)
    {
        save_playground_status(response_parsed[1], response_parsed[2], response_parsed[3],
        atoi(response_parsed[4]), atoi(response_parsed[5]));
    }
}

void save_playground_status(const char *player, const char *competitor_name, const char playground_cp[PLAYGROUND_SIZE], 
unsigned short user_pt, unsigned short competitor_ps)
{

    if ((strncmp(username, player, USERNAME_LENGTH)) != 0)
        return;

    if (strlen(competitor) == 0)
        strncat(competitor, competitor_name, USERNAME_LENGTH);

    if (strlen(playground) == 0)
        strncat(playground, playground_cp, PLAYGROUND_SIZE);
    
    if (user_points != user_pt)
        user_points = user_pt;
    
    if (competitor_ps != competitor_points)
        competitor_points = competitor_ps;

}

void draw_playground()
{

    CLEAR_SCREEN;

    printf("%s: %d\t\t\t%s: %d\n\n\n", username, user_points, competitor, competitor_points);
    
    for (int i = 0; i < PLAYGROUND_SIZE; i++)
    {
        printf("%c ", playground[i]);
        
        if ((i + 1) % 3 == 0)
            printf("\n");

    }

    fflush(stdout);
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
    if (game_is_start)
    {
        (void)_;
        keep_running = 0; 
    }
    else
    {
        exit(_);
    }
}

void clear_screen()
{
    #ifdef __WIN32
        system("cls");
    #else
        system("clear");
    #endif
}