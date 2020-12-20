#include "fuse_log.h"

void fuse_append_log(void *identifier, fuse_event_t event)
{
	s64 index = atomic64_inc_return(&fuse_num_logs) - 1;
	ktime_t log_time = ktime_get();

	if (index < FUSE_MAX_LOG_ENTRY) {
		fuse_logs[index].time = log_time;
		fuse_logs[index].identifier = identifier;
		fuse_logs[index].event = event;
	}
}

ssize_t fuse_logdev_read(struct file *file, char __user *buffer, size_t size, loff_t *offset) {
	s64 num_logs = atomic64_read(&fuse_num_logs);

	if (*offset >= sizeof(struct fuse_log_entry) * num_logs) {
		return 0;
	} else {
		ssize_t size_to_read = min(sizeof(struct fuse_log_entry) * num_logs - *offset, size);

		printk("fuse: reading logs, num_logs: %lld, size: %ld\n", num_logs, size_to_read);
		copy_to_user(buffer, ((char *)fuse_logs) + *offset, size_to_read);
		*offset += size_to_read;

		return size_to_read;
	}
}
