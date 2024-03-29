#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <regex.h>
#include <string.h>


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
#define GET_PLAYGROUND_REQUEST "GET PLAYGROUND"
#define COMPETITOR_DISCONNECTED_RESPONSE "COMPETITOR DISCONNECTED"
#define SELECT_REQUEST_FORMAT "SELECT %s %d %s \r\n" // SELECT $username $choose_number $competitor
#define SELECT_REQUEST_LENGTH ((USERNAME_LENGTH*2) + 20)
#define PLAYGROUND_SIZE 10
#define CLEAR_SCREEN clear_screen();
#define PLAYER_FOUND_RESPONSE "PLAYER FOUND"
#define REMOVE_COMPETITOR memset(competitor, '\0', USERNAME_LENGTH)
#define REMOVE_TURN_PLAYING memset(turn_playing, '\0', USERNAME_LENGTH)

regex_t reegex;
static volatile sig_atomic_t keep_running = 1;

pthread_t thread;
pthread_attr_t pthread_custom_attr;

// login status 0 means memory is not enough, login status 1 means login successful, and login status -1 means this username exists
int port = 8013, sock = 0, client_fd, valread, game_is_start = 0, login_status = 2;

char server_address[1024];
char username[USERNAME_LENGTH] = {0} ,turn_playing[USERNAME_LENGTH] = {0}, competitor[USERNAME_LENGTH] = {0};
char playground[PLAYGROUND_SIZE];
struct sockaddr_in socket_address;
unsigned short user_points, competitor_points;
char player_character, competitor_character;

#define CLEAR_BUFFER memset(buffer, '\0', BUFFER_SIZE)

void initial_settings();
int client_setup();
void game_controller();
void send_login_request();
void request_to_find_a_player();
void close_end_of_string(char *text);
int select_player_request(const char select);
char selected_number();
void playground_parser(char *buffer);
void response_manager(char *buffer);
void save_playground_status(const char *player, const char *competitor_name, const char playground_cp[PLAYGROUND_SIZE],
    unsigned short user_pt, unsigned short competitor_ps);
void clear_screen();
void change_username();
// int check_player_found();

void draw_playground();
void play_game();
void playing();
void remove_competitor_data();

static void signal_handler(int _);

void *server_listener(void *arg)
{   
    int sock = *((int *)arg);

    char buffer[BUFFER_SIZE] = {0};

    while (1)
    {
        recv(sock, buffer, BUFFER_SIZE, 0);

        if (strlen(buffer) != 0)
        {
            response_manager(buffer);
            CLEAR_BUFFER;
        }
    }
}


