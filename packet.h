/**
 * Header for message packet used for inter-process communication
 */
#ifndef PACKET_H_
#define PACKET_H_

/**
 * Packet format
 * 1(1): version
 * 2(1): command
 *
 */

#define PACKET_PAYLOAD_SZ 4096 // max buffer size
#define PACKET_HEAD_SZ 16

typedef struct packet_s
{
	unsigned char version;
	unsigned char command;
	char optional[14];
	char payload[PACKET_PAYLOAD_SZ];
} Packet;

int packet_encode(char *msg, Packet *p);
int packet_decode(char *msg, Packet *p);
void packet_print(Packet *p);

#endif
