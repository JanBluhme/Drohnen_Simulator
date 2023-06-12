#include "socket/Socket_impl.hpp"
#include "socket/PosixError.hpp"
#include <stdexcept>
#include <cerrno>
#include <unistd.h>

Socket_impl::Socket_impl()
	: m_sock( -1 )
{}

Socket_impl::Socket_impl(int socket_type)
	: m_sock( -1 )
{
	create(socket_type);
}

Socket_impl::Socket_impl(Socket_impl&& o)
	: m_sock(std::move(o.m_sock))
	, m_addr(o.m_addr)
{}

Socket_impl::~Socket_impl() {
	close();
}

bool Socket_impl::is_valid() const {
	return m_sock.is_valid();
}

void Socket_impl::close() {
	m_sock.close();
}

void Socket_impl::shutdown_recv() {
	if(is_valid()) {
		::shutdown(m_sock.fd(), SHUT_RD);
	}
}
void Socket_impl::shutdown_send() {
	if(is_valid()) {
		::shutdown(m_sock.fd(), SHUT_WR);
	}
}
void Socket_impl::shutdown() {
	if(is_valid()) {
		::shutdown(m_sock.fd(), SHUT_RDWR);
	}
}

void Socket_impl::join_multicast_group(std::string const& group) const {
	ip_mreq mreq;
	mreq.imr_multiaddr.s_addr = inet_addr(group.c_str());
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	errno = 0;
	int setsockopt_return = setsockopt(
		  m_sock.fd()
		, IPPROTO_IP
		, IP_ADD_MEMBERSHIP
		, (char*) &mreq
		, sizeof(mreq)
	);
	if( setsockopt_return == -1 ) {
		throw PosixError("Can't setsockopt (IP_ADD_MEMBERSHIP)", errno);
	}
}

void Socket_impl::enable_reuse_address(bool enable) const {
	int on = enable ? 1 : 0;
	errno = 0;
	int setsockopt_return = setsockopt (
		  m_sock.fd()
		, SOL_SOCKET
		, SO_REUSEADDR
		, (const char*)(&on)
		, sizeof ( on )
	);
	if( setsockopt_return == -1 ) {
		throw PosixError("Can't setsockopt (SO_REUSEADDR)", errno);
	}
}

void Socket_impl::create(int socket_type) {
	errno = 0;
	int new_fd = ::socket(AF_INET, socket_type | SOCK_CLOEXEC, 0);
	if(new_fd == -1) {
		throw PosixError("Can't create socket", errno);
	}
	m_sock.reassign(new_fd);
}

void Socket_impl::enable_broadcast(bool enable) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't enable_broadcast on invalid socket");
	}
	int broadcastEnable = enable ? 1 : 0;
	errno = 0;
	int setsockopt_return = setsockopt(
		  m_sock.fd()
		, SOL_SOCKET
		, SO_BROADCAST
		, (const char*)(&broadcastEnable)
		, sizeof(broadcastEnable)
	);
	if( setsockopt_return == -1 ) {
		throw PosixError("Can't setsockopt (SO_BROADCAST)", errno);
	}
}

void Socket_impl::bind(const int port) {
	if( !is_valid() ) {
		throw std::runtime_error("Can't bind invalid socket");
	}
	m_addr._addr.sin_family = AF_INET;
	m_addr._addr.sin_port = htons ( port );
	m_addr._addr.sin_addr.s_addr = htonl(INADDR_ANY);
	errno = 0;
	int bind_return = ::bind(m_sock.fd(), m_addr.sockaddr_ptr(), m_addr.size());
	if( bind_return == -1 ) {
		throw PosixError("Can't bind", errno);
	}
}

void Socket_impl::listen(int max_connections) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't listen on invalid socket");
	}
	errno = 0;
	int listen_return = ::listen(m_sock.fd(), max_connections);
	if( listen_return == -1 ) {
		throw PosixError("Error listen", errno);
	}
}

void Socket_impl::accept(Socket_impl& new_socket) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't accept on invalid socket");
	}
	socklen_t s = new_socket.m_addr.size();
	errno = 0;
	int new_fd = ::accept(m_sock.fd(), new_socket.m_addr.sockaddr_ptr(), &s);
	if( new_fd == -1 ) {
		throw PosixError("Can't accept", errno);
	}
	new_socket.m_sock.reassign(new_fd);
}

void Socket_impl::connect(const std::string host, const int port) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't connect invalid socket");
	}
	Address partner;
	partner.resolve(host, port);
	errno = 0;
	int status = ::connect ( m_sock.fd(), partner.sockaddr_ptr(), partner.size() );
	if( status == -1 ) {
		throw PosixError("Can't connect", errno);
	}
}
void Socket_impl::connect(const std::string host, const int port, int max_tries) const {
	PosixError ee("",0);
	while(max_tries) {
		--max_tries;
		try {
			connect(host, port);
			return;
		} catch(const PosixError& e) {
			if(max_tries == 0) {
				ee = e;
			}
			usleep(100000);
		}
	}
	throw ee;
}

