#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include "utils.h"

int main(int argc, char *argv[]) {
    int listen_sockfd, send_sockfd;
    struct sockaddr_in client_addr, server_addr_to, server_addr_from;
    socklen_t addr_size = sizeof(server_addr_to);
    struct timeval tv;
    struct packet pkt;
    struct packet ack_pkt;
    char buffer[PAYLOAD_SIZE];
    unsigned short seq_num = 0;
    unsigned short ack_num = 0;
    char last = 0;
    char ack = 0;
    struct timeval timeout;
    struct packet_with_seq send_window[WINDOW_SIZE];
    unsigned short next_seq_num = 0;
    unsigned short ack_expected = 0;
    unsigned short ack_received;
    
    // read filename from command line argument
    if (argc != 2) {
        printf("Usage: ./client <filename>\n");
        return 1;
    }
    char *filename = argv[1];
    
    // Create a UDP socket for listening
    listen_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (listen_sockfd < 0) {
        perror("Could not create listen socket");
        return 1;
    }
    
    // Create a UDP socket for sending
    send_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (send_sockfd < 0) {
        perror("Could not create send socket");
        return 1;
    }

    // Configure the server address structure to which we will send data
    memset(&server_addr_to, 0, sizeof(server_addr_to));
    server_addr_to.sin_family = AF_INET;
    server_addr_to.sin_port = htons(SERVER_PORT_TO);
    server_addr_to.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    // Configure the client address structure
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(CLIENT_PORT);
    client_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the listen socket to the client address
    if (bind(listen_sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
        perror("Bind failed");
        close(listen_sockfd);
        return 1;
    }
    
    // Open file for reading
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        close(listen_sockfd);
        close(send_sockfd);
        return 1;
    }
    
    // TODO: Read from file, and initiate reliable data transfer to the server
    // Build and send packet
    // Wait for acknowledgment
    // You need to implement acknowledgment handling here
    // Receive an acknowledgment from the server
    // Ensure the acknowledgment corresponds to the sent packet
    // If acknowledgment received is correct, update sequence number and continue sending
    // Otherwise, resend the packet

    // tv.tv_sec = 1;
    // tv.tv_usec = 0;
    // setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

    // Initialize variables for Go-Back-N
    unsigned short base = 0; // Sequence number of oldest unacknowledged packet
    unsigned short nextseqnum = 0; // Sequence number of next packet to send
    struct packet window[WINDOW_SIZE]; // Send window
    int window_count = 0; // Number of packets in the window
    clock_t start, end;

    while (!feof(fp) || ack_expected != next_seq_num) {
        // Send packets within window
        while (next_seq_num < ack_expected + WINDOW_SIZE && !feof(fp)) {
            struct packet pkt;
            size_t bytes_read = fread(pkt.payload, 1, PAYLOAD_SIZE, fp);
            if (bytes_read == 0 && feof(fp))
                last = 1;
            build_packet(&pkt, next_seq_num, last, bytes_read);
            struct packet_with_seq pkt_with_seq = { pkt, next_seq_num };
            sendto(send_sockfd, &pkt_with_seq, sizeof(pkt_with_seq), 0, (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
            printf("Packet %d sent\n", next_seq_num);
            if (next_seq_num == ack_expected) // Start timer for oldest unacknowledged packet
                timeout.tv_sec = TIMEOUT_SEC;
            next_seq_num++;
        }

        // Receive acknowledgments
        struct packet ack_pkt;
        ssize_t bytes_read = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0, NULL, NULL);
        if (bytes_read >= 0 && bytes_read == sizeof(struct packet) && ack_pkt.acknum >= ack_expected) {
            printf("Received acknowledgment for packet %d\n", ack_pkt.acknum);
            ack_received = ack_pkt.acknum;
            while (ack_expected <= ack_received) {
                ack_expected++;
                if (ack_expected == next_seq_num) // Stop timer if all packets are acknowledged
                    timeout.tv_sec = 0;
            }
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("Timeout occurred, resending packets %d-%d\n", ack_expected, next_seq_num - 1);
            next_seq_num = ack_expected; // Resend unacknowledged packets
        } else {
            perror("recvfrom");
            // Handle error
        }
    }
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

