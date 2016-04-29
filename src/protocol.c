/**
 * protocol.c
 * Paweł Szewczyk
 * 2016-04-26
 */

#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "protocol.h"

/**
 * @brief pakuje init_msg
 * @param[in] msg Wiadomość do spakowania
 * @param[out] dst Bufor wynikowy
 * @param[in] n Rozmiar bufora dst
 * @return liczba zapisanych bajtów lub ujemny kod błędu w przypadku błędu
 */
static int pack_init_msg(struct init_msg *msg, unsigned char *dst, int n)
{
	uint16_t timeout = msg->timeout << 2;
	uint16_t period = msg->period << 2;

	if (n < 4)
		return -ENOMEM;

	dst[0] = msg->type << 5
		| ((timeout & 0xf800) >> 11);
	dst[1] = ((timeout & 0x07f8) >> 3);
	dst[2] = ((timeout & 0x0004) << 5)
		| ((period & 0xfe00) >> 9);
	dst[3] = ((period & 0x01fc) >> 1);

	return 4;
}

/**
 * @brief rozpakowuje init_msg
 * @param[in] src Bufor zawierający spakowaną wiadomość
 * @param[out] dst Struktura wynikowa
 * @return typ wiadomości lub ujemny kod błędu w przypadku błędu
 */
static int unpack_init_msg(unsigned char *src, struct init_msg *dst)
{
	uint16_t timeout;
	uint16_t period;

	dst->type = (src[0] & 0xe0) >> 5;
	timeout = ((uint16_t)(src[0] & 0x1f) << 11)
		| ((uint16_t)(src[1]) << 3)
		| ((uint16_t)(src[2] & 0x80) >> 5);
	period = ((uint16_t)(src[2] & 0x7f) << 9)
		| ((uint16_t)(src[3] & 0xfe) << 1);

	dst->timeout = timeout >> 2;
	dst->period = period >> 2;

	return dst->type;
}

/**
 * @brief pakuje data_msg
 * @param[in] msg Wiadomość do spakowania
 * @param[out] dst Bufor wynikowy
 * @param[in] n Rozmiar bufora dst
 * @return liczba zapisanych bajtów lub ujemny kod błędu w przypadku błędu
 */
static int pack_data_msg(struct data_msg *msg, unsigned char *dst, int n)
{
	uint16_t count = msg->count << 3;
	int i;
	unsigned char *ptr;

	if (n < DATA_T_SIZE * msg->count + 2)
		return -ENOMEM;

	dst[0] = (msg->type << 5)
		| ((count & 0xf800) >> 11);
	dst[1] = (count & 0x07f8) >> 3;

	ptr = dst + 2;
	for (i = 0; i < msg->count; ++i) {
		(*(uint16_t *)ptr) = msg->data[i].id;
		ptr += 2;
		(*(uint32_t *)ptr) = msg->data[i].data;
		ptr += 4;
	}

	return DATA_T_SIZE * msg->count + 2;
}

/**
 * @brief rozpakowuje data_msg
 * @param[in] src Bufor zawierający spakowaną wiadomość
 * @param[out] dst Struktura wynikowa
 * @return typ wiadomości lub ujemny kod błędu w przypadku błędu
 */
static int unpack_data_msg(unsigned char *src, struct data_msg *dst)
{
	int i;
	unsigned char *ptr;

	dst->type = (src[0] & 0xe0) >> 5;
	dst->count = (((uint16_t)(src[0] & 0x1f) << 9)
		| ((uint16_t)(src[1]) << 3)) >> 3;

	dst->data = malloc(dst->count * sizeof(*dst->data));

	ptr = src + 2;
	for (i = 0; i < dst->count; ++i) {
		dst->data[i].id = *(uint16_t *)ptr;
		ptr += 2;
		dst->data[i].data = *(uint32_t *)ptr;
		ptr += 4;
	}

	return dst->type;
}

/**
 * @brief pakuje info_msg
 * @param[in] msg Wiadomość do spakowania
 * @param[out] dst Bufor wynikowy
 * @param[in] n Rozmiar bufora dst
 * @return liczba zapisanych bajtów lub ujemny kod błędu w przypadku błędu
 */
static int pack_info_msg(struct info_msg *msg, unsigned char *dst, int n)
{
	if (n < 1)
		return -ENOMEM;

	dst[0] = msg->type << 5;

	return 1;
}

/**
 * @brief rozpakowuje init_msg
 * @param[in] src Bufor zawierający spakowaną wiadomość
 * @param[out] dst Struktura wynikowa
 * @return typ wiadomości lub ujemny kod błędu w przypadku błędu
 */
static int unpack_info_msg(unsigned char *src,struct info_msg *dst)
{	
	dst->type = (src[0] & 0xe0) >> 5;

	return dst->type;
}

int pack_msg(void *msg, unsigned char *dst, int n)
{
	struct info_msg *header;
	int i, off;

	header = msg;
	switch (header->type) {
	case INIT_MSG:
	case RECONF_MSG:
		return pack_init_msg(msg, dst, n);
	case DATA_MSG:
		return pack_data_msg(msg, dst, n);
	case ERR_MSG:
	case ACK_MSG:
	case FINIT_MSG:
		return pack_info_msg(msg, dst, n);
	default:
		return -ENOTSUP;
	}
}

int unpack_msg(unsigned char *src, union msg *dst)
{
	uint8_t type = (src[0] & 0xe0) >> 5;

	switch (type) {
	case INIT_MSG:
	case RECONF_MSG:
		return unpack_init_msg(src, &dst->init);
	case DATA_MSG:
		return unpack_data_msg(src, &dst->data);
	case ERR_MSG:
	case ACK_MSG:
	case FINIT_MSG:
		return unpack_info_msg(src, &dst->info);
	default:
		return -ENOTSUP;
	}

	return -1;
}

void cleanup_msg(union msg *msg)
{
	if (msg->type == DATA_MSG)
		free(msg->data.data);
}
