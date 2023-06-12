#pragma once

class FileDescriptor {
private:
	int _fd;

public:
	FileDescriptor(FileDescriptor const&) = delete;
	FileDescriptor& operator=(FileDescriptor const&) = delete;
	
	FileDescriptor();
	FileDescriptor(int fd);
	FileDescriptor(FileDescriptor&& o);
	~FileDescriptor();
	void reassign(int new_fd);
	bool is_valid() const;
	bool can_read(long timeout_secs, int timeout_usecs) const;
	bool can_write(long timeout_secs, int timeout_usecs) const;
	bool can_except(long timeout_secs, int timeout_usecs) const;
	int fd() const;
	void set_non_blocking ( bool b );
	void close();
};
