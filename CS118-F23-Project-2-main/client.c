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
    unsigned short next_seq_num = 0;
    unsigned short base = 0;
    struct packet window[WINDOW_SIZE];
    int total_bytes_sent = 0;
    
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

    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

    while (!feof(fp)) {
        // // Read data from file
        // size_t bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
        // if (bytes_read == 0) {
        //     if (feof(fp))
        //         last = 1;
        //     else {
        //         perror("Error reading from file");
        //         break;
        //     }
        // }
        // if (bytes_read < PAYLOAD_SIZE) {
        //     if (feof(fp)) {
        //         last = 1;
        //     }
        // }

        // // Create packet
        // build_packet(&pkt, seq_num, ack_num, last, ack, bytes_read, buffer);
        // printf("Packet created\n");

        while (next_seq_num < base + WINDOW_SIZE && !feof(fp)) {
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
            total_bytes_sent += bytes_read;
            // Create packet and add it to the sending window
            build_packet(&window[next_seq_num % WINDOW_SIZE], next_seq_num, ack_num, last, ack, bytes_read, buffer);
            printf("Packet %d created\n", next_seq_num);
            // Send the packet
            sendto(send_sockfd, &window[next_seq_num % WINDOW_SIZE], sizeof(window[next_seq_num % WINDOW_SIZE]), 0,
                (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
            printf("Packet %d sent\n", next_seq_num);
            // Increment sequence number for the next packet
            usleep(100000);
            next_seq_num++;
        }
        
        // Receive Acknowledgement
        int bytes_read_from_socket = 0;
        bytes_read_from_socket = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0, NULL, NULL);

        if (bytes_read_from_socket > 0 && ack_pkt.acknum >= base) {
            // Acknowledgment received
            printf("Acknowledgment received for sequence number %d\n", seq_num);
            total_bytes_sent = total_bytes_sent - ((ack_pkt.acknum - base + 1) * ack_pkt.length);
            base += (ack_pkt.acknum - base) + 1; // Update sequence number for next packet
            if(pkt.last)
                pkt.signoff = 1;
        } else if (bytes_read_from_socket > 0) {
            printf("Cumulatively acknowledged sequence number %d\n", ack_pkt.acknum);
        } else if (errno == EAGAIN || errno == EWOULDBLOCK || bytes_read_from_socket <= 0) {
            // Timeout occurred
            if (pkt.last && pkt.signoff) {
                break;
            } else {
                printf("Timeout occurred, resending packets starting from: %d\n", base);
                fseek(fp, -total_bytes_sent, SEEK_CUR);
                total_bytes_sent = 0;
                next_seq_num = base;
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

