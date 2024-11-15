#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/*
Project 1 Team:
Rahul Mallick
Ben Grisey
*/

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "127.0.0.1"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

//////////////////////////////
/// Helper functions Start ///
//////////////////////////////

// Replaces all instances of '%20' with a space and all instances of '%25' with '%'
void replace_URL_encodings(char *str) {
    char *src = str;
    char *dst = str;
    
    // Idea: uses two pointer method of traversing through string and replacing URL encodings with valid equivalents
    while (*src) {
        if (strncmp(src, "%20", 3) == 0) {
            *dst++ = ' ';
            src += 3;
        } else if (strncmp(src, "%25", 3) == 0) {
            *dst++ = '%';
            src += 3;
        }
        else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

// Finds last occurence of '.' and gets the file type (without the '.')
const char *extract_file_type(const char *path) {
    const char *file_type = strrchr(path, '.');
    if (file_type == NULL) {
        return "";
    }
    return file_type + 1;
}

////////////////////////////
/// Helper functions End ///
////////////////////////////

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    
    // char file_name[] = "test.txt";
    // printf("Received request:\n%s\n", buffer);
    // printf("-------");
    
    // Variables to hold first parts of the request header
    char method[10];
    char path[BUFFER_SIZE];
    char http_version[20];

    // Use sscanf to parse the first request line
    if (sscanf(buffer, "%9s %1023s %19s", method, path, http_version) != 3) {
        // Invalid request format
        return;
    }

    // Getting path without slash ('/')
    char path_without_slash[BUFFER_SIZE];
    strcpy(path_without_slash, path + 1);

    // If the requested path is "/", default to index.html
    if (strcmp(path, "/") == 0) {
        strcpy(path_without_slash, "index.html");
    }

    replace_URL_encodings(path_without_slash);

    // Print the parsed fields (for debugging purposes)
    printf("Method: %s\n", method);
    printf("Path w/o slash: %s\n", path_without_slash);
    printf("HTTP Version: %s\n", http_version);
    printf("File type: %s\n", extract_file_type(path_without_slash));
    //printf("-------");

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    if (strcmp(extract_file_type(path_without_slash), "ts") == 0) {
        proxy_remote_file(app, client_socket, request);
    } else { 
    // may need to replace "path_without_slash" with "file_name"
        serve_local_file(client_socket, path_without_slash);
    }
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        // File not found, send a 404 response
        char response[] = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: text/plain\r\n"
                          "Content-Length: 14\r\n"
                          "\r\n"
                          "File not found";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    // File found, read its contents to determine content length
    fseek(file, 0, SEEK_END);
    long content_length = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Determine file type of the file
    const char *file_type = extract_file_type(path);

    // Construct response headers
    char headers[1024];
    // If requesting a txt file
    if (strcmp(file_type, "txt") == 0) {
        sprintf(headers, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: %ld\r\n"
                        "\r\n", content_length);

        // Send response headers
        send(client_socket, headers, strlen(headers), 0);
    // If requesting a html file
    } else if (strcmp(file_type, "html") == 0) {
        sprintf(headers, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %ld\r\n"
                        "\r\n", content_length);

        // Send response headers
        send(client_socket, headers, strlen(headers), 0);
    // If requesting a jpg file
    } else if (strcmp(file_type, "jpg") == 0) {
        sprintf(headers, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: image/jpeg\r\n"
                        "Content-Length: %ld\r\n"
                        "\r\n", content_length);

        // Send response headers
        send(client_socket, headers, strlen(headers), 0);
    // Everything else, just send the binaries
    } else {
        sprintf(headers, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: application/octet-stream\r\n"
                        "Content-Length: %ld\r\n"
                        "\r\n", content_length);

        // Send response headers
        send(client_socket, headers, strlen(headers), 0);
    }

    // Send file content
    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    // Close the file
    fclose(file);
}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    // char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    // send(client_socket, response, strlen(response), 0);
    
    // Step one: establish connection with remote server
    ssize_t bytes_sent, bytes_received;
    char response_buffer[BUFFER_SIZE];
    int remote_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (remote_socket < 0) {
        perror("socket failed");
        return;
    }

    char bad_gateway_response[] = "HTTP/1.1 502 Bad Gateway\r\n"
                                  "Content-Type: text/plain\r\n"
                                  "Content-Length: 11\r\n"
                                  "\r\n"
                                  "Bad Gateway";

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_addr.s_addr = inet_addr(app->remote_host);
    remote_addr.sin_port = htons(app->remote_port);
    
    if (inet_aton(app->remote_host, &remote_addr.sin_addr) <= 0) {
        perror("inet_aton failed");
        close(remote_socket);
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    if (connect(remote_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) < 0) {
        perror("connect failed");
        close(remote_socket);
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    // Step two: forward original request to remote server
    if (send(remote_socket, request, strlen(request), 0) < 0) {
        perror("send failed");
        close(remote_socket);
        send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
        return;
    }

    // Step three: receive response from remote server, and forward it to client
    // char response_to_send[BUFFER_SIZE] = {0};
    // int num_bytes_read = 0;
    // while (num_bytes_read = recv(remote_socket, response_to_send, sizeof(response_to_send), 0) > 0) {
    //     if (send(client_socket, response_to_send, num_bytes_read, 0) == -1) {
    //         perror("send failed");
    //         close(remote_socket);
    //         return;
    //     }
    //     memset(response_to_send, 0, BUFFER_SIZE);
    // }
    while(1)
    {
        // Clear buffer then read from remote
        memset(response_buffer, 0, BUFFER_SIZE);
        bytes_received = recv(remote_socket, response_buffer, sizeof(response_buffer), 0);
        if (bytes_received == -1) {
            perror("recv failed");
            close(remote_socket);
            send(client_socket, bad_gateway_response, strlen(bad_gateway_response), 0);
            return;
        }
        if (bytes_received == 0) {
            perror("recv ended");
            close(remote_socket);
            return;
        }

        // Send the content of the .ts file to the client
        bytes_sent = send(client_socket, response_buffer, bytes_received, 0);
        if (bytes_sent == -1) {
            perror("send to client failed");
            close(remote_socket);
            return;
        }
    }
}