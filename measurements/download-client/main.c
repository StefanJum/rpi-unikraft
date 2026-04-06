#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <unistd.h>
#include <string.h> 
#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1000000
#define TIMES_TO_SEND_BUFFER 10
#define REPEAT_TEST 100
#define PORT 8080

void fill_buffer_with_random_values(char *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        buffer[i] = rand() % 256;
    }
}

static void millisleep(unsigned int millisec)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = millisec / 1000;
	ts.tv_nsec = (millisec % 1000) * 1000000;
	do
		ret = nanosleep(&ts, &ts);
	while (ret && errno == EINTR);
}

char buffer[BUFFER_SIZE];
int main(int argc, char *argv[])
{
    setvbuf(stdout, NULL, _IONBF, 0);
	printf("Hello world!\n");

    int sockfd, connfd;
    struct sockaddr_in servaddr, cli;
 
    // socket create and verification
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        printf("socket creation failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully created..\n");
    bzero(&servaddr, sizeof(servaddr));
 
    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("192.168.178.221");
    servaddr.sin_port = htons(PORT);
 
    // connect the client socket to server socket
    int res = connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if (res != 0) {
        printf("connection with the server failed... %d\n", res);
        exit(0);
    }
    else
        printf("connected to the server..\n");
 
    fill_buffer_with_random_values(buffer, BUFFER_SIZE);

    for (size_t j = 0; j < REPEAT_TEST; j++) {    
        for (int i = 0; i < TIMES_TO_SEND_BUFFER; i++) {
            ssize_t bytes_written = 0;
            ssize_t total_bytes_written = 0;

            while (total_bytes_written < BUFFER_SIZE) {
                bytes_written = write(sockfd, buffer + total_bytes_written, BUFFER_SIZE - total_bytes_written);
                if (bytes_written < 0) {
                    perror("write");
                    close(sockfd);
                    exit(EXIT_FAILURE);
                }

                total_bytes_written += bytes_written;
            }
        }

        printf("Finished test %zu will wait for 1 seconds.\n", j);
        millisleep(1000);
    }

    printf("Will close the connection with the server in 10 seconds.\n");

    millisleep(10000);
    close(sockfd);
    printf("Done sleeping!\n");

	return 0;
}
