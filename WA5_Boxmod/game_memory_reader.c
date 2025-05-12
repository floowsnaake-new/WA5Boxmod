#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <pid> <hex_address> <output_format>\n", argv[0]);
        printf("Example: %s 1234 0x0183617\n", argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = atoi(argv[1]);
    unsigned long address = strtoul(argv[2], NULL, 16);
    // Optional: specify output format (e.g., hex or decimal)
    // For simplicity, we'll assume hex output

    // Open the mem file
    char mem_path[64];
    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    int mem_fd = open(mem_path, O_RDONLY);
    if (mem_fd == -1) {
        perror("Failed to open mem");
        return EXIT_FAILURE;
    }

    // Seek to the specified address
    if (lseek(mem_fd, address, SEEK_SET) == -1) {
        perror("Failed to seek");
        close(mem_fd);
        return EXIT_FAILURE;
    }

    // Read 2 bytes
    unsigned char buffer[2];
    ssize_t bytes_read = read(mem_fd, buffer, 2);
    if (bytes_read != 2) {
        perror("Failed to read");
        close(mem_fd);
        return EXIT_FAILURE;
    }

    close(mem_fd);

    // Print the read bytes
    printf("Bytes at address 0x%lX: 0x%02X 0x%02X\n", address, buffer[0], buffer[1]);

    return 0;
}
