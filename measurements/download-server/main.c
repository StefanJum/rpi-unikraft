#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 1000000
#define TIMES_TO_DOWNLOAD_BUFFER 10
#define REPEAT_TEST 100

// TODO: Try to turn of wifi on the mac? (This will prevent useless requests like arp, broadcast, unicast, etc. from reaching the PI and causing interrupts)

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
double timesTaken[REPEAT_TEST];
int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    srand(time(NULL));

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Got connection. Will start receiving\n");

    for (size_t j = 0; j < REPEAT_TEST; j++) {    
        struct timespec start, end;

        for (int i = 0; i < TIMES_TO_DOWNLOAD_BUFFER; i++) {
            ssize_t bytes_read = 0;
            ssize_t total_bytes_read = 0;

            while (total_bytes_read < BUFFER_SIZE) {
                bytes_read = read(new_socket, buffer + total_bytes_read, BUFFER_SIZE - total_bytes_read);
                if (bytes_read < 0) {
                    perror("read");
                    close(new_socket);
                    close(server_fd);
                    exit(EXIT_FAILURE);
                }

                if (total_bytes_read == 0 && i == 0) {
                    clock_gettime(CLOCK_MONOTONIC, &start);
                }

                total_bytes_read += bytes_read;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &end);
        timesTaken[j] = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    }


    printf("Done running %d test.\nReceived %d bytes per test.\nTimer per test in seconds:\n[", REPEAT_TEST, TIMES_TO_DOWNLOAD_BUFFER * BUFFER_SIZE);

    double total = 0;
    for (size_t i = 0; i < REPEAT_TEST; i++)
    {
        total += timesTaken[i];
        printf("%.5f", timesTaken[i]);
        if (i != REPEAT_TEST - 1) {
            printf(", ");
        }
    }
    printf("]\n");

    printf("Average time taken: %.5f seconds\n", total / REPEAT_TEST);
    double bits_per_second = (TIMES_TO_DOWNLOAD_BUFFER * BUFFER_SIZE * 8) / (total / REPEAT_TEST);
    double mega_bits_per_second = bits_per_second / 1000 / 1000;
    printf("%d bytes set per test. %d bits sent per test. %.2f bits per second average = %.4f megabits\n", TIMES_TO_DOWNLOAD_BUFFER * BUFFER_SIZE, TIMES_TO_DOWNLOAD_BUFFER * BUFFER_SIZE * 8, bits_per_second, mega_bits_per_second);

    printf("Succesfully received all data will wait for 10 seconds before closing the conneciton\n");
    millisleep(10000);

    close(new_socket);
    close(server_fd);

    printf("exiting\n");

    return 0;
}
