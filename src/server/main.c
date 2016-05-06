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
#include <stdlib.h>
#include <unistd.h>

#include "../protocol.h"
#include "../helper.h"

#define FIRST_NAME "localhost"
#define RING 0
#define DOUBLE_LIST 1
#define PORT_FIRST 4001
#define PORT_LAST 4444
#define MAX_DATA 256
#define SENSOR_PERIOD 1000 //ms
#define SENSOR_TIMEOUT 1000 //ms
#define SERVER_TIMEOUT 1000 //ms

int mode = RING;
int sock_first, sock_last;
pthread_t first_thread;
struct sockaddr_in first_addr, last_addr;

// Sends initializing message to sensors
void send_init_msg()
{
    union msg msg;
    msg.init.type = INIT_MSG;
    msg.init.timeout = SENSOR_TIMEOUT;
    msg.init.period = SENSOR_PERIOD;
    int buf_len = sizeof(msg.init);
    unsigned char* buf = malloc(buf_len);
    if(pack_msg(&msg, buf, buf_len) < 0)
    {
        print_error("Failed to pack init msg");
        exit(-1);
    }
    unsigned first_len = sizeof(first_addr);
    if(sendto(sock_first, buf, buf_len, 0, (struct sockaddr* )&first_addr, first_len) < 0)
    {
        print_error("Failed to send init msg");
        exit(-1);
    }
    print_info("Init message sent");
    free(buf);
}

// Sends empty data message to first sensor to gather data
void send_data_msg()
{
    union msg msg;
    msg.data.type = DATA_MSG;
    msg.data.count = 0;
    msg.data.data = NULL;
    int buf_len = sizeof(msg.data);
    unsigned char* buf = malloc(buf_len);
    if(pack_msg(&msg, buf, buf_len) < 0)
    {
        print_error("Failed to pack data msg");
        exit(-1);
    }
    unsigned first_len = sizeof(first_addr);
    if(sendto(sock_first, buf, buf_len, 0, (struct sockaddr* )&first_addr, first_len) < 0)
    {
        print_error("Failed to send data msg");
        exit(-1);
    }
    print_info("Data message sent");
    free(buf);
}

// Loop for communication with first sensor
void* first_loop(void* arg)
{
    print_success("Loop thread started");
    send_init_msg();
    while(1)
    {
        send_data_msg();
        usleep(SERVER_TIMEOUT*1000);
    }
    return NULL;
}

// Loop for communication with last sensor
void last_loop()
{
    print_info("Last loop");
    while(1)
    {
        if(mode == RING)
        {
            print_info("Waiting for data...");
            char buf[MAX_DATA];
            unsigned last_addr_len = sizeof(last_addr);
            int len = recvfrom(sock_last, buf, MAX_DATA, 0, (struct sockaddr*)&last_addr, &last_addr_len);

            print_info("Received %d bytes: %s", len, buf);

            //TODO: parsowanie i przetwarzanie wyniku
        }
        else
        {
            print_info("Mode changed to double-list");
        }
        usleep(SERVER_TIMEOUT*1000);
    }
}

// Function initializing sockets
int initialize_sockets()
{
    sock_first = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_first < 0)
    {
        print_error("Failed to create socket");
        return -1;
    }
    sock_last = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_last < 0)
    {
        print_error("Failed to create socket");
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
    last_addr.sin_port = htons(PORT_LAST);

    //TODO gethostbyname jest sprecyzowana w dokumentacji jako przestarzała!!!
    #warning "Using obsolete gethostbyname function!"

    struct hostent *hostinfo = gethostbyname(FIRST_NAME);
    first_addr.sin_family = AF_INET;
    first_addr.sin_addr = *(struct in_addr *)hostinfo->h_addr;
    first_addr.sin_port = htons(PORT_FIRST);
}

// Creates thread for communication with first sensor
int initialize_thread()
{
    if(pthread_create(&first_thread, NULL, first_loop, NULL) != 0)
    {
        return -1;
    }
    print_info("Thread created");
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
        print_error("Failed to bind socket");
        return -1;
    }
    print_success("Server launched");
    if(initialize_thread() < 0)
    {
        print_error("Failed to init thread");
        return -1;
    }
    last_loop();
    pthread_join(first_thread, NULL);
    return 0;
}
