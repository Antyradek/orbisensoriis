/**
 * helper.h
 * Radosław Świątkiewicz
 * 2016-05-04
 */

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
void print_error(char* error_msg)
{
    //zastanawiam się, dlaczego to działa...
    printf(TEXT_COLOR_RED "[!] %s" TEXT_COLOR_DEFAULT "\n", error_msg);
}

//i na zielono
void print_success(char* success_msg)
{
    printf(TEXT_COLOR_GREEN "[V] %s" TEXT_COLOR_DEFAULT "\n", success_msg);
}

//żółto
void print_warning(char* warning_msg)
{
    printf(TEXT_COLOR_YELLOW "[?] %s" TEXT_COLOR_DEFAULT "\n", warning_msg);
}

//i na domyślnie
void print_info(char* info_msg)
{
    printf("[I] %s\n", info_msg);
}
