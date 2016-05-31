#!/usr/bin/env python3
# TIN - orbisenoriis
# Testy
# Autor: Mateusz Blicharski

import getopt
import os
import socket
from subprocess import Popen, PIPE, DEVNULL
import sys
import time

# Słownik z danymi pobranymi z pliku konfiguracyjnego
config = {}


def setup_sensors(sensors_no, start_port, last_port):
    """
    Tworzy senros_no czujników zaczynając od start_port i kończąc na last_port
    :param sensors_no:
    :param start_port:
    :param last_port:
    :return: lista uchytów na czujniki
    """
    try:
        sensor_path = config['SENSOR_PATH']
    except KeyError:
        print('[-] Path to a sensor binary missing! (SENSOR_PATH in conf file)')
        sys.exit(1)

    print('[*] Spawning ' + str(sensors_no) + ' sensors.')

    sensors = []
    for i in range(0, sensors_no - 1):
        sensors.append(Popen([sensor_path, 'localhost', str(start_port + i), 'localhost', str(start_port + 1 + i),
                              str(i)], stdin=DEVNULL, stdout=PIPE, stderr=DEVNULL))

    sensors.append(Popen([sensor_path, 'localhost', str(start_port + sensors_no - 1),
                          'localhost', str(last_port), str(sensors_no - 1)], stdin=DEVNULL, stdout=PIPE, stderr=DEVNULL))

    return sensors


def setup_server(first_port, last_port):
    """
    Uruchamia serwer z odpowiednim first_port i last_port
    :param first_port: port wejściowy pierwszego czujnika
    :param last_port: port wyjściowy ostatniego czujnika
    :return: uchwyt na serwer
    """
    try:
        server_path = config['SERVER_PATH']
    except KeyError:
        print('[-] Path to a server binary missing! (SERVER_PATH in conf file)')
        sys.exit(1)

    print('[*] Starting server.')

    server = Popen([server_path, '-f', 'localhost:' + str(first_port), '-l', str(last_port)],
                   stdin=DEVNULL, stdout=PIPE, stderr=DEVNULL)

    return server