int main()
{
    signal(SIGINT, signal_handler);
    initial_settings();
    if (!client_setup()) return 1;
    game_controller();
    pthread_join(thread, NULL);
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
int client_setup()
{   
    errno = pthread_attr_init(&pthread_custom_attr);
    if (errno != 0)
    {
        puts("pthread_attr_init");
        return 0;
    }


    

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

    errno = pthread_create(&thread, &pthread_custom_attr, server_listener, (void *)&sock);
    if (errno != 0)
    {
        puts("Fail to create thread");
        return 0;
    }
    
    errno = pthread_attr_destroy(&pthread_custom_attr);
    if (errno != 0)
    {
      puts("pthread_attr_destroy");
      return 0;
    }

    return 1;
}

void game_controller()
{
    CLEAR_SCREEN;
    send_login_request();

    // printf("%s\n", send_login_request() ? "LOGIN TO SERVER" : "CAN NOT LOGIN TO SERVER! Maybe Your username is Exist! Try With another name");

    // TODO: fix this! When the username is not valid this must happen!

    while (login_status == 2) {/* Wait until the login_status number changes */}

    while (login_status == -1)
    {
        change_username();
        send_login_request(); 
    }
    
    if (login_status == 0)
        exit(EXIT_FAILURE);
    else
        game_is_start = 1;


    request_to_find_a_player();
    puts("the Server Searching to find a player  ...");
    fflush(stdout);

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
void send_login_request()
{   
    char login_request[LOGIN_REQUEST_SIZE];
    sprintf(login_request, LOGIN_REQUEST_FORMAT, username);
    send(sock, login_request, strlen(login_request), 0);
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


// int check_player_found()
// {
//     recv(sock, buffer, BUFFER_SIZE, 0);

//     if ((strlen(buffer) == strlen(PLAYER_FOUND_RESPONSE)) && (strcmp(buffer, PLAYER_FOUND_RESPONSE) == 0))
//         return 1;

//     return 0;
//     CLEAR_BUFFER;

// }

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

void playground_parser(char *buffer)
{
    char *buffer_slice; 
    int index = 0;

    buffer_slice = strtok(buffer, RESTRICT_PARAS_CHAR);

    if (buffer_slice == NULL || strcmp("PLAYGROUND",buffer_slice))
        return;

    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);

    if (buffer_slice == NULL || strcmp(username, buffer_slice))
        return;
    
    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);

    if (strlen(competitor) == 0)
    {
        while (*buffer_slice != '\0')
        {
            competitor[index] = *buffer_slice;
            buffer_slice = buffer_slice+1;
            index++;
        }
    }


    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
    index = 0;
    while (*buffer_slice != '\0')
    {
        playground[index] = *buffer_slice;
        buffer_slice = buffer_slice+1;
        index++;
    }

    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
    user_points = atoi(buffer_slice);

    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
    competitor_points = atoi(buffer_slice);

    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
    player_character = *buffer_slice;

    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
    competitor_character = *buffer_slice;

    buffer_slice = strtok(NULL, RESTRICT_PARAS_CHAR);
    index = 0;
    while (*buffer_slice != '\0')
    {
        turn_playing[index] = *buffer_slice;
        buffer_slice++;
        index++;
    }
}

void response_manager(char *buffer)
{   
    int regex_value;
    regex_value = regcomp(&reegex, "^PLAYGROUND.*", 0);
    regex_value = regexec(&reegex, buffer, 0, NULL, 0);

    // match
    if (regex_value == 0) 
    {   
        playground_parser(buffer);
        draw_playground();
        play_game();
    }
    else // not match
    {

        if (strlen(buffer) == LOGIN_OK_SIZE && strcmp(buffer, LOGIN_STATUS_OK) == 0)
            login_status = 1;
        
        if (strlen(buffer) == LOGIN_NOT_OK_SIZE)
        {
            if (strcmp(buffer, LOGIN_STATUS_FAILED_MEMORY_SIZE) == 0)
            {
                login_status = 0;
                printf("the server memory is not enough! please try next time.\n");
                fflush(stdout);
            }

            else if (strcmp(buffer, LOGIN_STATUS_FAILED_MEMORY_SIZE) == 0)
            {
                login_status = -1;
                printf("this name exists please choose another name! \n");
                fflush(stdout);
            }
        }

        if (strlen(buffer) == strlen(COMPETITOR_DISCONNECTED_RESPONSE) && strcmp(buffer, COMPETITOR_DISCONNECTED_RESPONSE) == 0)
        {   
            CLEAR_SCREEN
            remove_competitor_data();
            puts("competitor disconnected");
            puts("Wait for a player to found");
            request_to_find_a_player();
            fflush(stdout);
        }

        // TODO: Think About the playerfound response is nesseary or it is useless!
        // if (strlen(buffer) == strlen(PLAYER_FOUND_RESPONSE) && strcmp(buffer, PLAYER_FOUND_RESPONSE) == 0)
        // {
        //     printf("Player Found! \n");
        //     fflush(stdout);
        // }

    }    

    // if (strcmp(PLAYGROUND_RESPONSE, response_parsed[0]) == 0)
    // {
    //     save_playground_status(response_parsed[1], response_parsed[2], response_parsed[3],
    //     atoi(response_parsed[4]), atoi(response_parsed[5]));
    // }
}

void remove_competitor_data()
{
    REMOVE_COMPETITOR;
    REMOVE_TURN_PLAYING;
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

    printf("%s: %d\t\t\t%s: %d\n", username, user_points, competitor, competitor_points);
    printf("Your Character: %c\t\t\tCompetitor Character: %c\n\n\n",  player_character, competitor_character);
    
    for (int i = 0; i < PLAYGROUND_SIZE; i++)
    {
        printf("\t%c", playground[i]);
        
        if ((i + 1) % 3 == 0)
            printf("\n");

    }

    fflush(stdout);
}

void play_game()
{
    if (strcmp(username, turn_playing) != 0)
    {
        printf("\n\nWait for the opponent to play ...\n");
        fflush(stdout);
    }
    else if (strcmp(username, turn_playing) == 0)
    {
        printf("\n\nIt's your turn to choose. Enter the desired number (between 1 and 9)\n");
        fflush(stdout);
        playing();
    }
}

void playing()
{
    int choose;
    char input[100];
    
    fgets(input, sizeof(input), stdin);
    
    while (sscanf(input, "%d", &choose) != 1)
    {
        printf("Please Enter a Number Not Character!\n");
        fflush(stdout);
        fgets(input, sizeof(input), stdin);
    }

    while (1)
    {

        if (choose < 1 || choose > 9)
        {
            printf("Please choose between 1 and 9 numbers!\n");
            fflush(stdout);
            scanf("%d", &choose);
        }
        else
        {   
            choose--;
            if (playground[choose] != '-')
            {
                printf("This number has been selected! choose another number\n");
                fflush(stdout);
                scanf("%d", &choose);
            }
            else
            {
                char select_request[SELECT_REQUEST_LENGTH] = {0};
                sprintf(select_request, SELECT_REQUEST_FORMAT, username, choose, competitor);

                send(sock, select_request, SELECT_REQUEST_LENGTH, 0);
                return;
            }
        }
    
    }
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

    pthread_cancel(thread);
}

void clear_screen()
{
    #ifdef __WIN32
        system("cls");
    #else
        system("clear");
    #endif
}