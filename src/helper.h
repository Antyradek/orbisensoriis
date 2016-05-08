/**
 * helper.h
 * Radosław Świątkiewicz
 * 2016-05-04
 *
 * Mateusz Blicharski
 * Zamiana printf na vfprintf
 * 2016-05-06 
 */

#include <stdio.h>
#include <stdarg.h>

/**
 * @brief Nagłówek zawierający kilka pomocnych stałych i funkcji.
 */

//Makra ANSI ESCAPE CODES dla ładnych kolorków w terminalu.
#define TEXT_COLOR_RED          "\033[31m"
#define TEXT_COLOR_GREEN        "\033[32m"
#define TEXT_COLOR_YELLOW       "\033[33m"
#define TEXT_COLOR_DEFAULT      "\033[39m"

//TODO napisać odpowiednio komentarze w doxygenie, jak ogarnę, dlaczego nie mam autouzupełniania

//drukuje na czerwono
void print_error(const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    fprintf(stderr, TEXT_COLOR_RED "[!] ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);

}

//i na zielono
void print_success(const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    fprintf(stdout, TEXT_COLOR_GREEN "[V] ");
    vfprintf(stdout, fmt, argp);
    fprintf(stdout, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);
}

//żółto
void print_warning(const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    fprintf(stderr, TEXT_COLOR_YELLOW "[?] ");
    vfprintf(stderr, fmt, argp);
    fprintf(stderr, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);
}

//i na domyślnie
void print_info(const char* fmt, ...)
{
    va_list argp;
    va_start(argp, fmt);
    fprintf(stdout, "[I] ");
    vfprintf(stdout, fmt, argp);
    fprintf(stdout, "\n" TEXT_COLOR_DEFAULT);
    va_end(argp);
}
