#pragma once
#include "util/name_this_thread.hpp"
#include "util/time_this.hpp"
#include "socket/TCP_Socket.hpp"
#include "make_command_set.hpp"
#include <algorithm>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdlib>

struct Bench {
	using clock_t = std::chrono::steady_clock;
	std::string_view name;
	clock_t::time_point start;
	uint64_t bytes;
	uint64_t packets;
	std::mutex mutex;
	
	Bench(std::string_view name)
		: name(name)
		, start(clock_t::now())
		, bytes(0)
		, packets(0)
	{}
	
	void add(uint64_t bytes) {
		std::lock_guard<std::mutex> lock(mutex);
		this->bytes += bytes;
		++packets;
	}
	
	~Bench() {
		auto print = [&](double value, std::string_view what) {
			char si_prefix[] = {' ', 'k', 'M', 'G', 'T'};
			std::cerr << "Bench: " << name << ": ";
			std::size_t p = 0;
			while(p < std::size(si_prefix) && value > 1024)  {
				++p;
				value /= 1024;
			}
			std::cerr << value << ' ' << si_prefix[p] << what << '\n';
		};
		
		std::chrono::duration<double> d(clock_t::now() - start);
		double bytes_per_second   = bytes / d.count();
		double bytes_per_packet   = (double)bytes / (double)packets;
		double packets_per_second = packets / d.count();
		print(bytes_per_second,  "Bytes/s");
		print(bytes_per_packet,  "Bytes/Packet");
		print(packets_per_second,"Packets/s");
	}
};

static constexpr bool do_socket_bench = true;

template<typename Serializer, typename SBuffer, typename Socket, typename Message>
bool send(Socket& socket, SBuffer& buffer, Message const& message) {
	return time_this("send", [&]() {
		//std::cout << "SEND\n";
		buffer.reset();
		uint64_t size = 0;
		if(!Serializer::serialize(buffer, size, message)) {
			return false;
		}
		size = buffer.count();
		buffer.count() = 0;
		if(!Serializer::serialize(buffer, size)) {
			return false;
		}
		buffer.count() = size;
		socket.send(buffer.data(), buffer.count());
		if constexpr(do_socket_bench) {
			static Bench bench("send");
			bench.add(size);
		}
		return true;
	});
}

template<typename Serializer, typename Message, typename DBuffer, typename Socket>
std::optional<Message> receive(Socket& socket, DBuffer& buffer) {
	return time_this("receive", [&]() -> std::optional<Message> {
		//std::cout << "RECEIVE\n";
		buffer.reset(sizeof(uint64_t));
		if(socket.peek(buffer.data(), sizeof(uint64_t)) != sizeof(uint64_t)) {
			return {};
		}
		uint64_t size;
		if(!Serializer::deserialize(buffer, size)) {
			return {};
		}
		buffer.reset(size);
		if(socket.recv_exact(buffer.data(), size) != size) {
			return {};
		}
		if constexpr(do_socket_bench) {
			static Bench bench("recv");
			bench.add(size);
		}
		Message message;
		if(Serializer::deserialize(buffer, size, message)) {
			return message;
		}
		return {};
	});
}

template<typename Servable, typename Serializer, typename SBuffer, typename DBuffer>
struct Servlet {
	using command_set_t = typename Servable::CommandSet;
	Servable&   servable;
	TCP_Socket  socket;
	std::mutex  mutex;
	bool        _is_done;
	bool        _is_running;
	bool const  verbose;
	std::thread thread;
	
	Servlet(Servlet const&) = delete;
	Servlet& operator=(Servlet const&) = delete;
	Servlet(Servlet&&) = default;
	Servlet& operator=(Servlet&&) = default;
	
	Servlet(Servable& servable, bool verbose = false)
		: servable(servable)
		, _is_done(false)
		, _is_running(true)
		, verbose(verbose)
	{}
	~Servlet() {
		set_stop();
		if(thread.joinable()) {
			thread.join();
		}
	}
	
	void set_stop() {
		std::lock_guard<std::mutex> lock(mutex);
		_is_running = false;
	}
	
	bool is_running() {
		std::lock_guard<std::mutex> lock(mutex);
		return _is_running;
	}
	
	bool is_done() {
		std::lock_guard<std::mutex> lock(mutex);
		return _is_done;
	}
	
	void start() {
		thread = std::thread(&Servlet::run, this);
	}
	
