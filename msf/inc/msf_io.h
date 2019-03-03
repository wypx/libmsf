
#include <msf_utils.h>
#include <msf_list.h>

struct io_buffer {
	u8 		*data;
	size_t 	size;
	size_t 	off;
	size_t 	ready;

	s32 	flags;
};


struct io_header {
	struct list_head ihdr_queue;
	struct list_head ihdr_parent;
	s32 ihdr_type;
	struct io_buffer ihdr_buf;
	s32 ihdr_flags;

	void (*ihdr_free)(struct io_obj *, void *);
	void *ihdr_freearg;
};

struct io_obj {
	struct io_header io_hdr;

	struct io_duplex *io_dplx;
	ssize_t (*io_special)(struct io_obj *, s32, void *, size_t);
	ssize_t (*io_method)(s32, void *, size_t);
	size_t io_blocksize;

	s32 io_timeout;
	//struct event io_ev;
};

struct io_duplex {
	struct io_obj *io_src;
	struct io_obj *io_dst;
};

struct io_filter {
	struct io_header io_hdr;
	/* keep above same as io_obj */

	s32 (*io_filter)(void *arg, struct io_filter *filt, 
			 struct io_buffer *in, s32 flags);
	void *io_state;

	struct io_buffer io_tmpbuf;
};

