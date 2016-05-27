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

/** Gniazdo poprzedniego czujnika. */
int socket_prev;
/** Gniazdo następnego czujnika. */
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

/**
* @brief Zainicjalizuj gniazda i podłącz się do nich.
* @return 0 gdy się udało, lub błąd.
*/
static int initilize_sockets();

/**
* @brief Pobierz ostatni pomiar.
* @return Ostatni pomiar czujnika.
*/
static int get_measurement();

/**
* @brief Wykonaj i zapisz pomiar. Może być on pobrany za pomocą get_measurement().
*/
static void measure();

static void print_data(struct data_msg* received_msg);

static void add_measurement(struct data_msg* base_msg);

static int send_msg(enum direction send_dir, union msg* msg);

static void sleep_action(int milliseconds);

static int get_actual_socket(enum direction send_dir);

static int wait_timeout_action(enum direction send_dir, int milliseconds);

static int read_msg(enum direction socket_dir, union msg* read_msg);


static void action();
