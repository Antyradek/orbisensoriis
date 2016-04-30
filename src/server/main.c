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

#define FIRST_NAME "localhost"
#define RING 0
#define DOUBLE_LIST 1
#define PORT 1337
#define MAX_DATA 256

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
        //TODO: wysyłanie wiadomości
    }
    return NULL;
}

// Loop for communication with last sensor
void last_loop()
{
    printf("Last loop\n");
    while(1)
    {
        if(mode == RING)
        {
            printf("Waiting for data...\n");
            char buf[MAX_DATA];
            unsigned last_addr_len = sizeof(last_addr);
            int len = recvfrom(sock_last, buf, MAX_DATA, 0, (struct sockaddr*)&last_addr, &last_addr_len);
            printf("Received %d bytes: %s\n", len, buf);
            //TODO: parsowanie i przetwarzanie wyniku
        }
        else
        {
            printf("Mode changed to double-list\n");
        }
    }
}

// Function initializing sockets
int initialize_sockets()
{
    sock_first = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_first < 0)
    {
        printf("Failed to create socket\n");
        return -1;
    }
    sock_last = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_last < 0)
    {
        printf("Failed to create socket\n");
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


    struct hostent *hostinfo = gethostbyname(FIRST_NAME);
    first_addr.sin_family = AF_INET;
    first_addr.sin_addr = *(struct in_addr *)hostinfo->h_addr;
    first_addr.sin_port = htons(PORT);    
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
        printf("Failed to bind socket\n");
        return -1;
    }
    printf("Server launched\n");
    if(initialize_thread() < 0)
    {
        printf("Failed to init thread\n");
        return -1;
    }
    last_loop();
    pthread_join(first_thread, NULL);
    return 0;
}