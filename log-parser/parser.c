#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fuse_log.h"

#define DEVICE_PATH "/dev/fuse_log"
#define MAX_IDENTIFIERS 1024

enum state {
    initial = 0,
    vfs,
    fuse_direct_io,
    page_pin,
    before_enqueue,
    enqueue,
};

#define TIME_VFS 0
#define TIME_PAGE_PIN 1
#define TIME_ENQUEUE 2
#define TIME_LAST TIME_ENQUEUE

static void *known_identifiers[MAX_IDENTIFIERS];
static enum state states[MAX_IDENTIFIERS];
static long long timestamps[MAX_IDENTIFIERS];
static long long time_sum[TIME_LAST + 1];
static size_t num_identifiers = 0;

ssize_t read_next_entry (int fd, struct fuse_log_entry *entry) {
    ssize_t read_size = read(fd, entry, sizeof(struct fuse_log_entry));

    if (read_size < sizeof(struct fuse_log_entry) && read_size > 0) {
        while (read_size < sizeof(struct fuse_log_entry)) {
            ssize_t current_read_size = read(fd, (char *)entry + read_size, sizeof(struct fuse_log_entry) - read_size);
            if (current_read_size > 0) {
                read_size += current_read_size;
            } else {
                return current_read_size;
            }
        }
        return read_size;
    } else {
        return read_size;
    }
}

static inline size_t get_identifier_index(void *identifier) {
#ifdef SEPARATE_IDENTIFIERS
    for (size_t i = 0; i < num_identifiers; i++) {
        if (known_identifiers[i] == identifier) {
            return i;
        }
    }

    num_identifiers++;
    if (num_identifiers < MAX_IDENTIFIERS) {
        known_identifiers[num_identifiers - 1] = identifier;
        return num_identifiers - 1;
    } else {
        fprintf(stderr, "Maximum identifier reached.");
        exit(1);
    }
#else
    return 0;
#endif
}

int main(int argc, char *argv[]) {
    int log_fd = open(DEVICE_PATH, O_RDONLY);
    struct fuse_log_entry entry;

    while (read_next_entry(log_fd, &entry) > 0) {
        size_t index = get_identifier_index(entry.identifier);
        switch (states[index]) {
            case initial:
                if (entry.event == fl_vfs_write_enter) {
                    states[index] = vfs;
                    timestamps[index] = entry.time;
                    break;
                } else {
                    goto invalid_state;
                }
            case vfs:
                if (entry.event == fl_fuse_direct_io_enter) {
                    states[index] = fuse_direct_io;
                    time_sum[TIME_VFS] += entry.time - timestamps[index];
                    break;
                } else {
                    goto invalid_state;
                }
            case fuse_direct_io:
                if (entry.event == fl_fuse_get_user_pages_begin) {
                    states[index] = page_pin;
                    timestamps[index] = entry.time;
                    break;
                } else {
                    goto invalid_state;
                }
            case page_pin:
                if (entry.event == fl_fuse_get_user_pages_end) {
                    states[index] = before_enqueue;
                    time_sum[TIME_ENQUEUE] += entry.time - timestamps[index];
                    break;
                } else {
                    goto invalid_state;
                }
            case before_enqueue:
                if (entry.event == fl_fuse_enqueue_begin) {
                    states[index] = enqueue;
                    timestamps[index] = entry.time;
                    break;
                } else {
                    goto invalid_state;
                }
            case enqueue:
                if (entry.event == fl_fuse_enqueue_end) {
                    states[index] = initial;
                    time_sum[TIME_ENQUEUE] += entry.time - timestamps[index];
                    break;
                } else {
                    goto invalid_state;
                }
            default:
                goto invalid_state;
        }

        continue;

invalid_state:
        fprintf(stderr, "State error for identifier %p, state: %d, event: %d\n", entry.identifier, states[index], entry.event);
        states[index] = initial;
    }

    printf("TIME_VFS: %lld\n", time_sum[TIME_VFS]);
    printf("TIME_PAGE_PIN: %lld\n", time_sum[TIME_PAGE_PIN]);
    printf("TIME_ENQUEUE: %lld\n", time_sum[TIME_ENQUEUE]);

    return 0;
}

