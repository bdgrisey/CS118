#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
#define QUEUE_SIZE 100



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
};

//Queue structure
typedef struct{
    struct packet queue[QUEUE_SIZE];
    short head, tail, num_entries;
}circular_queue;


// Utility function to build a packet
void build_packet(struct packet* pkt, short seqnum, short acknum, char last, char ack,unsigned int length, const char* payload) {
    pkt->seqnum = seqnum;
    pkt->acknum = acknum;
    pkt->ack = ack;
    pkt->last = last;
    pkt->signoff = 0;
    pkt->length = length;
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

// Utility functions for circular queue

bool queue_empty(circular_queue *q)
{
    return (q->num_entries == 0);
}

bool queue_full(circular_queue *q)
{
    return (q->num_entries == QUEUE_SIZE);
}

bool enqueue(circular_queue *q, struct packet* pkt)
{
    //check if q is full
    if(queue_full(q))
        return false;

    q->queue[q->tail] = *pkt;
    q->num_entries++;
    q->tail = (q->tail + 1) % QUEUE_SIZE;

    return true;
}

bool dequeue(circular_queue *q)
{
    if(queue_empty(q))
        return false;

    q->head = (q->head + 1) % QUEUE_SIZE;
    q->num_entries--;
    return true;
}









#endif