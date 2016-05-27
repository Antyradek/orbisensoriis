#include "main.h"

static int initilize_sockets()
{
    struct addrinfo hints;
    struct addrinfo *result;
    struct addrinfo *rp;
    int addrinfo_out;
    int sfd;
    //zmodyfikowany kod z przykładu w „man 3 getaddrinfo”

    print_info("Now initializing socket to listen to previous sensor.");
    memset(&hints, 0, sizeof(struct addrinfo));
    //poprzedni, będzie użyty do bind
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
    * addr_prev - adres poprzednika.
    * port_prev - nasz port na którym nasłuchujemy danych od następnego czujnika
    * next_hints - opis naszego gniazda
    * next_result - zwracana struktura z adresami
    */
    addrinfo_out = getaddrinfo(addr_prev, port_prev, &hints, &result);
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

    print_info("Now initializing socket to send data to next sensor.");
    memset(&hints, 0, sizeof(struct addrinfo));
    //nim łączymy się do następnika
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
    int send_socket = get_actual_socket(send_dir);
    //pakowanie
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
        print_error("Failed to pack data msg.");
        return -1;
    }
    //wysyłanie
    if(send_socket == socket_next)
    {
        //wysyłamy do następnika, to gniazdo powstało za pomocą connect, czyli używamy write
        if(write(socket_next, buf, packed_msg_size) != packed_msg_size)
        {
            print_error("Failed to send message.");
            return -2;
        }
        return 0;
    }
    else
    {
        //wysyłamy do poprzednika, a to dniazdo powstało za pomocą bind, zatem używamy send
        if (send(socket_next, buf, packed_msg_size, 0) != packed_msg_size)
        {
            print_error("Failed to send message.");
            return -2;
        }
        return 0;
    }

}

static void sleep_action(int milliseconds)
{
    print_info("Now going to sleep for %d ms...", milliseconds);
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
    socket_timeout.tv_sec = 0;
    socket_timeout.tv_usec = milliseconds * 1000;
    //tutaj czekamy na pakiet
    int no_timeout = select(get_actual_socket(send_dir) + 1, &select_sockets, NULL, NULL, &socket_timeout);
    return no_timeout;
}

static int read_msg(enum direction socket_dir, union msg* read_msg)
{
    int read_socket = get_actual_socket(socket_dir);
    ssize_t nread;
    unsigned char buf[BUF_SIZE];
    if(read_socket == socket_next)
    {
        //gniazdo connect, używamy write
        nread = read(socket_next, buf, BUF_SIZE);
        if (nread == -1)
        {
            print_info("Failed to read data.");
            return -1;
        }
    }
    else
    {
        //gniazdo bind, używamy recvfrom
        nread = recv(socket_prev, buf, BUF_SIZE, 0);
        if (nread == -1)
        {
            print_info("Failed to receive message.");
            return -1;
        }
    }

    //UWAGA!!! Trzeba sprzątnąć po tej funkcji poniżej.
    int msg_type = unpack_msg(buf, read_msg);
    return msg_type;
}

static void emergency1()
{
    print_info("Waiting for ERR_MSG from next sensor...");
    union msg msg;
    //czekamy na ERR_MSG
    while(1)
    {
        int msg_id = read_msg(NEXT, &msg);
        if(msg_id != ERR_MSG)
        {
            if(msg_id > 0) print_warning("Unexpected message, expected ERR_MSG. Ignoring.");
            else print_error("Error receiving message, retrying.");
            cleanup_msg(&msg);
            continue;
        }
        print_success("Received ERR_MSG from next sensor.");
        break;
    }
    //wysyłamy ACK_MSG do następnika
    msg.type = ACK_MSG;
    while(send_msg(NEXT, &msg) != 0);
    print_success("Sent ACK_MSG to next sensor.");
    //wysyłamy ERR_MSG do poprzednika
    msg.type = ERR_MSG;
    while(send_msg(PREV, &msg) != 0);
    print_success("Sent ERR_MSG to prevous sensor.");
    //czekamy na ACK_MSG od poprzednika
    print_info("Waiting for ACK_MSG from previous sensor...");
    while(1)
    {
        if(wait_timeout_action(PREV, NEIGHBOUR_TIMEOUT))
        {
            //odebraliśmy
            int msg_id = read_msg(PREV, &msg);
            if(msg_id != ACK_MSG)
            {
                if(msg_id > 0) print_warning("Unexpected message, expected ACK_MSG. Ignoring.");
                else print_error("Error receiving message, retrying.");
                //możliwy pakiet z danymi, warto posprzątać
                cleanup_msg(&msg);
                continue;
            }
            print_success("Received ACK_MSG from previous sensor.");
            //czekamy na FINIT od poprzednika
            print_info("Waiting for FINIT_MSG from previous sensor...");
            while(1)
            {
                msg_id = read_msg(PREV, &msg);
                if(msg_id != FINIT_MSG)
                {
                    if(msg_id > 0) print_warning("Unexpected message, expected FINIT_MSG. Ignoring.");
                    else print_error("Error receiving message, retrying.");
                    cleanup_msg(&msg);
                    continue;
                }
                print_success("Received FINIT_MSG from previous sensor.");
                //wysyłamy FINIT_MSG do następnika
                msg.type = FINIT_MSG;
                while(send_msg(NEXT, &msg) != 0);
                print_success("Sent FINIT_MSG to next sensor.");
                print_info("Switching back to normal mode.");
                state = NORMAL;
                return;
            }
        }
        else
        {
            print_warning("Timeout! Sending FINIT_MSG to next sensor.");
            //był timeout
            //wysyłamy FINIT_MSG do następnika
            msg.type = FINIT_MSG;
            while(send_msg(NEXT, &msg) != 0);
            print_success("Sent FINIT_MSG to next sensor.");
            print_info("Switching to data initialization mode.");
            state = STARTING_DATA;
            return;
        }
    }
}

