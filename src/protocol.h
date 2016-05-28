/**
 * protocol.h
 * Paweł Szewczyk
 * 2016-04-26
 */

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

/**
 * Określa typ wiadomości
 */
enum msg_type {
    INIT_MSG = 1,
    DATA_MSG = 2,
    ERR_MSG = 3,
    ACK_MSG = 4,
    FINIT_MSG = 5,
    RECONF_MSG = 6,
};

/**
 * Wiadomość inicjująca
 */
struct init_msg {
    /** typ */
    uint8_t type;
    /** czas nieaktywności czujników */
    uint16_t timeout;
    /** maksymalny czas aktywności czujników */
    uint16_t period;
    /** ilość czujników w sieci */
    uint8_t count;
};

/**
 * Dane z czujnika
 */
struct data_t {
    /** id czujnika */
    uint16_t id;
    /** Dane z czujnika */
    uint32_t data;
};

/** docelowy rozmiar pakietu z danymi pojedynczego czujnika */
#define DATA_T_SIZE 6

/**
 * Wiadomość z danymi czujników
 */
struct data_msg {
    /** Typ wiadomości */
    uint8_t type;
    /** Liczba pakietów z danymi */
    uint16_t count;
    /** Lista pakietów z danymi */
    struct data_t *data;
};

/**
 * Wiadomość informacyjna
 */
struct info_msg {
    /** Typ wiadomości */
    uint8_t type;
};

/**
 * Pomocniczy typ reprezentujący dowolną wiadomość
 */
union msg {
    uint8_t type;
    struct info_msg info;
    struct data_msg data;
    struct init_msg init;
};

/**
 * @brief pakuje podaną wiadomość
 * @param[in] msg Wskaźnik na wiadomość do spakowania
 * @param[out] dst Bufor wynikowy
 * @param[in] n Rozmiar bufora dst
 * @return 0 lub ujemny kod błędu w przypadku błędu
 */
int pack_msg(void *msg, unsigned char *dst, int n);

/**
 * @brief rozpakowuje spakowaną wiadomość
 * @param[in] src Bufor zawierający spakowaną wiadomość
 * @param[out] dst Struktura wynikowa
 * @return 0 lub ujemny kod błędu w przypadku błędu
 */
int unpack_msg(unsigned char *buf, union msg *dst);

/**
 * @brief Zwalnia dodatkową pamięć zaalokowaną dla wiadomości
 * @detail Zwalnia pamięć zaalokowaną przez bibliotekę. Nie wywołuje free na podanym wskaźniku. 
 * @param[in] msg Wiadomość do zniszczenia
 */
void cleanup_msg(union msg *msg);

#endif
