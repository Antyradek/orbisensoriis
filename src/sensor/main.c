/**
 * 04.05.2016
 * @author Radosław Świątkiewicz
 */
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../protocol.h"
#include "../helper.h"

int socket_prev;
int socket_next;

//paramentry następnika
const char* addr_next;
const char* port_next;
//paramentry poprzednika
const char* addr_prev;
const char* port_prev;

//wysyłające
struct sockaddr_in sockaddr_next;
struct sockaddr_in sockaddr_prev;

// Initializacja gniazd
/*
int initialize_sockets()
{
    socket_next = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_next < 0)
    {
        print_error("Failed to create next sensor socket");
        return -1;
    }
    socket_prev = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_prev < 0)
    {
        print_error("Failed to create previous sensor socket");
        return -1;
    }

	socket_in_from_next = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_in_from_next < 0)
    {
        print_error("Failed to create input socket from next sensor");
        return -1;
    }

	socket_in_from_prev = socket(AF_INET, SOCK_DGRAM, 0);
    if(socket_in_from_prev < 0)
    {
        print_error("Failed to create input socket from previous sensor");
        return -1;
    }
    return 0;
}*/

//initializacja adresów
int initilize_sockets()
{
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *rp;
	int addrinfo_out;
	int sfd;
	//zmodyfikowany kod z przykładu w „man 3 getaddrinfo”

	print_info("Now initializing passive socket to listen to previous sensor");
	memset(&hints, 0, sizeof(struct addrinfo));
	//pasywny, do niego się łączymy, będzie użyty do bind
	hints.ai_flags = AI_PASSIVE;
	//tylko ipv4
	hints.ai_family = AF_INET;
	//datagramowy
	hints.ai_socktype = SOCK_DGRAM;
	//dowolny protokół
	hints.ai_protocol = 0;
	//pozostałe są zwracane przez getaddrinfo
	hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
	/* Ustawiamy adresy
	NULL - gniazdo pasywne, nie łączymy się nim na żadny adres.
	port_in_from_next - nasz port na którym nasłuchujemy danych od następnego czujnika
	next_hints - opis naszego gniazda
	next_result - zwracana struktura z adresami
	*/
	addrinfo_out = getaddrinfo(NULL, port_prev, &hints, &result);
	if(addrinfo_out != 0)
	{
		print_error("Failed to getaddrinfo()");
		return addrinfo_out;
	}
	//mamy listę jednokierunkową kilku adresów, sprawdzamy po kolei
	for (rp = result; rp != NULL; rp = rp -> ai_next)
	{
		sfd = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);
		if (sfd == -1)
		{
			print_info("One address failed");
			continue;
		}
		if (bind(sfd, rp -> ai_addr, rp -> ai_addrlen) == 0)
		{
			break;
		}
		close(sfd);
	}
	if (rp == NULL)
	{
		print_error("Could not bind");
		return -1;
	}
	//cieknąca pamięć to zła rzecz
	freeaddrinfo(result);
	socket_prev = sfd;
	print_success("Socket to previous sensor succeeded!");

	print_info("Now initializing active socket to send data to next sensor");
	memset(&hints, 0, sizeof(struct addrinfo));
	//aktywny, nim łączymy się do następnika
	hints.ai_flags = 0;
	//tylko ipv4
	hints.ai_family = AF_INET;
	//datagramowy
	hints.ai_socktype = SOCK_DGRAM;
	//dowolny protokół
	hints.ai_protocol = 0;
	//pozostałe są zwracane przez getaddrinfo
	hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

	addrinfo_out = getaddrinfo(addr_next, port_next, &hints, &result);
	if(addrinfo_out != 0)
	{
		print_error("Failed to getaddrinfo()");
		return addrinfo_out;
	}
	for (rp = result; rp != NULL; rp = rp -> ai_next)
	{
		sfd = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);
		if (sfd == -1)
		{
			print_info("One address failed");
			continue;
		}
		if (connect(sfd, rp -> ai_addr, rp -> ai_addrlen) == 0)
		{
			break;
		}
		close(sfd);
	}
	if (rp == NULL)
	{
		print_error("Could not connect");
		return -1;
	}
	socket_next = sfd;
	freeaddrinfo(result);
	print_success("Socket to next sensor succeeded!");

	return 0;

}

int main(int argc, char const *argv[])
{
	if(argc != 5)
	{
		printf("Użycie: [adres_poprzednika] [port_poprzednika] [adres_następnika] [port_następnika] \n");
		return 0;
	}
	addr_prev = argv[1];
	port_prev = argv[2];

	addr_next = argv[3];
	port_next = argv[4];

	initilize_sockets();

	//TODO inicjalizacja aktywnych adresów do wysyłania

}
