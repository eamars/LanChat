/**
 * message packet used for inter-process communication
 */

#include "packet.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int packet_encode(char *msg, Packet *p)
{
	msg[0] = p->version;
	msg[1] = p->command;

	memcpy(msg + 2, p->optional, 14); // copy optional header
	memcpy(msg + PACKET_HEAD_SZ, p->payload, PACKET_PAYLOAD_SZ); // copy payload
	return 0;
}

int packet_decode(char *msg, Packet *p)
{
	p->version = msg[0];
	p->command = msg[1];

	memcpy(p->optional, msg + 2, 14);
	memcpy(p->payload, msg + PACKET_HEAD_SZ, PACKET_PAYLOAD_SZ);
	return 0;
}


void packet_print(Packet *p)
{
	printf("Version: %u\nCommand: %u\nOptional: [%s]\nPayload: [%s]\n",
		p->version,
		p->command,
		p->optional,
		p->payload
	);

}
