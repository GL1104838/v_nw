#ifndef COMMUNICATION_H
#define COMMUNICATION_H

typedef enum communication_type {
	BEG,
	PRO,
	END,
	INI,
	REQ,
	CLO,
	ACK,
	ERR,
	WAIT,
	INVALID
};

typedef struct communication_data {
	enum communication_type communication_type;
	int * args;
	unsigned int args_count;
};

void read_communication(FILE * rfd, struct communication_data * data);
void write_communication(FILE * wfd, struct communication_data * data);
void setup_communication_data(FILE * rfd, struct communication_data * data);

#endif