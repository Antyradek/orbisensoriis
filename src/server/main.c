/**
    30.04.2016 - Server
    @author: Paweł Jankowski
*/
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

#define HOSTNAME "localhost"
#define RING 0
#define DOUBLE_LIST 1
#define PORT 1337

int mode = RING;
int sock_first, sock_last;
pthread_t first_thread;
struct sockaddr_in first_addr, last_addr;

// Loop for communication with first sensor
void* first_loop(void* arg)
{
    printf("Thread started\n");
    while(1)
    {

    }
    return NULL;
}

// Loop for communication with last sensor
void last_loop()
{
    printf("Last loop\n");
    while(1)
    {

    }
}

// Function initializing sockets
int initialize_sockets()
{
    sock_first = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_first < 0)
    {
        printf("Failed to create socket");
        return -1;
    }
    sock_last = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_last < 0)
    {
        printf("Failed to create socket");
        return -1;
    }
    return 0;
}

// Fills sock_addr structures
void initilize_sockaddr()
{
    memset(&first_addr, 0, sizeof(first_addr));
    memset(&last_addr, 0, sizeof(last_addr));

    last_addr.sin_family = AF_INET;
    last_addr.sin_addr.s_addr = INADDR_ANY;
    last_addr.sin_port = htons(PORT);

    first_addr.sin_family = AF_INET;
}

// Creates thread for communication with first sensor
int initialize_thread()
{
    if(pthread_create(&first_thread, NULL, first_loop, NULL) != 0)
    {
        return -1;
    }   
    printf("Thread created\n");
    return 0;
}

int main(int argc, char const *argv[])
{
    if(initialize_sockets() < 0)
    {
        return -1;
    }
    initilize_sockaddr();
    if(bind(sock_last, (struct sockaddr*) &last_addr, sizeof(last_addr)) < 0)
    {
        printf("Failed to bind socket");
        return -1;
    }
    if(initialize_thread() < 0)
    {
        printf("Failed to init thread");
        return -1;
    }
    printf("Server launched\n");
    last_loop();
    pthread_join(first_thread, NULL);
    printf("Job finished\n");
    return 0;
}