#include "socket/Address.hpp"
#include "socket/PosixError.hpp"
#include <cerrno>
#include <cstring>

sockaddr* Address::sockaddr_ptr() const {
	return (sockaddr*)(&_addr);
}

size_t Address::size() const {
	return sizeof(_addr);
}

void Address::clear() {
	memset(&_addr, 0, size());
}

Address::Address() {
	clear();
}

Address::Address(const std::string& host, int port) {
	clear();
	resolve(host, port);
}

Address::Address(int port) {
	clear();
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port);
	_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
}

void Address::resolve(const std::string& host, int port) {
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons ( port );
	errno = 0;
	int status = inet_pton( AF_INET, host.c_str(), &_addr.sin_addr );
	if(status != 1) {
		if(errno) {
			throw PosixError("Can't resolve host", errno);
		}
	}
}

std::ostream& operator<<(std::ostream& o, const Address& a) {
	static char digits[] = "0123456789ABCDEF";
	std::string r(3*a.size(), 0 );
	const unsigned char* data = (const unsigned char*)a.sockaddr_ptr();
	for(size_t i = 0; i < a.size(); ++i) {
		size_t c = 3*i;
		r[c++] = digits[(data[i] >> 4) & 0x0F];
		r[c++] = digits[(data[i] >> 0) & 0x0F];
		r[c  ] = ' ';
	}
	return o << r << "\n";
}
