#pragma once
#include <cstdint>
#include "socket/Socket_Base.hpp"

class TCP_Socket : public Socket_Base {
	friend class TCP_ServerSocket;

public:
	TCP_Socket()
		: Socket_Base(SOCK_STREAM)
	{}
	uint64_t send(const void* buffer, uint64_t size) const {
		return socket().send(buffer, size);
	}
	uint64_t recv(void* buffer, uint64_t size) const {
		return socket().recv(buffer, size);
	}
	uint64_t recv_exact(void* buffer, uint64_t size) const {
		return socket().recv_exact(buffer, size);
	}
	uint64_t peek(void* buffer, uint64_t size) const {
		return socket().peek(buffer, size);
	}
	uint64_t recv_nonblocking(void* buffer, uint64_t size) const {
		return socket().recv_nonblocking(buffer, size);
	}
	uint64_t peek_nonblocking(void* buffer, uint64_t size) const {
		return socket().peek_nonblocking(buffer, size);
	}
};

class TCP_ServerSocket : public TCP_Socket {
public:
	TCP_ServerSocket(int port, bool enable_reuse_address = false, int max_connections = 128) {
		if(enable_reuse_address) {
			socket().enable_reuse_address(true);
		}
		socket().bind(port);
		socket().listen(max_connections);
	}
	void accept(TCP_Socket& new_socket) const {
		socket().accept(new_socket.socket());
	}
};

class TCP_ClientSocket : public TCP_Socket {
public:
	TCP_ClientSocket(const std::string& host, int port) {
		socket().connect(host, port);
	}
	TCP_ClientSocket(const std::string& host, int port, int max_tries) {
		socket().connect(host, port, max_tries);
	}
};
