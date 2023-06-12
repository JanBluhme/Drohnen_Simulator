#pragma once
#include "socket/Socket_impl.hpp"
#include "socket/PosixError.hpp"

class Socket_Base {
private:
	Socket_impl _socket;

protected:
	Socket_impl& socket() {
		return _socket;
	}
	const Socket_impl& socket() const {
		return _socket;
	}
public:
	Socket_Base(int socket_type) {
		_socket.create(socket_type);
	}
	bool can_read(long timeout_secs, int timeout_usecs) const {
		return _socket.can_read(timeout_secs, timeout_usecs);
	}
	bool can_write(long timeout_secs, int timeout_usecs) const {
		return _socket.can_write(timeout_secs, timeout_usecs);
	}
	void close() {
		_socket.close();
	}
	bool is_valid() const {
		return _socket.is_valid();
	}
	void shutdown_recv() {
		_socket.shutdown_recv();
	}
	void shutdown_send() {
		_socket.shutdown_send();
	}
	void shutdown() {
		_socket.shutdown();
	}
};
