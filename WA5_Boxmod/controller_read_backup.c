#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <dirent.h>
#include <string.h>
#include <sys/ioctl.h>
#include <stdint.h> // For int16_t

#define INPUT_DIR "/dev/input"

int get_device_name(int fd, char *name, size_t size) {
    if (ioctl(fd, EVIOCGNAME(size), name) < 0) {
        perror("ioctl EVIOCGNAME");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <target_pid> <hex_address>\n", argv[0]);
        printf("Example: %s 12275 0x561B7044D2CA\n", argv[0]);
        return EXIT_FAILURE;
    }

    int target_pid = atoi(argv[1]);
    // Convert the address string to unsigned long
    unsigned long mem_address = strtoul(argv[2], NULL, 16);

    printf("Monitoring PS4 controller. On button press, reading from PID=%d, address=0x%lX\n\n", target_pid, mem_address);

    DIR *dir;
    struct dirent *entry;
    int device_fds[256];
    char device_paths[256][64];
    int device_count = 0;

    // Search for PS4 Controller devices
    dir = opendir(INPUT_DIR);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", INPUT_DIR, entry->d_name);
            int fd = open(path, O_RDONLY);
            if (fd < 0) continue;

            char name[256];
            if (get_device_name(fd, name, sizeof(name)) == 0) {
                if (strcmp(name, "Sony Interactive Entertainment Wireless Controller") == 0) {
                    printf("Found PS4 Controller: %s\n", path);
                    strncpy(device_paths[device_count], path, sizeof(device_paths[device_count]));
                    device_fds[device_count] = fd;
                    device_count++;
                } else {
                    close(fd);
                }
            } else {
                close(fd);
            }
        }
    }
    closedir(dir);

    if (device_count == 0) {
        printf("Playstation 4 Controller not found.\n");
        return 0;
    }

    printf("Monitoring The Controller...\n");
    for (int i = 0; i < device_count; i++) {
        printf("%s\n", device_paths[i]);
    }

    // Main event loop
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        int max_fd = -1;
        for (int i = 0; i < device_count; i++) {
            FD_SET(device_fds[i], &readfds);
            if (device_fds[i] > max_fd)
                max_fd = device_fds[i];
        }

        int ret = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (ret < 0) {
            perror("select");
            break;
        }

        for (int i = 0; i < device_count; i++) {
            if (FD_ISSET(device_fds[i], &readfds)) {
                struct input_event ev;
                ssize_t n = read(device_fds[i], &ev, sizeof(ev));
                if (n == sizeof(ev)) {
                    // Check for button press event
                    if (ev.type == EV_KEY && ev.value == 1) { // button pressed
                        printf("Button pressed: Coordinate Y ",mem_address);
                        char mem_path[64];
                        snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", target_pid);

                        int mem_fd = open(mem_path, O_RDONLY);
                        if (mem_fd == -1) {
                            perror("Failed to open target process memory");
                            continue;
                        }

                        if (lseek(mem_fd, (off_t)mem_address, SEEK_SET) == -1) {
                            perror("Failed to seek to address");
                            close(mem_fd);
                            continue;
                        }

// Read 2 bytes from memory into buffer
unsigned char buffer[2];
ssize_t bytes_read = read(mem_fd, buffer, 2);
if (bytes_read != 2) {
    perror("Failed to read memory");
    close(mem_fd);
    continue;
}
close(mem_fd);

// Convert buffer to unsigned int16_t explicitly
uint16_t value = (uint16_t)((buffer[1] << 8) | buffer[0]);

// Output only the numeric value
printf("%u\n", value);
                    }
                }
            }
        }
    }

    // Close device fds
    for (int i = 0; i < device_count; i++) {
        close(device_fds[i]);
    }

    return 0;
}
