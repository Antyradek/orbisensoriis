#!/usr/bin/env python3
# TIN - orbisenoriis
# Testy
# Autor: Mateusz Blicharski

import getopt
import os
import shutil
import sys

# Słownik z danymi pobranymi z pliku konfiguracyjnego
config = {}


def test_normal():
    print('[*] Testing normal work of a network.')

    try:
        sensors_no = int(config['SENSORS_NO'])
    except KeyError:
        print('[-] Number of sensors missing! (pass it by SENSORS_NO in conf file or -n option)')
        sys.exit(1)

    try:
        server_path = config['SERVER_PATH']
    except KeyError:
        print('[-] Path to a server binary missing! (SERVER_PATH in conf file)')
        sys.exit(1)

    try:
        sensor_path = config['SENSOR_PATH']
    except KeyError:
        print('[-] Path to a sensor binary missing! (SENSOR_PATH in conf file)')
        sys.exit(1)

    print('[*] Spawning ' + str(sensors_no) + ' sensors.')

    fifo_dir = 'tmp_fifo'
    if os.path.exists(fifo_dir):
        shutil.rmtree(fifo_dir)

    os.mkdir(fifo_dir)

    fifos = []
    for i in range(0, sensors_no):
        os.mkfifo(fifo_dir + '/fifo' + str(i), 0o777)
        os.system(sensor_path + ' localhost ' + str(4001 + i) + ' localhost ' + str(4002 + i) + ' ' + str(i) +
                  ' 1>' + fifo_dir + '/fifo' + str(i) + ' 2>&1 &')
        fifos.append(open(fifo_dir + '/fifo' + str(i), 'r'))

    print('[*] Starting server.')
    os.mkfifo(fifo_dir + '/server_fifo', 0o777)
    os.system(server_path + ' -l ' + str(4001 + sensors_no) + ' 1>' + fifo_dir + '/server_fifo 2>&1 &')
    server_fifo = open(fifo_dir + '/server_fifo')

    for test_no in range(1, 11):
        print('[*] Test: ' + str(test_no))
        measurements = {}
        for i in range(0, sensors_no):
            line = fifos[i].readline()
            while 'measurement ' not in line:
                line = fifos[i].readline()
            measurements[i] = int(line.split(' ')[-1][:-2])
        line = server_fifo.readline()
        while not ('Received' in line and 'measurements' in line):
            line = server_fifo.readline()
        received_no = int(line.split(' ')[2])
        if received_no != sensors_no:
            print('[-] Failed: server received ' + str(received_no) + ' measurements, ' +
                  str(sensors_no) + ' expected.')
            continue
        received = {}
        for i in range(0, received_no):
            line = server_fifo.readline()
            tokens = line.split(' ')
            sensor_no = int(tokens[2])
            received[sensor_no] = int(tokens[4])

        if received == measurements:
            print('[+] Success!')
        else:
            print('[-] Failed!')
            print('    Data from sensors:')
            for key in measurements:
                print('    ' + str(key) + ': ' + str(measurements[key]))
            print('    Data received by server:')
            for key in received:
                print('    ' + str(key) + ': ' + str(received[key]))


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


# Słownik z trybami testowania
test_modes = {
    'normal': test_normal,
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
