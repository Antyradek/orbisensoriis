#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum msg_type {
	INIT_MSG = 1,
	DATA_MSG = 2,
	ERR_MSG = 3,
	ACK_MSG = 4,
	FINIT_MSG = 5,
	RECONF_MSG = 6,
};

struct init_msg {
	uint8_t type;
	uint16_t timeout;
	uint16_t period;
};

struct data_t {
	uint16_t id;
	uint32_t data;
};

#define DATA_T_SIZE 6

struct data_msg {
	uint8_t type;
	uint16_t count;
	struct data_t *data;
};

struct info_msg {
	uint8_t type;
};

union msg {
	uint8_t type;
	struct info_msg info;
	struct data_msg data;
	struct init_msg init;
};

int pack_msg(void *msg, unsigned char *dst, int n);
int unpack_msg(unsigned char *buf, union msg *dst);
void cleanup_msg(union msg *msg);

#endif
