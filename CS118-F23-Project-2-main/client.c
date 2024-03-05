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


    // Define a structure to represent packets in the window
    struct window_packet {
        struct packet pkt;
        int sent; // Flag to track if packet has been sent
    };

    // Main loop for sending packets
    while (!feof(fp) || base < next_seq_num) {
        // Send packets within the window
        while (next_seq_num < base + WINDOW_SIZE && !feof(fp)) {
            // Read data from file
            size_t bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
            if (bytes_read == 0 && feof(fp)) {
                last = 1;
            }

            // Create packet
            build_packet(&send_window[next_seq_num % WINDOW_SIZE].pkt, next_seq_num, ack_num, last, ack, bytes_read, buffer);
            send_window[next_seq_num % WINDOW_SIZE].sent = 0; // Mark packet as not yet sent

            next_seq_num++;
        }

        // Send packets within the window if not already sent
        for (int i = base; i < next_seq_num; i++) {
            if (!send_window[i % WINDOW_SIZE].sent) {
                sendto(send_sockfd, &send_window[i % WINDOW_SIZE].pkt, sizeof(struct packet), 0, (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
                send_window[i % WINDOW_SIZE].sent = 1; // Mark packet as sent
            }
        }

        // Receive acknowledgments
        while (1) {
            struct packet ack_pkt;
            bytes_read = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0, NULL, NULL);
            if (bytes_read >= 0) {
                // Acknowledgment received
                last_ack_received = ack_pkt.acknum;
                // Slide window if acknowledgment for base packet
                if (last_ack_received == base) {
                    base++;
                }
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout occurred, resend packets
                break;
            } else {
                // Handle other errors
                perror("recvfrom");
                // Handle error
            }
        }
    }
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

