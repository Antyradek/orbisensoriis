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
#include <sys/time.h>
#include <errno.h>

#include "../protocol.h"
#include "../helper.h"

#define DEF_FIRST_NAME "localhost"
#define FIRST_NAME_MAX_LEN 128
#define RING 0
#define DOUBLE_LIST 1
#define DEF_PORT_FIRST 4001
#define DEF_PORT_LAST 4444
#define MAX_DATA 256
#define FIRST 1
#define LAST -1
#define DEF_SENSOR_PERIOD 1000 //ms
#define DEF_SENSOR_TIMEOUT 1000 //ms
#define SERVER_TIMEOUT 1000 //ms
#define ERROR_TIMEOUT 3000 //ms

int resolving_error = 0;
int mode = RING;
int close_first = 0;
int sock_first, sock_last;
pthread_t first_thread;
struct sockaddr_in first_addr, last_addr;
struct timeval timeout, error_timeout;

char first_name[FIRST_NAME_MAX_LEN];
unsigned short port_first;
unsigned short port_last;
unsigned short sensor_period;
unsigned short sensor_timeout;


// Sends initializing message to sensors
void send_init_msg()
{
    union msg msg;
    msg.init.type = INIT_MSG;
    msg.init.timeout = sensor_timeout;
    msg.init.period = sensor_period;

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
void send_data_msg(int num)
{
    int sockfd = sock_first;
    struct sockaddr_in addr = first_addr;
    char* nums = "first";
    if(num == LAST)
    {
        sockfd = sock_last;
        addr = last_addr;
        nums = "last";
    }
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
    unsigned first_len = sizeof(addr);
    if(sendto(sockfd, buf, buf_len, 0, (struct sockaddr* )&addr, first_len) < 0)
    {
        print_error("Failed to send data msg");
        exit(-1);
    }
    print_info("Data message sent to %s sensor", nums);
    free(buf);
}

// Sends error message
int send_error_msg(int num)
{
    int sockfd = sock_first;
    struct sockaddr_in addr = first_addr;
    char* nums = "first";
    if(num == LAST)
    {
        sockfd = sock_last;
        addr = last_addr;
        nums = "last";
    }
    union msg msg;
    msg.info.type = ERR_MSG;
    int buf_len = sizeof(msg.info);
    unsigned char* buf = malloc(buf_len);
    if(pack_msg(&msg, buf, buf_len) < 0)
    {
        print_error("Failed to pack info msg");
        exit(-1);
    }
    unsigned len = sizeof(addr);
    if(sendto(sockfd, buf, buf_len, 0, (struct sockaddr* )&addr, len) < 0)
    {
        print_error("Failed to send error msg\nClosing socket");
        close(sockfd);
        free(buf);
        return -1;
    }
    print_info("Error message sent to %s sensor", nums);
    free(buf);
    return 0;
}

/**
* @brief Przyjmij wiadomość inicjalizującą o ostatniego czujnika.
* @param received_msg Struktura odebranej wiadomości
* @return 0 gdy się udało, lub inna liczba na błąd.
*/
static int take_init_msg(struct init_msg received_msg)
{
    print_success("Received initial message back");
    //TODO zmienia stan na jakiś inny i dopiero wtedy wysyła dane
    return 0;
}

/**
* @brief Przyjmij pakiet z danymi i wyświetl je.
* @param received_msg Struktura z danymi
* @return 0 jeśli się udało, lub błąd, gdy inna liczba.
*/
static int take_data_msg(struct data_msg received_msg)
{
    int data_count = received_msg.count;
    print_success("Received %d measurements:", data_count);
    int i;
    for(i = 0; i < data_count; ++i)
    {
        print_info("Sensor: %d Data: %d", received_msg.data[i].id, received_msg.data[i].data);
    }
    return 0;
}

// Receive ack message
int receive_ack_and_finit(int num)
{
    int sockfd = sock_first;
    struct sockaddr_in addr = first_addr;
    char* nums = "first";
    if(num == LAST)
    {
        sockfd = sock_last;
        addr = last_addr;
        nums = "last";
    }


    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &error_timeout, sizeof(struct timeval)) < 0)
    {
        print_error("Failed to set error timeout");
    }
    else
    {
        print_success("Error timeout set");
    }

    unsigned char buf[MAX_DATA];
    unsigned addr_len = sizeof(addr);
    print_info("Waiting for ack from %d...", num);
    int len = recvfrom(sockfd, buf, MAX_DATA, 0, (struct sockaddr*)&addr, &addr_len);
    union msg received_msg;
    int msg_type = unpack_msg(buf, &received_msg);
    if(len < 0 || msg_type != ACK_MSG)
    {
        print_error("Failed to receive ack msg from %s sensor", nums);
        print_info("Received %d type instead", msg_type);
        print_success("Closing socket");
        close(sockfd);
        return -1;
    }
    print_success("Received ack msg from %d", num);
    len = recvfrom(sockfd, buf, MAX_DATA, 0, (struct sockaddr*)&addr, &addr_len);
    msg_type = unpack_msg(buf, &received_msg);
    if(len < 0 || msg_type != FINIT_MSG)
    {
        print_error("Failed to receive finit msg from %s sensor", nums);
        print_info("Received %d type instead", msg_type);
        print_success("Closing socket");
        close(sockfd);
        return -1;
    }
    print_success("Received finit msg from %d", num);
    if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
    {
        print_error("Failed to return to normal timeout");
    }
    else
    {
        print_success("Returned to normal timeout");
    }
    return 0;
}

