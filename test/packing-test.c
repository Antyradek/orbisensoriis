/**
 * packing-test.c
 * Paweł Szewczyk
 * 2016-04-26
 */

#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "protocol.h"

#define MAX14 0x3fff
#define MAX13 0x1fff

void test_init() {
	struct init_msg in = {INIT_MSG, 0x0001, 0x0001};
	union msg out;
	unsigned char buf[32];

	in.timeout = rand() % MAX14;
	in.period = rand() % MAX14;

	pack_msg(&in, buf, 32);
	unpack_msg(buf, &out);

	assert(in.type == out.init.type);
	assert(in.period == out.init.period);
	assert(in.timeout == out.init.timeout);
}

void test_data() {
	struct data_msg in;
	union msg out;
	unsigned char buf[MAX13];
	int i;

	in.type = DATA_MSG;
	in.count = rand() % 64;

	in.data = malloc(in.count * sizeof(*in.data));
	for (i = 0; i < in.count; ++i) {
		in.data[i].id = rand() % 0xffff;
		in.data[i].data = rand() % 0xffffffff;
	}

	pack_msg(&in, buf, MAX13);
	unpack_msg(buf, &out);

	assert(in.type == out.data.type);
	assert(in.count == out.data.count);

	cleanup_msg(&out);
}

void test_info() {
	struct info_msg in;
	union msg out;
	unsigned char buf[16];

        in.type = ERR_MSG;
	pack_msg(&in, buf, 16);
	unpack_msg(buf, &out);
	assert(in.type == out.info.type);

        in.type = ACK_MSG;
	pack_msg(&in, buf, 16);
	unpack_msg(buf, &out);
	assert(in.type == out.info.type);

        in.type = FINIT_MSG;
	pack_msg(&in, buf, 16);
	unpack_msg(buf, &out);
	assert(in.type == out.info.type);
}

int main()
{
	int i, nmb = 1000;
	srand(time(NULL));

	printf("init_msg: \n");
	for(i = 0; i < nmb; ++i)
		test_init();
	printf(" [OK] %d/%d\n", nmb, i);
	
	printf("data_msg: \n");
	for(i = 0; i < nmb; ++i)
		test_data();
	printf(" [OK] %d/%d\n", nmb, i);

	printf("info_msg: \n");
	for(i = 0; i < nmb; ++i)
		test_info();
	printf(" [OK] %d/%d\n", nmb, i);
}
