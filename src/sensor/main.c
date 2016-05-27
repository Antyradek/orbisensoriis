#include "main.h"

static int initilize_sockets()
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
    * NULL - gniazdo pasywne, nie łączymy się nim na żadny adres.
    * port_in_from_next - nasz port na którym nasłuchujemy danych od następnego czujnika
    * next_hints - opis naszego gniazda
    * next_result - zwracana struktura z adresami
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

static int get_measurement()
{
    return last_measurement;
}

static void measure()
{
    //wykonuje teraz bardzo skomplikowane pomiary wartości szczęścia we wszechświecie...
    last_measurement = rand() % 10000;
}

static void print_data(struct data_msg* received_msg)
{
    int old_data_count = received_msg -> count;

    print_success("Received %d measurements:", old_data_count);
    int i;
    for(i = 0; i < old_data_count; ++i)
    {
        print_info("Sensor: %d Data: %d", received_msg -> data[i].id, received_msg -> data[i].data);
    }
}

static void add_measurement(struct data_msg* base_msg)
{
    int measure_count = base_msg -> count;
    int new_measure_count = measure_count + 1;
    uint32_t new_measure = get_measurement();

    //rezerwacja pamięci
    struct data_t* all_data = malloc(new_measure_count * sizeof(struct data_t));
    //kopiowanie starych wyników
    memcpy(all_data, base_msg -> data, measure_count * sizeof(struct data_t));
    //dodanie nowego wyniku
    all_data[measure_count].id = sensor_id;
    all_data[measure_count].data = new_measure;
    //usuwanie starych danych
    free(base_msg -> data);
    //zastąpienie ich nowymi
    base_msg -> data = all_data;
    base_msg -> count = new_measure_count;
}

static int send_msg(enum direction send_dir, union msg* msg)
{
    int extra_size = 0;
    if(msg -> type == DATA_MSG)
    {
        extra_size = msg -> data.count * sizeof(struct data_t);
    }
    int buf_len = sizeof(msg) + extra_size;

    unsigned char buf[buf_len];
    int packed_msg_size = pack_msg(msg, buf, buf_len);
    if(packed_msg_size < 0)
    {
        print_error("Failed to pack data msg");
        return -1;
    }
    if(write(get_actual_socket(send_dir), buf, packed_msg_size) != packed_msg_size)
    {
        print_error("Failed to send data msg");
        return -2;
    }
    return 0;
}


/**
* @brief Wyślij pakiet ze swoją daną do następnika
* @return 0 jeśli się udało, lub błąd, gdy inna liczba.
*/
static int init_data_msg()
{
    uint32_t new_measure = get_measurement();

    //rezerwacja pamięci
    struct data_t all_data[sizeof(struct data_t)];
    all_data[0].id = sensor_id;
    all_data[0].data = new_measure;

    //wysyłanie wiadomości do kolejnego czujnika
    union msg msg;
    msg.data.type = DATA_MSG;
    msg.data.count = 1;
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
    print_success("Sent Data Message to next sensor with first measurement %d", new_measure);
    return 0;
}

/**
* @brief Przyjmij informację o błędzie i wykonaj odpowiednią akcję w zależności od stanu
* @return 0 jeśli się udało, lub błąd, gdy inna liczba.
*/
static int take_error_msg()
{
    //ta wiadomość powinna przyjść tylko w trybie normalnym
    if(state != NORMAL)
    {
        print_warning("Didn't expect ERR_MSG now, ignoring");
        return UNEXPECTED_MESSAGE;
    }
    state = EMERGENCY_2;
    print_warning("Received ERR_MSG, switching mode to Emergency 2.");
    return 0;
}

/**
* @brief Przyjmij pakiet o rekonfiguracji i wykonaj odpowiednią akcję w zależności od stanu
* @return 0 jeśli się udało, lub błąd, gdy inna liczba.
*/
static int take_reconf_msg()
{
    //przejście w rtyb konfiguracj, tylko w stanie normalnym
    if(state != NORMAL)
    {
        print_warning("Didn't expect RECONF_MSG now, ignoring");
        return UNEXPECTED_MESSAGE;
    }
    state = INITIALIZING;
    print_info("Received RECONF_MSG, resetting and waiting for initial message.");
    return 0;
}

