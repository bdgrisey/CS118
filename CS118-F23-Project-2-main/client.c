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

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

    // Initialize variables for Go-Back-N
    unsigned short base = 0; // Sequence number of oldest unacknowledged packet
    unsigned short nextseqnum = 0; // Sequence number of next packet to send
    struct packet window[WINDOW_SIZE]; // Send window
    int window_count = 0; // Number of packets in the window

    while (!feof(fp) || window_count > 0) {
        // Send packets in the window
        while (window_count < WINDOW_SIZE && !feof(fp)) {
            // Read data from file
            size_t bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
            if (bytes_read == 0) {
                if (feof(fp))
                    last = 1;
                else {
                    perror("Error reading from file");
                    break;
                }
            }
            if (bytes_read < PAYLOAD_SIZE) {
                if (feof(fp)) {
                    last = 1;
                }
            }

            // Create packet
            build_packet(&pkt, nextseqnum, ack_num, last, ack, bytes_read, buffer);
            printf("Packet created\n");

            // Add packet to window
            window[window_count++] = pkt;

            // Send packet
            sendto(send_sockfd, &pkt, sizeof(pkt), 0, (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
            printf("Packet %d sent\n", nextseqnum);
            nextseqnum++; // Update sequence number for next packet
        }

        // Receive acknowledgments
        struct packet ack_pkt;
        bytes_read = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0, NULL, NULL);
        if (bytes_read >= 0 && ack_pkt.acknum >= base && ack_pkt.acknum < base + WINDOW_SIZE) {
            // Acknowledgment received within window range
            printf("Acknowledgment received for sequence number %d\n", ack_pkt.acknum);

            // Slide window
            while (base <= ack_pkt.acknum) {
                base++;
                window_count--;
            }
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout occurred, resend all unacknowledged packets in the window
            printf("Timeout occurred, resending packets in window\n");
            for (int i = 0; i < window_count; i++) {
                sendto(send_sockfd, &window[i], sizeof(window[i]), 0, (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
                printf("Packet %d resent\n", window[i].seqnum);
            }
        } else {
            // Error occurred
            perror("recvfrom");
            // Handle error
        }
    }
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

