#include <linux/fs.h>
#include <linux/uio.h>

ssize_t fuse_generic_file_write_iter(struct kiocb *iocb, struct iov_iter *from);
ssize_t fuse__generic_file_write_iter(struct kiocb *iocb, struct iov_iter *from);