uint64_t Socket_impl::send(const void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't send on invalid socket");
	}
	uint64_t s = 0;
	while(s != size) {
		errno = 0;
		ssize_t status = ::send( m_sock.fd(), ((uint8_t*)buffer) + s, size - s, MSG_NOSIGNAL );
		if( status == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			throw PosixError("Error: send", errno);
		}
		s += status;
	}
	return s;
}

uint64_t Socket_impl::sendto(const Address& dst, const void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't sentto on invalid socket");
	}
	while(true) {
		errno = 0;
		ssize_t r = ::sendto(m_sock.fd(), buffer, size, 0, dst.sockaddr_ptr(), dst.size());
		if(r == -1) {
			if(errno != EINTR) {
				continue;
			}
			throw PosixError("Error: sendto", errno);
		}
		return (uint64_t)r;
	}
}

uint64_t Socket_impl::recv(void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't recv on invalid socket");
	}
	while(true) {
		errno = 0;
		ssize_t status = ::recv( m_sock.fd(), ((uint8_t*)buffer), size, 0 );
		if( status == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			throw PosixError("Error: recv", errno);
		}
		return status;
	}
}

uint64_t Socket_impl::recv_exact(void* buffer, uint64_t size) const {
	uint64_t r = 0;
	while(r != size) {
		uint64_t status = recv(((uint8_t*)buffer) + r, size - r);
		r += status;
		if(status == 0) {
			break;
		}
	}
	return r;
}

uint64_t Socket_impl::recv_nonblocking(void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't recv_nonblocking on invalid socket");
	}
	while(true) {
		errno = 0;
		ssize_t status = ::recv( m_sock.fd(), buffer, size, MSG_DONTWAIT );
		if( status == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			if(		(errno == EAGAIN)
				||	(errno == EWOULDBLOCK)
			) {
				return -1;
			}
			throw PosixError("Error: recv", errno);
		}
		return status;
	}
}

uint64_t Socket_impl::peek(void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't peek on invalid socket");
	}
	while(true) {
		errno = 0;
		ssize_t status = ::recv( m_sock.fd(), buffer, size, MSG_PEEK );
		if( status == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			throw PosixError("Error: peek", errno);
		}
		return (uint64_t)status;
	}
}

uint64_t Socket_impl::peek_nonblocking(void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't peek_nonblocking on invalid socket");
	}
	while(true) {
		errno = 0;
		ssize_t status = ::recv( m_sock.fd(), buffer, size, MSG_PEEK | MSG_DONTWAIT );
		if( status == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			if(		(errno == EAGAIN)
				||	(errno == EWOULDBLOCK)
			) {
				return -1;
			}
			throw PosixError("Error: peek", errno);
		}
		return (uint64_t)status;
	}
}

uint64_t Socket_impl::recvfrom(Address& src, void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't recvfrom on invalid socket");
	}
	socklen_t s = src.size();
	while(true) {
		errno = 0;
		ssize_t r = ::recvfrom(m_sock.fd(), buffer, size, 0, src.sockaddr_ptr(), &s);
		if(r == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			throw PosixError("Error: recvfrom", errno);
		}
		return (uint64_t)r;
	}
}

uint64_t Socket_impl::recvfrom_nonblocking(Address& src, void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't recvfrom_nonblocking on invalid socket");
	}
	socklen_t s = src.size();
	while(true) {
		errno = 0;
		ssize_t status = ::recvfrom( m_sock.fd(), buffer, size, MSG_DONTWAIT, src.sockaddr_ptr(), &s);
		if( status == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			if(		(errno == EAGAIN)
				||	(errno == EWOULDBLOCK)
			) {
				return -1;
			}
			throw PosixError("Error: recvfrom_nonblocking", errno);
		}
		return status;
	}
}

uint64_t Socket_impl::peekfrom(Address& src, void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't peekfrom on invalid socket");
	}
	socklen_t s = src.size();
	while(true) {
		errno = 0;
		ssize_t r = ::recvfrom(m_sock.fd(), buffer, size, MSG_PEEK, src.sockaddr_ptr(), &s);
		if(r == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			throw PosixError("Error: peekfrom", errno);
		}
		return (uint64_t)r;
	}
}

uint64_t Socket_impl::peekfrom_nonblocking(Address& src, void* buffer, uint64_t size) const {
	if( !is_valid() ) {
		throw std::runtime_error("Can't peekfrom_nonblocking on invalid socket");
	}
	socklen_t s = src.size();
	while(true) {
		errno = 0;
		ssize_t status = ::recvfrom( m_sock.fd(), buffer, size, MSG_PEEK | MSG_DONTWAIT, src.sockaddr_ptr(), &s);
		if( status == -1 ) {
			if(errno == EINTR) {
				continue;
			}
			if(		(errno == EAGAIN)
				||	(errno == EWOULDBLOCK)
			) {
				return -1;
			}
			throw PosixError("Error: peekfrom_nonblocking", errno);
		}
		return (uint64_t)status;
	}
}

bool Socket_impl::can_read(long timeout_secs, int timeout_usecs) const {
	return m_sock.can_read(timeout_secs, timeout_usecs);
}

bool Socket_impl::can_write(long timeout_secs, int timeout_usecs) const {
	return m_sock.can_write(timeout_secs, timeout_usecs);
}
