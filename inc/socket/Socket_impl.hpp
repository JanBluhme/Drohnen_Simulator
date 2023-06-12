#pragma once
#include "socket/Address.hpp"
#include "socket/FileDescriptor.hpp"
#include <string>
#include <cstdint>
#include <mutex>

class Socket_impl {
public:
	Socket_impl();
	Socket_impl(int socket_type);
	Socket_impl(Socket_impl&& o);
	~Socket_impl();

	void create(int socket_type);
	void enable_broadcast(bool enable) const;
	void enable_reuse_address(bool enable) const;
	void join_multicast_group(std::string const& group) const;

	void close();
	void shutdown_recv();
	void shutdown_send();
	void shutdown();
	void bind(int port);
	void listen(int max_connections) const;
	void accept(Socket_impl& new_socket) const;
	
	bool is_valid() const ;

	void connect(const std::string host, const int port) const;
	void connect(const std::string host, const int port, int max_tries) const;

	uint64_t send(const void* buffer, uint64_t size) const;
	uint64_t peek(void* buffer, uint64_t size) const;
	uint64_t recv(void* buffer, uint64_t size) const;
	uint64_t recv_exact(void* buffer, uint64_t size) const;
	uint64_t recv_nonblocking(void* buffer, uint64_t size) const;
	uint64_t peek_nonblocking(void* buffer, uint64_t size) const;
	
	uint64_t sendto(const Address& dst, const void* buffer, uint64_t size) const;
	uint64_t peekfrom(Address& src, void* buffer, uint64_t size) const;
	uint64_t recvfrom(Address& src, void* buffer, uint64_t size) const;
	uint64_t recvfrom_nonblocking(Address& src, void* buffer, uint64_t size) const;
	uint64_t peekfrom_nonblocking(Address& src, void* buffer, uint64_t size) const;
	
	bool can_read(long timeout_secs, int timeout_usecs) const;
	bool can_write(long timeout_secs, int timeout_usecs) const;

private:
	FileDescriptor m_sock;
	Address m_addr;
};
