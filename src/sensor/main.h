/**
* @brief Obiekt czujnika.
* @author Radosław Świątkiewicz
*/
#pragma once

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../protocol.h"
#include "../helper.h"

#define BUF_SIZE 512
#define UNEXPECTED_MESSAGE -3

//gniazda
int socket_prev;
int socket_next;
//paramentry następnika
const char* addr_next;
const char* port_next;
//paramentry poprzednika
const char* addr_prev;
const char* port_prev;
//numer czujnika
uint16_t sensor_id;
//wartości do cyklu sennego
int timeout;
int period;
//ostatni pomiar
int last_measurement;

//tryb pracy czujnika
enum sensor_state
{
    //czujnik oczekuje na wiadomość inicjalizującą
    INITIALIZING,
    //rormalny tryb pracy
    NORMAL,
    //awaryjny 1 - za przerwaniem
    EMERGENCY_1,
    //awaryjny 2 - przed przerwaniem
    EMERGENCY_2,
    //czujnik rozpoczyna wiadomość z danymi
    STARTING_DATA
};

enum direction
{
    //czujnik na przedzie
    NEXT,
    //czujnik z tyłu
    PREV
};

enum sensor_state state;

//czy czujnik jest obrócony i ma zamienione sąsiednie czujniki
int rotated180;


static void action(int milliseconds);
