#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// MACROS
#define SERVER_IP "127.0.0.1"
#define LOCAL_HOST "127.0.0.1"
#define SERVER_PORT_TO 5002
#define CLIENT_PORT 6001
#define SERVER_PORT 6002
#define CLIENT_PORT_TO 5001
#define PAYLOAD_SIZE 1024
#define WINDOW_SIZE 5
#define TIMEOUT 2
#define MAX_SEQUENCE 1024



// Packet Layout
// You may change this if you want to
struct packet {
    short seqnum;
    short acknum;
    char ack;
    char last;
    char signoff;
    unsigned int length;
    char payload[PAYLOAD_SIZE];
    long int position_before_reading;
    long int position_after_reading;
};

// Utility function to build a packet
void build_packet(struct packet* pkt, short seqnum, short acknum, char last, char ack,unsigned int length, const char* payload, long int position_before_reading, long int position_after_reading, char signoff) {
    pkt->seqnum = seqnum;
    pkt->acknum = acknum;
    pkt->ack = ack;
    pkt->last = last;
    pkt->signoff = signoff;
    pkt->length = length;
    pkt->position_before_reading = position_before_reading;
    pkt->position_after_reading = position_after_reading;
    memcpy(pkt->payload, payload, length);
}

// Utility function to print a packet
void printRecv(struct packet* pkt) {
    printf("RECV %d %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", (pkt->ack) ? " ACK": "");
}

void printSend(struct packet* pkt, int resend) {
    if (resend)
        printf("RESEND %d %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", pkt->ack ? " ACK": "");
    else
        printf("SEND %d %d%s%s\n", pkt->seqnum, pkt->acknum, pkt->last ? " LAST": "", pkt->ack ? " ACK": "");
}

#endif