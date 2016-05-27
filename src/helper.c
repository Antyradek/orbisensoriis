#include "helper.h"

#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>


//Makra ANSI ESCAPE CODES dla ładnych kolorków w terminalu.
#define TEXT_COLOR_RED          "\033[31m"
#define TEXT_COLOR_GREEN        "\033[32m"
#define TEXT_COLOR_YELLOW       "\033[33m"
#define TEXT_COLOR_DEFAULT      "\033[39m"


static pthread_mutex_t mutex;

void print_init()
{
    if (pthread_mutex_init(&mutex, NULL) != 0)
    {
        fprintf(stderr, TEXT_COLOR_RED "[!] Mutex init failed!\n");
    }
}

void print_error(const char* fmt, ...)
{
    pthread_mutex_lock(&mutex);
    va_list argp;
    va_start(argp, fmt);
    fprintf(stderr, TEXT_COLOR_RED "[!] ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);
    pthread_mutex_unlock(&mutex);

}

void print_success(const char* fmt, ...)
{
    pthread_mutex_lock(&mutex);
    va_list argp;
    va_start(argp, fmt);
    fprintf(stdout, TEXT_COLOR_GREEN "[V] ");
    vfprintf(stdout, fmt, argp);
    fprintf(stdout, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);
    pthread_mutex_unlock(&mutex);
}

void print_warning(const char* fmt, ...)
{
    pthread_mutex_lock(&mutex);
    va_list argp;
    va_start(argp, fmt);
    fprintf(stderr, TEXT_COLOR_YELLOW "[?] ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);
    pthread_mutex_unlock(&mutex);
}

void print_info(const char* fmt, ...)
{
    pthread_mutex_lock(&mutex);
    va_list argp;
    va_start(argp, fmt);
    fprintf(stdout, "[I] ");
    vfprintf(stdout, fmt, argp);
    fprintf(stdout, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);
    pthread_mutex_unlock(&mutex);
}