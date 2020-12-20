#define FUSE_MAX_LOG_ENTRY (1024 * 1024 * 1024)

typedef enum fuse_event {
	fl_vfs_write_enter,
	fl_fuse_direct_io_enter,
	fl_fuse_get_user_pages_begin,  // page pinning 
	fl_fuse_get_user_pages_end,
	fl_fuse_enqueue_begin,
	fl_fuse_enqueue_end,
} fuse_event_t;

struct fuse_log_entry {
	long long time;  // ktime_t, time as nanoseconds
	void *identifier;
	fuse_event_t event;
};