// Loop for communication with first sensor
void* first_loop(void* arg)
{
    print_success("Loop thread started");
    send_init_msg();
    while(1)
    {
        if(resolving_error)
        {
            usleep(SERVER_TIMEOUT*1000);
            continue;
        }
        send_data_msg(FIRST);
        if(mode == DOUBLE_LIST)
        {
            if(close_first)
                return NULL;
            print_info("Waiting for data from first...");
            unsigned char buf[MAX_DATA];
            unsigned first_addr_len = sizeof(first_addr);
            int len = recvfrom(sock_first, buf, MAX_DATA, 0, (struct sockaddr*)&first_addr, &first_addr_len);
            if(len < 0)
            {
                print_error("Second problem with network(not received msg from first sensor), terminating....");
                exit(-1);
            }
            union msg received_msg;
            int msg_type = unpack_msg(buf, &received_msg);
            if(msg_type == DATA_MSG)
                take_data_msg(received_msg.data);
            else
                print_warning("Received unknown bytes");
        }
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
        print_info("Waiting for data from last...");
        unsigned char buf[MAX_DATA];
        unsigned last_addr_len = sizeof(last_addr);
        int len = recvfrom(sock_last, buf, MAX_DATA, 0, (struct sockaddr*)&last_addr, &last_addr_len);
        if(len < 0)
        {
            mode = DOUBLE_LIST;
            resolving_error = 1;
            print_warning("Didn't receive data message from last sensor");
            print_success("Mode changed to double-list");
            if(send_error_msg(FIRST) < 0 || receive_ack_and_finit(FIRST) < 0)
            {
                close_first = 1;
            }
            if(send_error_msg(LAST) < 0 || receive_ack_and_finit(LAST) < 0)
            {
                resolving_error = 0;
                return;
            }
            resolving_error = 0;
        }
        else
        {
            union msg received_msg;
            int msg_type = unpack_msg(buf, &received_msg);
            switch(msg_type)
            {
            case INIT_MSG:
                take_init_msg(received_msg.init);
                break;
            case DATA_MSG:
                take_data_msg(received_msg.data);
                break;
            default:
                print_warning("Received unknown bytes");
            }
            cleanup_msg(&received_msg);
        }
        usleep(SERVER_TIMEOUT*1000);
        if(mode == DOUBLE_LIST)
        {
            print_info("Sending data to last sensor...");
            send_data_msg(LAST);
        }
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
    timeout.tv_usec = SERVER_TIMEOUT%1000;
    timeout.tv_sec = SERVER_TIMEOUT/1000;

    error_timeout.tv_usec = ERROR_TIMEOUT%1000;
    error_timeout.tv_sec = ERROR_TIMEOUT/1000;
    if(setsockopt(sock_first, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
    {
        print_error("Failed to setsockopt %s", strerror(errno));
    }
    if(setsockopt(sock_last, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) < 0)
    {
        print_error("Failed to setsockopt %s", strerror(errno));
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
    last_addr.sin_port = htons(port_last);

    struct hostent *hostinfo = gethostbyname(first_name);
    if(hostinfo == NULL)
    {
        print_error("Failed to gethostbyname, terminating...");
        exit(-1);
    }
    first_addr.sin_family = AF_INET;
    first_addr.sin_addr = *(struct in_addr *)hostinfo->h_addr;
    first_addr.sin_port = htons(port_first);
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

void print_usage()
{
    printf("Usage: server [OPTIONS]\n");
    printf("\t-f NAME:PORT\t\tName and port of a first sensor (localhost:4001 by default)\n");
    printf("\t-h\t\t\tPrint this message.\n");
    printf("\t-l PORT\t\t\tPort on which server will listen for data from a last sensor (4444 by default)\n");
    printf("\t-p PERIOD\t\tSensor's period in ms (1000 by default).\n");
    printf("\t-t TIMEOUT\t\tSensor's timeout in ms (1000 by default).\n");
}

int main(int argc, char *argv[])
{
    int option;
    print_init();

    // Przypisywanie wartości domyślnych
    port_first = DEF_PORT_FIRST;
    strncpy(first_name, "localhost", strlen("localhost"));
    port_last = DEF_PORT_LAST;
    sensor_timeout = DEF_SENSOR_TIMEOUT;
    sensor_period = DEF_SENSOR_PERIOD;

    while ((option = getopt(argc, argv,"f:hl:p:t:")) != -1) 
    {
        switch (option) 
        {
            case 'f':
            {
                unsigned int name_len;
                char* str = strstr(optarg, ":");
                if (str == NULL) 
                {
                    printf("Invalid argument for -f flag!\n");
                    print_usage();
                    exit(1);
                }

                name_len = str - optarg;
                if (name_len >= 1024)
                {
                    print_error("Firt sensor name too long!\n");
                    exit(1);
                }

                strncpy(first_name, optarg, name_len);
                first_name[name_len] = '\0';

                port_first = atoi(++str);
                break;
            }         
            case 'h':   
                print_usage();
                exit(0);
            case 'l': 
                port_last = atoi(optarg); 
                break;
            case 'p':  
                sensor_period = atoi(optarg);
                break;
            case 't':
                sensor_timeout = atoi(optarg);
                break;
            default: 
                print_usage(); 
                exit(1);
        }
    }

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
