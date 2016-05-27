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

/** Bufor pakietu wiadomości */
#define BUF_SIZE 512
/** Maksymalne oczekiwanie na sąsiadujący czujnik */
#define NEIGHBOUR_TIMEOUT 500

/** Gniazdo poprzedniego czujnika. */
int socket_prev;
/** Gniazdo następnego czujnika. */
int socket_next;
/** Adres następnika. */
const char* addr_next;
/** Port następnika. */
const char* port_next;
/** Adres poprzednika. */
const char* addr_prev;
/** Port poprzednika. */
const char* port_prev;
/** Identyfikator czujnika. */
uint16_t sensor_id;
/** Maksymalny czas oczekiwania na pakiet. */
int timeout;
/** Czas na jaki czujnik zasypia. */
int period;
/** Ostatni pomiar. */
int last_measurement;

struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;

/** Tryb pracy czujnika. */
enum sensor_state
{
    /** Czujnik oczekuje na wiadomość inicjalizującą. */
    INITIALIZING,
    /** Normalny tryb pracy. */
    NORMAL,
    /** Czujnik rozpoczyna wiadomość z danymi. */
    STARTING_DATA
};

/** Kierunek wysyłania/odbierania wiadomości. */
enum direction
{
    /** Czujnik z przodu */
    NEXT,
    /** Czujnik z tyłu */
    PREV,
};

/** Główny stan i tryb pracy czujnika */
enum sensor_state state;

/** Boolean - czy czujnik jest obrócony i ma zamieniony przód z tyłem */
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
* @brief Wykonaj i zapisz pomiar. Może być on potem pobrany za pomocą get_measurement().
*/
static void measure();

/**
* @brief Wypisz dane zawarte w pakiecie z danymi.
* @param received_msg Pakiet z danymi do wypisania z niego danych.
*/
static void print_data(struct data_msg* received_msg);

/**
* @brief Dodaj nowy pomiar do podanego pakietu.
* @param received_msg Pakiet z danymi do którego dopisujemy na koniec nowy pomiar.
*/
static void add_measurement(struct data_msg* base_msg);

/**
* @brief Wyślij pakiet w podanym kierunku.
* @param send_dir Kierunek do którego wysyłamy pakiet.
* @param msg Pakiet który wysyłamy.
* @return 0 gdy się udało, lub kod błędu.
*/
static int send_msg(enum direction send_dir, union msg* msg);

/**
* @brief Zawieś wątek na określoną ilość milisekund.
* @param milliseconds Czas na jaki ma zasnąć wątek.
*/
static void sleep_action(int milliseconds);

/**
* @brief Pobierz numer gniazda w zależności od kierunku w którym chcemy wysłać i tego, czy czujnik jest obrócony.
* @param send_dir Kierunek do którego wysyłamy pakiet.
* @return Identyfikator gniazda.
*/
static int get_actual_socket(enum direction send_dir);

/**
* @brief Czekaj na dane do pobrania z danego kierunku, ale tylko określoną ilość czasu.
* @param send_dir Kierunek z którego czekamy na pakiet.
* @param milliseconds Maksymalny czas, jaki czekamy na dane w milisekundach.
* @return 0 gdy dane przyszły i czekają na odczyt, lub inna liczba gdy czas minął wcześniej.
*/
static int wait_timeout_action(enum direction send_dir, int milliseconds);

/**
* @brief Czytaj pakiet z określonego kierunku.
* @param send_dir Kierunek z którego czytamy na pakiet.
* @param read_msg Wskaźnik do którego wsadzimy odebrany pakiet. Po użyciu należy zwolnić pamięć za pomocą cleanup_msg().
* @return 0 gdy dane udało się odczytać, lub inna liczba gdy nie.
*/
static int read_msg(enum direction socket_dir, union msg* read_msg);

/**
* @brief Wykonaj wszystkie akcje 1 trybu awaryjnego.
*/
static void emergency1();

/**
* @brief Wykonaj wszystkie akcje 2 trybu awaryjnego.
*/
static void emergency2();

/**
* @brief Obróć czujnik o 180° zamieniając poprzednika z następnikiem.
*/
static void rotate180();

/**
* @brief Wykonuj główną akcję w pętli.
*/
static void action();