static void emergency2()
{
    //ACK_MSG do poprzednika
    union msg msg;
    msg.type = ACK_MSG;
    while(send_msg(PREV, &msg) != 0);
    print_success("Sent ACK_MSG to previous sensor.");
    //ERR_MSG dalej do następnika
    msg.type = ERR_MSG;
    while(send_msg(NEXT, &msg) != 0);
    print_success("Sent ERR_MSG to next sensor.");
    //czekamy na ACK_MSG od następnika
    print_info("Waiting for ACK_MSG from next sensor...");
    while(1)
    {
        if(wait_timeout_action(NEXT, NEIGHBOUR_TIMEOUT))
        {
            //odebraliśmy
            int msg_id = read_msg(NEXT, &msg);
            if(msg_id != ACK_MSG)
            {
                if(msg_id > 0) print_warning("Unexpected message, expected ACK_MSG. Ignoring.");
                else print_error("Error receiving message, retrying.");
                //możliwy pakiet z danymi, warto posprzątać
                cleanup_msg(&msg);
                continue;
            }
            print_success("Received ACK_MSG from next sensor.");
            //czekamy na FINIT_MSG od następnika
            print_info("Waiting for FINIT_MSG from next sensor...");
            while(1)
            {
                msg_id = read_msg(NEXT, &msg);
                if(msg_id != FINIT_MSG)
                {
                    if(msg_id > 0) print_warning("Unexpected message, expected FINIT_MSG. Ignoring.");
                    else print_error("Error receiving message, retrying.");
                    cleanup_msg(&msg);
                    continue;
                }
                print_success("Received FINIT_MSG from next sensor.");
                print_info("Rotating sensor 180°.");
                //wysyłamy FINIT_MSG do nowego następnika
                msg.type = FINIT_MSG;
                while(send_msg(NEXT, &msg) != 0);
                print_success("Sent FINIT_MSG to next sensor.");
                print_info("Switching back to normal mode");
                state = NORMAL;
                return;
            }
        }
        else
        {
            //był timeout
            print_warning("Timeout! Rotating sensor 180°.");
            //obracamy czujnik
            rotate180();
            //wysyłamy FINIT_MSG do nowego następnika
            msg.type = FINIT_MSG;
            while(send_msg(NEXT, &msg) != 0);
            print_success("Sent FINIT_MSG to next sensor.");
            print_info("Switching to data initialization mode.");
            state = STARTING_DATA;
            return;
        }
    }
}

static void rotate180()
{
    rotated180 = !rotated180;
}

static void action()
{
    while(1)
    {
        if(state == INITIALIZING)
        {
            print_success("Sensor %d now waiting for initialization...", sensor_id);
            union msg received_msg;
            int msg_id = read_msg(PREV, &received_msg);
            if(msg_id == INIT_MSG)
            {
                timeout = received_msg.init.timeout;
                period = received_msg.init.period;
                print_success("Received INIT_MSG with timeout %d and period %d.", timeout, period);
                if(send_msg(NEXT, &received_msg) == 0)
                {
                    print_success("Sent INIT_MSG to next sensor");
                    state = NORMAL;
                }
                else
                {
                    print_error("Failed to send INIT_MSG to next sensor. Retrying.");
                }
            }
            else
            {
                print_warning("Unexpected message, expected INIT_MSG. Ignoring.");
            }
            cleanup_msg(&received_msg);
        }
        else if(state == NORMAL)
        {
            sleep_action(period);
            measure();
            print_info("Waiting for DATA_MSG from previous sensor or timeout...");
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
                        print_success("Sent DATA_MSG to next sensor with new measurement %d.", get_measurement());
                    }
                    else
                    {
                        print_error("Failed to send DATA_MSG to next sensor.");
                    }
                    cleanup_msg(&received_msg);
                }
                else if(msg_id == ERR_MSG)
                {
                    print_warning("Received ERR_MSG. Executing Emergency 2 actions.");
                    emergency2();
                    continue;
                }
                else if(msg_id == RECONF_MSG)
                {
                    print_info("Switched to Reconfigation mode.");
                    state = INITIALIZING;
                    continue;
                }
                else
                {
                    print_warning("Unexpected message, expected DATA_MSG, ERR_MSG or RECONF_MSG. Ignoring.");
                }
            }
            else
            {
                print_warning("Timeout! Executing Emergency 1 actions.");
                emergency1();
                continue;
            }
        }
        else if(state == STARTING_DATA)
        {
            sleep_action(period);
            measure();
            union msg msg;
            msg.type = DATA_MSG;
            msg.data.count = 0;
            msg.data.type = DATA_MSG;
            msg.data.data = NULL;
            add_measurement(&msg.data);
            if(send_msg(NEXT, &msg) == 0)
            {
                print_success("Initialized and sent DATA_MSG to next sensor with measurement %d.", get_measurement());
            }
            else
            {
                print_error("Failed to send DATA_MSG to next sensor.");
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

    print_init();
    initilize_sockets();
    srand(sensor_id);

    action();
}