static void sleep_action(int milliseconds)
{
    print_info("Now going to sleep.");
    usleep(milliseconds * 1000);
}

static int get_actual_socket(enum direction send_dir)
{
    if(!rotated180)
    {
        if(send_dir == NEXT)
        {
            return socket_next;
        }
        else if(send_dir == PREV)
        {
            return socket_prev;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        if(send_dir == NEXT)
        {
            return socket_prev;
        }
        else if(send_dir == PREV)
        {
            return socket_next;
        }
        else
        {
            return 0;
        }
    }
}

static int wait_timeout_action(enum direction send_dir, int milliseconds)
{
    //struktura do funkcji select
    fd_set select_sockets;
    //struktura od czasu
    struct timeval socket_timeout;
    //dodanie naszego deskryptora do gniazda
    FD_ZERO(&select_sockets);
    FD_SET(get_actual_socket(send_dir), &select_sockets);
    //ustawienie czasu
    socket_timeout.tv_usec = milliseconds * 1000;
    //tutaj czekamy na pakiet
    int no_timeout = select(get_actual_socket(send_dir) + 1, &select_sockets, NULL, NULL, &socket_timeout);
    return no_timeout;
}

static int read_msg(enum direction socket_dir, union msg* read_msg)
{
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    ssize_t nread;
    unsigned char buf[BUF_SIZE];
    int s;
    peer_addr_len = sizeof(struct sockaddr_storage);
    nread = recvfrom(get_actual_socket(socket_dir), buf, BUF_SIZE, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
    if (nread == -1)
    {
        print_info("Failed receive request.");
        return -1;
    }
    //sprawdzenie nadawcy pakietu
    char host[NI_MAXHOST], service[NI_MAXSERV];
    s = getnameinfo((struct sockaddr *) &peer_addr, peer_addr_len, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);
    if (s != 0)
    {
        print_error("getnameinfo() failed.");
        return -2;
    }
    //UWAGA!!! Trzeba sprzątnąć po tej funkcji poniżej.
    int msg_type = unpack_msg(buf, read_msg);
    return msg_type;
}

static void action()
{
    while(1)
    {
        if(state == INITIALIZING)
        {
            print_success("Sensor %d now waiting for initialization.", sensor_id);
            union msg received_msg;
            int msg_id = read_msg(PREV, &received_msg);
            if(msg_id == INIT_MSG)
            {
                timeout = received_msg.init.timeout;
                period = received_msg.init.period;
                print_success("Received Initial Message with timeout %d and period %d", timeout, period);
                if(send_msg(NEXT, &received_msg) == 0)
                {
                    print_success("Sent Initial message to next sensor");
                    state = NORMAL;
                }
            }
            else
            {
                print_warning("Didn't expect initial message now. Ignoring.");
            }
            cleanup_msg(&received_msg);
        }
        else if(state == NORMAL)
        {
            sleep_action(period);
            measure();
            if(wait_timeout_action(PREV, timeout))
            {
                union msg received_msg;
                int msg_id = read_msg(PREV, &received_msg);
                if(msg_id == DATA_MSG)
                {
                    print_data(&received_msg.data);
                    add_measurement(&received_msg.data);
                    if(send_msg(NEXT, &received_msg) == 0)
                    {
                        print_success("Sent Data Message to next sensor with new measurement %d", get_measurement());
                    }
                }
                //TODO else
                cleanup_msg(&received_msg);
            }
            else
            {
                print_warning("Timeout! Switching mode to Emergency 1.");
                state = EMERGENCY_1;
                continue;
            }
        }

    }
}

int main(int argc, char const *argv[])
{
    if(argc != 6)
    {
        printf("Usage: [prev_addr] [prev_port] [next_addr] [next_port] [sensor_id]\n");
        return 0;
    }
    addr_prev = argv[1];
    port_prev = argv[2];
    addr_next = argv[3];
    port_next = argv[4];
    sensor_id = (uint16_t)atoi(argv[5]);

    state = INITIALIZING;
    rotated180 = 0;

    initilize_sockets();
    srand(sensor_id);

    action();
}
