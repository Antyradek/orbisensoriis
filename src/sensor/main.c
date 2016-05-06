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

#define BUF_SIZE 512

int socket_prev;
int socket_next;

//paramentry następnika
const char* addr_next;
const char* port_next;
//paramentry poprzednika
const char* addr_prev;
const char* port_prev;

uint16_t sensor_id;
int timeout;
int period;

/**
  * @brief Initialize, connect and bind necessary sockets.
  * @return Error code, or 0.
*/
int initilize_sockets()
{
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *rp;
	int addrinfo_out;
	int sfd;
	//zmodyfikowany kod z przykładu w „man 3 getaddrinfo”

	print_info("Now initializing passive socket to listen to previous sensor...");
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
			print_info("One address failed.");
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
		print_error("Could not bind.");
		return -1;
	}
	//cieknąca pamięć to zła rzecz
	freeaddrinfo(result);
	socket_prev = sfd;
	print_success("Socket to previous sensor succeeded!");

	print_info("Now initializing active socket to send data to next sensor...");
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
			print_info("One address failed.");
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
		print_error("Could not connect.");
		return -1;
	}
	socket_next = sfd;
	freeaddrinfo(result);
	print_success("Socket to next sensor succeeded!");

	return 0;
}

/**
  * @brief Take initial message, set parameters and send to the next node.
  * @param timeout How long awake.
  * @param period How long sleep.
  * @return 0 on success
*/
int take_init_msg(struct init_msg received_msg)
{
	timeout = received_msg.timeout;
	period = received_msg.period;
	print_success("Received Initial Message with timeout %d and period %d", timeout, period);

	//wysyłanie wiadomości do kolejnego czujnika
	union msg msg;
    msg.init.type = INIT_MSG;
    msg.init.timeout = timeout;
    msg.init.period = period;
    int buf_len = sizeof(msg.init);
    unsigned char buf[buf_len];
	int packed_msg_size = pack_msg(&msg, buf, buf_len);
    if(packed_msg_size < 0)
    {
        print_error("Failed to pack init msg");
        return -1;
    }
    if(write(socket_next, buf, packed_msg_size) != packed_msg_size)
    {
        print_error("Failed to send init msg");
        return -2;
    }
    print_success("Sent Initial Message to next sensor");
	return 0;
}

/**
  * @brief Get last measurement.
  * @return The last measurement of the sensor
*/
int get_measurement()
{
	//miernik mierzy zaraz po obudzeniu się, a nie teraz
	//testowo podajemy losową liczbę.
	return rand() % 10000;
}

int take_data_msg(struct data_msg received_msg)
{
	int old_data_count = received_msg.count;

	print_success("Received %d measurements:", old_data_count);
	int i;
	for(i = 0; i < old_data_count; ++i)
	{
		print_info("Sensor: %d Data: %d", received_msg.data[i].id, received_msg.data[i].data);
	}

	int measure_count = old_data_count + 1;
	uint32_t new_measure = get_measurement();

	//rezerwacja pamięci
	struct data_t all_data[measure_count * sizeof(struct data_t)];
	//kopiowanie starych wyników
	memcpy(all_data, received_msg.data, old_data_count * sizeof(struct data_t));
	//dodanie nowego wyniku
	all_data[old_data_count].id = sensor_id;
	all_data[old_data_count].data = new_measure;

	//wysyłanie wiadomości do kolejnego czujnika
	union msg msg;
    msg.data.type = DATA_MSG;
    msg.data.count = measure_count;
    msg.data.data = all_data;
    int buf_len = sizeof(msg.data) + sizeof(all_data);
    unsigned char buf[buf_len];
	int packed_msg_size = pack_msg(&msg, buf, buf_len);
    if(packed_msg_size < 0)
    {
        print_error("Failed to pack data msg");
        return -1;
    }
    if(write(socket_next, buf, packed_msg_size) != packed_msg_size)
    {
        print_error("Failed to send data msg");
        return -2;
    }
    print_success("Sent Data Message to next sensor with new measurement %d", new_measure);
	return 0;


}

int main(int argc, char const *argv[])
{
	if(argc != 6)
	{
		printf("Użycie: [adres_poprzednika] [port_poprzednika] [adres_następnika] [port_następnika] [id_czujnika]\n");
		return 0;
	}
	addr_prev = argv[1];
	port_prev = argv[2];
	addr_next = argv[3];
	port_next = argv[4];
	sensor_id = (uint16_t)atoi(argv[5]);
	initilize_sockets();
	srand(sensor_id);

	print_success("Sensor %d is now ready for work.", sensor_id);

	//główna pętla
	struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    unsigned char buf[BUF_SIZE];
	int s;
	while(1)
	{
		//odebranie pakietu
		peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recvfrom(socket_prev, buf, BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (nread == -1)
		{
			print_info("Failed receive request.");
			continue;
		}
		//sprawdzenie nadawcy pakietu
       char host[NI_MAXHOST], service[NI_MAXSERV];
       s = getnameinfo((struct sockaddr *) &peer_addr, peer_addr_len, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
       if (s == 0)
	   {
		   //rozpakowanie wiadomości
		   union msg received_msg;
		   int msg_type = unpack_msg(buf, &received_msg);
		   //akcja w zależności od wiadomości
		   switch(msg_type)
		   {
			   case INIT_MSG:
			   		take_init_msg(received_msg.init);
					break;
				case DATA_MSG:
					take_data_msg(received_msg.data);
					break;
				//TODO case pozostałe:
				default:
					print_warning("Received unknown %ld bytes from %s:%s: %s\n", (long int) nread, host, service, buf);
		   }
	   }
    	else
		{
			print_error("getnameinfo() failed");
		}
	}

}
