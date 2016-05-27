/**
 * helper.h
 * Radosław Świątkiewicz
 * 2016-05-04
 *
 * Mateusz Blicharski
 * Zamiana printf na vfprintf
 * 2016-05-06 
 * Podział na plik *.c i *.h
 * Synchronizacja z użyciem semafora
 * 2016-05-27
 */

#ifndef __HELPER_H__
#define __HELPER_H__

/**
 * @brief Nagłówek zawierający kilka pomocnych stałych i funkcji.
 */

//TODO napisać odpowiednio komentarze w doxygenie, jak ogarnę, dlaczego nie mam autouzupełniania

//tworzy semafor, powinna być wywołana przed pozostałymi metodami print_*
void print_init();

//drukuje na czerwono
void print_error(const char* fmt, ...);

//i na zielono
void print_success(const char* fmt, ...);

//żółto
void print_warning(const char* fmt, ...);

//i na domyślnie
void print_info(const char* fmt, ...);

#endif