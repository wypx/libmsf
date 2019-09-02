/*
* FreeBSD file AIO features and quirks:
*
* if an asked data are already in VM cache, then aio_error() returns 0,
* and the data are already copied in buffer;
*
* aio_read() preread in VM cache as minimum 16K (probably BKVASIZE);
* the first AIO preload may be up to 128K;
*
* aio_read/aio_error() may return EINPROGRESS for just written data;
*
* kqueue EVFILT_AIO filter is level triggered only: an event repeats
* until aio_return() will be called;
*
* aio_cancel() cannot cancel file AIO: it returns AIO_NOTCANCELED always.
*/



