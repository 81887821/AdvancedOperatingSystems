#include <linux/ktime.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/uio.h>

#define FUSE_MAX_LOG_ENTRY (1024 * 1024 * 1024)

typedef enum fuse_event {
	fl_vfs_write_enter,
	fl_vfs_read_enter,
	fl_fuse_getxattr_enter,
	fl_fuse_direct_io_enter,
	fl_fuse_get_user_pages_begin, // page pinning 
	fl_fuse_get_user_pages_end,
	fl_fuse_enqueue_begin,
	fl_fuse_enqueue_end,
} fuse_event_t;

struct fuse_log_entry {
	ktime_t time;
	void *identifier;
	fuse_event_t event;
};

extern struct fuse_log_entry *fuse_logs;
extern atomic64_t fuse_num_logs;

void fuse_append_log(void *identifier, fuse_event_t event);
ssize_t fuse_logdev_read(struct file *file, char __user *buffer, size_t size, loff_t *offset);