def test_normal():
    print('[*] Normal work test.')

    try:
        sensors_no = int(config['SENSORS_NO'])
    except KeyError:
        print('[-] Number of sensors missing! (pass it by SENSORS_NO in conf file or -n option)')
        sys.exit(1)

    sensors = setup_sensors(sensors_no, 4001, 4444)
    server = setup_server(4001, 4444)

    for test_no in range(1, 11):
        print('[*] Test: ' + str(test_no))
        measurements = {}
        for i in range(0, sensors_no):
            line = sensors[i].stdout.readline()
            while b'measurement ' not in line:
                line = sensors[i].stdout.readline()
            measurements[i] = int(line.split(b' ')[-1][:-2])

        line = server.stdout.readline()
        while not (b'Received' in line and b'measurements' in line):
            line = server.stdout.readline()

        received_no = int(line.split(b' ')[2])
        if received_no != sensors_no:
            print('\033[-] Failed: server received ' + str(received_no) + ' measurements, ' +
                  str(sensors_no) + ' expected.\033[39m')
            continue

        received = {}
        for i in range(0, received_no):
            line = server.stdout.readline()
            tokens = line.split(b' ')
            sensor_no = int(tokens[2])
            received[sensor_no] = int(tokens[4])

        if received == measurements:
            print('\033[32m[+] Success!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))
        else:
            print('\033[31m[-] Failed!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))

    for p in sensors:
        p.kill()
    server.kill()


def test_emergency():
    print('[*] Emergency mode test.')

    try:
        sensors_no = int(config['SENSORS_NO'])
    except KeyError:
        print('[-] Number of sensors missing! (pass it by SENSORS_NO in conf file or -n option)')
        sys.exit(1)

    sensors = setup_sensors(sensors_no, 4001, 4444)
    server = setup_server(4001, 4444)

    print('[*] Killing first sensor... (it may take a while)')
    time.sleep(2)
    sensors[0].kill()

    # Czytamy dane z okresu normalnego działania sieci
    for p in sensors[1:]:
        line = p.stdout.readline()
        while b'ACK_MSG' not in line:
            line = p.stdout.readline()

    line = server.stdout.readline()
    while b'double-list' not in line:
        line = server.stdout.readline()

    for test_no in range(1, 11):
        print('[*] Test: ' + str(test_no))
        measurements = {}
        for i in range(1, sensors_no):
            line = sensors[i].stdout.readline()
            while b'measurement ' not in line:
                line = sensors[i].stdout.readline()
            measurements[i] = int(line.split(b' ')[-1][:-2])

        line = server.stdout.readline()
        while not (b'Received' in line and b'measurements' in line):
            line = server.stdout.readline()

        received_no = int(line.split(b' ')[2])
        if received_no != sensors_no - 1:
            print('\033[31m[-] Failed: server received ' + str(received_no) + ' measurements, ' +
                  str(sensors_no - 1) + ' expected.\033[39m')
            continue

        received = {}
        for i in range(0, received_no):
            line = server.stdout.readline()
            tokens = line.split(b' ')
            sensor_no = int(tokens[2])
            received[sensor_no] = int(tokens[4])

        if received == measurements:
            print('\033[32m[+] Success!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))
        else:
            print('\033[31m[-] Failed!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))

    for p in sensors[1:]:
        p.kill()
    server.kill()

    sensors = setup_sensors(sensors_no, 4001, 4444)
    server = setup_server(4001, 4444)

    print('[*] Killing last sensor... (it may take a while)')
    time.sleep(2)
    sensors[-1].kill()

    # Czytamy dane z okresu normalnego działania sieci
    for p in sensors[:-1]:
        line = p.stdout.readline()
        while b'ACK_MSG' not in line:
            line = p.stdout.readline()

    line = server.stdout.readline()
    while b'double-list' not in line:
        line = server.stdout.readline()

    for test_no in range(1, 11):
        print('[*] Test: ' + str(test_no))
        measurements = {}
        for i in range(0, sensors_no - 1):
            line = sensors[i].stdout.readline()
            while b'measurement ' not in line:
                line = sensors[i].stdout.readline()
            measurements[i] = int(line.split(b' ')[-1][:-2])

        line = server.stdout.readline()
        while not (b'Received' in line and b'measurements' in line):
            line = server.stdout.readline()

        received_no = int(line.split(b' ')[2])
        if received_no != sensors_no - 1:
            print('\033[31m[-] Failed: server received ' + str(received_no) + ' measurements, ' +
                  str(sensors_no - 1) + ' expected.\033[39m')
            continue

        received = {}
        for i in range(0, received_no):
            line = server.stdout.readline()
            tokens = line.split(b' ')
            sensor_no = int(tokens[2])
            received[sensor_no] = int(tokens[4])

        if received == measurements:
            print('\033[32m[+] Success!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))
        else:
            print('\033[31m[-] Failed!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))

    for p in sensors[:-1]:
        p.kill()
    server.kill()

    if sensors_no <= 2:
        return

    sensors = setup_sensors(sensors_no, 4001, 4444)
    server = setup_server(4001, 4444)

    print('[*] Killing middle sensor... (it may take a while)')
    killed_sensor_no = len(sensors) // 2
    time.sleep(2)
    sensors[killed_sensor_no].kill()

    # Czytamy dane z okresu normalnego działania sieci
    for i in range(0, sensors_no):
        if i == killed_sensor_no:
            continue
        line = sensors[i].stdout.readline()
        while b'ACK_MSG' not in line:
            line = sensors[i].stdout.readline()

    line = server.stdout.readline()
    while b'double-list' not in line:
        line = server.stdout.readline()

    for test_no in range(1, 11):
        print('[*] Test: ' + str(test_no))
        measurements = {}
        for i in range(0, sensors_no):
            if i == killed_sensor_no:
                continue
            line = sensors[i].stdout.readline()
            while b'measurement ' not in line:
                line = sensors[i].stdout.readline()
            measurements[i] = int(line.split(b' ')[-1][:-2])

        # Musimy uwzględnić fakt, że dane mogą zostać wypisane w dowolnej kolejności
        line = server.stdout.readline()
        while not (b'Received' in line and b'measurements' in line):
            line = server.stdout.readline()

        received_no = int(line.split(b' ')[2])

        line = server.stdout.readline()
        received = {}
        while not (b'Received' in line and b'measurements' in line):
            if b'Sensor' in line and b'Data' in line:
                tokens = line.split(b' ')
                received[int(tokens[2])] = int(tokens[4])
            line = server.stdout.readline()
        received_no += int(line.split(b' ')[2])

        if received_no != sensors_no - 1:
            print('\033[31m[-] Failed: server received ' + str(received_no) + ' measurements, ' +
                  str(len(sensors) - 1) + ' expected.\033[39m')
            continue

        while len(received) != received_no:
            line = server.stdout.readline()
            if b'Sensor' in line and b'Data' in line:
                tokens = line.split(b' ')
                received[int(tokens[2])] = int(tokens[4])

        if received == measurements:
            print('\033[32m[+] Success!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))
        else:
            print('\033[31m[-] Failed!\033[39m')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))

    for i in range(0, sensors_no):
        if i == killed_sensor_no:
            continue
        sensors[i].kill()
    server.kill()


def print_usage():
    print("tests.py [OPTIONS]")
    print("\t-c\tPath to config file (./tests_conf by default)")
    print("\t-m\tTesting mode: normal or emergency")
    print("\t-n\tNumber of sensors (SENSORS_NO from conf file by default)")
    print("\t-h\tDisplay this message")


def parse_config(file_name):
    global config
    with open(file_name, 'r') as file:
        for line in file:
            # dzielenie linii bez znaku końca linii
            tokens = line[:-1].split(' ')
            config[tokens[0]] = tokens[1]


# Mapowanie nazwy trybu testowania na funkcje testującą
test_modes = {
    'normal': test_normal,
    'emergency': test_emergency,
}


def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'c:hm:n:', [])
    except getopt.GetoptError as e:
        print(e)
        print_usage()
        sys.exit(1)

    conf_file_name = None
    test_mode = 'normal'
    sensors_no = None
    for i in opts:
        if '-c' in i:
            conf_file_name = i[1]
        elif '-m' in i:
            test_mode = i[1]
        elif '-n' in i:
            sensors_no = i[1]
        elif '-h' in i:
            print_usage()
            sys.exit(0)

    if not conf_file_name:
        if os.path.isfile('tests_conf'):
            conf_file_name = 'tests_conf'
        else:
            print('[-] Default config file missing!')
            sys.exit(1)

    parse_config(conf_file_name)

    if sensors_no:
        config['SENSORS_NO'] = sensors_no

    try:
        test_func = test_modes[test_mode]
    except KeyError:
        print('[-] Unsupported test mode: ' + test_mode)
        print('    Supported modes: normal, emergency')
        sys.exit(1)
    test_func()


if __name__ == '__main__':
    main()
