#include "socket/FileDescriptor.hpp"
#include "socket/PosixError.hpp"
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

bool wrap_select(
	  int fd
	, long timeout_secs
	, int timeout_usecs
	, bool select_read
	, bool select_write
	, bool select_except
) {
	while(true) {
		timeval deadline;
		deadline.tv_sec = timeout_secs;
		deadline.tv_usec = timeout_usecs;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		errno = 0;
		int r = select(
			  fd + 1
			, select_read   ?  &fds : 0
			, select_write  ?  &fds : 0
			, select_except ?  &fds : 0
			, &deadline
		);
		switch(r) {
		case -1:
			if(errno == EINTR) {
				continue;
			}
			throw PosixError("Error select", errno);
		case 0:
			return false;
		default:
			return FD_ISSET(fd, &fds); // true!!!
		}
	}
}

FileDescriptor::FileDescriptor()
	: _fd(-1)
{}

FileDescriptor::FileDescriptor(int fd)
	: _fd(fd)
{}

FileDescriptor::FileDescriptor(FileDescriptor&& o)
	: _fd(o._fd)
{
	o._fd = -1;
}

FileDescriptor::~FileDescriptor() {
	reassign(-1);
}

void FileDescriptor::reassign(int new_fd) {
	close();
	_fd = new_fd;
}

bool FileDescriptor::is_valid() const {
	return _fd > -1;
}

bool FileDescriptor::can_read(long timeout_secs, int timeout_usecs) const {
	return wrap_select(_fd, timeout_secs, timeout_usecs, true, false, false);
}

bool FileDescriptor::can_write(long timeout_secs, int timeout_usecs) const {
	return wrap_select(_fd, timeout_secs, timeout_usecs, false, true, false);
}

bool FileDescriptor::can_except(long timeout_secs, int timeout_usecs) const {
	return wrap_select(_fd, timeout_secs, timeout_usecs, false, false, true);
}

int FileDescriptor::fd() const {
	return _fd;
}

void FileDescriptor::set_non_blocking ( bool b ) {
	errno = 0;
	int opts = fcntl ( _fd, F_GETFL );
	if ( opts < 0 ) {
		throw PosixError( "Error: set_non_blocking, get_opts", errno);
		return;
	}
	if ( b ) {
		opts |= O_NONBLOCK;
	}
	else {
		opts &= ~O_NONBLOCK;
	}
	errno = 0;
	opts = fcntl ( _fd, F_SETFL, opts );
	if ( opts < 0 ) {
		throw PosixError( "Error: set_non_blocking, set_opts", errno);
	}
}

void FileDescriptor::close() {
	if(is_valid()) {
		errno = 0;
		int r = ::close(_fd);
		if(r < 0) {
			throw PosixError( "Error: close", errno);
		}
		_fd = -1;
	}
}
