#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#define INPUT_DIR "/dev/input"

int get_device_name(int fd, char *name, size_t size) {
    if (ioctl(fd, EVIOCGNAME(size), name) < 0) {
        perror("ioctl EVIOCGNAME");
        return -1;
    }
    return 0;
}

int main() {

printf("Playstation 4 Controller Input Tester\nType 0 and 4 Disabled for now\n\n");

    DIR *dir;
    struct dirent *entry;
    int device_fds[256];
    char device_paths[256][64];
    int device_count = 0;

    // Open /dev/input directory
    dir = opendir(INPUT_DIR);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    // Search for device matching the name
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            char path[128];
            snprintf(path, sizeof(path), "%s/%s", INPUT_DIR, entry->d_name);
            int fd = open(path, O_RDONLY);
            if (fd < 0) {
                // Unable to open device, skip
                continue;
            }

            char name[256];
            if (get_device_name(fd, name, sizeof(name)) == 0) {
                // Check if device name matches
                if (strcmp(name, "Sony Interactive Entertainment Wireless Controller") == 0) {
                    printf("Found PS4 Controller: %s\n", path);
                    // Store for monitoring
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

    // Monitor input events
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
                    // Filter out events with type 0 and 3
                    if (ev.type != 0 && ev.type != 3) {
                        printf("Event from %s: Type=%u, Code=%u, Value=%d\n",
                            device_paths[i], ev.type, ev.code, ev.value);
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