	void run() {
		name_this_thread("Servlet");
		if(verbose) {
			std::cerr << "Servlet started...\n";
		}
		SBuffer sbuffer;
		DBuffer dbuffer;
		while(is_running()) {
			try {
				{
					long timeout_secs = 0;
					int timeout_usecs = 100000;
					if(!socket.can_read(timeout_secs, timeout_usecs)) {
						continue;
					}
				}
				using request_t = typename command_set_t::Request;
				std::optional<request_t> request = receive<Serializer, request_t>(
					socket, dbuffer
				);
				if(!request) {
					if(verbose) {
						std::cerr << "Servlet: No requests available\n";
					}
					break;
				}
				using response_t = typename command_set_t::Response;
				response_t response = std::visit(
					[&](auto const& r) 
						-> response_t
					{
						return response_t{ servable.handle(r) };
					}
					, request->request
				);
				if(!send<Serializer>(socket, sbuffer, response)) {
					continue;
				}
			} catch(PosixError const& e) {
				std::cerr << e.what() << '\n';
				break;
			}
		}
		if(verbose) {
			std::cerr << "Servlet done...\n";
		}
		std::lock_guard<std::mutex> lock(mutex);
		_is_done = true;
	}
};

template<typename Servable, typename Serializer, typename SBuffer, typename DBuffer>
struct Server {
	using command_set_t = typename Servable::CommandSet;
	using servlet_t     = Servlet<Servable, Serializer, SBuffer, DBuffer>;
	Servable&                               servable;
	std::mutex                              mutex;
	bool                                    _is_running;
	bool const                              verbose;
	std::vector<std::unique_ptr<servlet_t>> servlets;
	std::thread                             thread;
	
	Server(Servable& servable, int port, bool verbose = false)
		: servable(servable)
		, _is_running(true)
		, verbose(verbose)
		, thread(&Server::run, this, port)
	{}
	~Server() {
		set_stop();
		if(thread.joinable()) {
			thread.join();
		}
	}
	void set_stop() {
		std::lock_guard<std::mutex> lock(mutex);
		for(auto& s : servlets) {
			s->set_stop();
		}
		_is_running = false;
	}
	bool is_running() {
		std::lock_guard<std::mutex> lock(mutex);
		return _is_running;
	}
	void run(int port) {
		name_this_thread("Server");
		if(verbose) {
			std::cerr << "Server started...\n";
		}
		TCP_ServerSocket socket(port, true);
		while(is_running()) {
			{
				std::lock_guard<std::mutex> lock(mutex);
				servlets.erase(
					std::remove_if(
						  servlets.begin()
						, servlets.end()
						, [](auto& s) {
							return s->is_done();
						}
					)
					, servlets.end()
				);
			}
			try {
				long timeout_secs = 0;
				int timeout_usecs = 100000;
				if(!socket.can_read(timeout_secs, timeout_usecs)) {
					continue;
				}
				{
					std::lock_guard<std::mutex> lock(mutex);
					servlets.push_back(
						std::make_unique<servlet_t>(servable, verbose)
					);
				}
				socket.accept(servlets.back()->socket);
				if(verbose) {
					std::cerr << "Client connected...\n";
				}
				servlets.back()->start();
			} catch (PosixError const& e) {
				std::cerr << e.what() << '\n';
			}
		};
		if(verbose) {
			std::cerr << "Server done...\n";
		}
	}
};


template<typename CommandSet, typename Serializer, typename SBuffer, typename DBuffer>
struct Client {
	TCP_ClientSocket socket;
	SBuffer sbuffer;
	DBuffer dbuffer;
	
	Client(Client const&) = delete;
	Client& operator=(Client const&) = delete;
	Client(Client&&) = default;
	Client& operator=(Client&&) = default;
	
	Client(const std::string& host, int port)
		: socket(host, port)
	{}
	
	template<typename T>
	std::optional<typename T::Response> call(typename T::Request const& request) {
		auto r = _call({request});
		if(r && std::holds_alternative<typename T::Response>(r->response)) {
			return std::get<typename T::Response>(r->response);
		}
		return {};
	}
	
	template<typename T>
	typename T::Response fatal_call(typename T::Request const& request) {
		auto r = _call({request});
		if(r && std::holds_alternative<typename T::Response>(r->response)) {
			return std::get<typename T::Response>(r->response);
		}
		std::cerr << "no valid response for " << request << '\n';
		std::exit(1);
	}
	
	std::optional<typename CommandSet::Response> _call(typename CommandSet::Request const& request) {
		try {
			if(!send<Serializer>(socket, sbuffer, request)) {
				return {};
			}
			{
				long timeout_secs = 10;
				int timeout_usecs = 0;
				if(!socket.can_read(timeout_secs, timeout_usecs)) {
					return {};
				}
			}
			return receive<Serializer, typename CommandSet::Response>(
				socket, dbuffer
			);
		} catch(PosixError const& e) {
			std::cerr << e.what() << '\n';
			return {};
		}
	}
};
