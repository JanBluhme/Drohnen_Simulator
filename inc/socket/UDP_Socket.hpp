#pragma once
#include <cstdint>
#include "socket/Socket_Base.hpp"

class UDP_Socket : public Socket_Base {
public:
	UDP_Socket()
		: Socket_Base(SOCK_DGRAM)
	{}
	void bind(int port) {
		socket().bind(port);
	}
	void enable_broadcast(bool enable) {
		socket().enable_broadcast(enable);
	}
	void enable_reuse_address(bool enable) {
		socket().enable_reuse_address(enable);
	}
	void join_multicast_group(std::string const& group) {
		socket().join_multicast_group(group);
	}
	uint64_t sendto(const Address& dst, const void* buffer, uint64_t size ) const {
		return socket().sendto(dst, buffer, size);
	}
	uint64_t recvfrom(Address& src, void* buffer, uint64_t size) const {
		return socket().recvfrom(src, buffer, size);
	}
	uint64_t peekfrom(Address& src, void* buffer, uint64_t size) const {
		return socket().peekfrom(src, buffer, size);
	}
	uint64_t recvfrom_nonblocking(Address& src, void* buffer, uint64_t size) const {
		return socket().recvfrom_nonblocking(src, buffer, size);
	}
	uint64_t peekfrom_nonblocking(Address& src, void* buffer, uint64_t size) const {
		return socket().peekfrom_nonblocking(src, buffer, size);
	}
};

class UDP_ServerSocket : public UDP_Socket {
public:
	UDP_ServerSocket(int port) {
		socket().bind(port);
	}
};

class UDP_ClientSocket : public UDP_Socket {
public:
	UDP_ClientSocket(const std::string& host, int port) {
		socket().connect(host, port);
	}
	uint64_t send(const void* buffer, uint64_t size) const {
		return socket().send(buffer, size);
	}
	uint64_t recv(void* buffer, uint64_t size) const {
		return socket().recv(buffer, size);
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
