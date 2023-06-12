#pragma once
#include <arpa/inet.h>
#include <string>
#include <cstddef>
#include <ostream>

class Address{
	friend class Socket_impl;

private:
	sockaddr_in _addr;

private:
	sockaddr* sockaddr_ptr() const;
	size_t size() const;
	void clear();
	void resolve(const std::string& host, int port);

public:
	Address();
	Address(const std::string& host, int port);

	// broadcast
	Address(int port);

	friend
	std::ostream& operator<<(std::ostream& o, const Address& a);
};
