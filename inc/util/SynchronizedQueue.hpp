#pragma once
#include <mutex>
#include <condition_variable>
#include <queue>
#include <optional>

template<typename T>
class SynchronizedQueue {
public:
	using value_t = T;
	using pop_t   = std::optional<value_t>;
private:
	std::queue<value_t>     data;
	std::mutex              mutex;
	std::condition_variable state_changed;
	bool                    _is_running = true;
public:
	bool is_running() {
		std::lock_guard<std::mutex> lock(mutex);
		return _is_running;
	}
	
	void stop() {
		{
			std::lock_guard<std::mutex> lock(mutex);
			_is_running = false;
		}
		state_changed.notify_all();
	}
	
	template<typename... Ts>
	bool emplace(Ts&&... as) {
		std::unique_lock<std::mutex> lock(mutex);
		if(_is_running) {
			data.emplace(std::forward<Ts>(as)...);
			lock.unlock();
			state_changed.notify_all();
			return true;
		}
		return false;
	}
	
	bool push(value_t const& v) {
		std::unique_lock<std::mutex> lock(mutex);
		if(_is_running) {
			data.push(v);
			lock.unlock();
			state_changed.notify_all();
			return true;
		}
		return false;
	}
	pop_t pop() {
		std::unique_lock<std::mutex> lock(mutex);
		while(_is_running && data.empty()) {
			state_changed.wait(lock);
		}
		if(data.empty()) {
			return {};
		} else {
			auto v = data.front();
			data.pop();
			lock.unlock();
			state_changed.notify_all();
			return v;
		}
	}
};
