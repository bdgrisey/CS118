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
    short last_seq_num = -1;
    short ack_num = 0;
    char last = 0;
    char last_packet = 0;
    char ack = 0;
    short next_seq_num = 0;
    short sent_seq_num = 0;
    short base = 0;
    struct packet* window[MAX_WINDOW_SIZE]; //make this a pointer
    int total_bytes_sent = 0;
    circular_queue master_queue; //make this the actual data
    int frame_size = 0; // frame_size <= WINDOW_SIZE
    //AIMD VARS
    int current_window = 3;
    int dup_ack_cnt = 0;
    
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
    tv.tv_usec = 500000;
    setsockopt(listen_sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));

    // Initialize master_queue
    master_queue.head = 0;
    master_queue.tail = 0;
    master_queue.num_entries = 0;

    while (1) {
        
        // Populate master queue
        while (!queue_full(&master_queue) && !feof(fp))
        {
            size_t bytes_read = fread(buffer, 1, PAYLOAD_SIZE, fp);
            if (bytes_read == 0) {
                if (feof(fp)) {
                    last_packet = 1;
                    last_seq_num = next_seq_num;
                } else {
                    perror("Error reading from file");
                    break;
                }
            }
            if (bytes_read < PAYLOAD_SIZE) {
                if (feof(fp)) {
                    last_packet = 1;
                    last_seq_num = next_seq_num;
                }
            }
            total_bytes_sent += bytes_read;
            // Create packet and add it to the master queue
            build_packet(&(master_queue.queue[master_queue.tail]), next_seq_num, ack_num, last_packet, ack, bytes_read, buffer);
            enqueue(&master_queue);
            printf("Packet %d created\n", next_seq_num);
            next_seq_num++;
        }



        // Create and send window
        // Point window to the head of the master_queue
        if(!last)
            frame_size = assign_range(&master_queue, window, current_window);
        sent_seq_num = base;
        while ((sent_seq_num < base + frame_size)  && (!last)) {
            // Send the packet
            sendto(send_sockfd, window[sent_seq_num % frame_size], sizeof(*window[sent_seq_num % frame_size]), 0,
                (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
            printf("Packet %d sent\n", sent_seq_num);
            // Increment sequence number for the next packet
            usleep(100);
            sent_seq_num++;
        }
    
        // Receive Acknowledgement  and send window/signoff
        int bytes_read_from_socket = 0;
        bytes_read_from_socket = recvfrom(listen_sockfd, &ack_pkt, sizeof(ack_pkt), 0, NULL, NULL);

        if (bytes_read_from_socket > 0 && ack_pkt.acknum >= base) {
            // Acknowledgment received
            printf("Acknowledgment received for sequence number %d\n", ack_pkt.acknum);
            //TODO: ADD CODE TO DEQUEUE ACKED PACKETS
            int acked_range = (ack_pkt.acknum - base + 1);
            for(short j = 0; j < acked_range; j++)
            {
                dequeue(&master_queue);
            }
            base += acked_range; // Update sequence number for next packet
            current_window = (current_window < MAX_WINDOW_SIZE) ? current_window+1 : current_window; // Additive Increase to window
            dup_ack_cnt = 0; // restart counter for duplicate acks
            printf("Base: %d\n", base);
            printf("Last: %d\n", last);
            if(ack_pkt.acknum == last_seq_num)
            {
                last = 1;
                // If last meaningful packet ACKed then set last frame to signoff
                window[0]->last = 1;
                window[0]->signoff = 1;
                for(short i = 0; i < TIMEOUT; i++)
                {
                    // spam signoff packets to server
                    printf("Spam Signoff\n");
                    sendto(send_sockfd, window[0], sizeof(*window[0]), 0,
                        (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
                }
            }
                
        } else if (bytes_read_from_socket > 0) {
            printf("Cumulatively acknowledged sequence number %d\n", ack_pkt.acknum);
            dup_ack_cnt++;
            if(dup_ack_cnt >= 3)
            {
                // Multiplicative decrease
                current_window = (current_window/2 > 0) ? current_window/2 : 1;
                dup_ack_cnt = 0;
                //perhaps max the window size
            }
            if(last)
            {
                for(short i = 0; i < TIMEOUT; i++)
                {
                    // spam signoff packets to server until no response
                    printf("Spam Signoff\n");
                    sendto(send_sockfd, window[0], sizeof(*window[0]), 0,
                        (struct sockaddr *)&server_addr_to, sizeof(server_addr_to));
                }
            }
        } else if (errno == EAGAIN || errno == EWOULDBLOCK || bytes_read_from_socket <= 0) {
            // Timeout occurred
            if (window[0]->last && window[0]->signoff) {
                sleep(2);
                break;
            } else {
                printf("Timeout occurred, resending packets starting from: %d\n", base);
                total_bytes_sent = 0;
            }
        } else {
            // Error occurred
            perror("recvfrom");
            // Handle error
        }
    }
    
    printf("File sent successfully.\n");
    
    fclose(fp);
    close(listen_sockfd);
    close(send_sockfd);
    return 0;
}

