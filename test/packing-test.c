#include <assert.h>
#include <time.h>
#include <stdlib.h>

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

int main()
{
	int i;
	srand(time(NULL));

	for(i = 0; i < 1000; ++i) {
		test_init();
		test_data();
		/* TODO test other packets */
	}
}
