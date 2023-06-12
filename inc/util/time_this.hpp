#pragma once
#include <chrono>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

struct TimeStats {
	struct S {
		double      sum = 0;
		std::size_t n   = 0;
		
		void update(double t) {
			sum += t;
			++n;
		}
		
		double average() const {
			return sum / n;
		}
		
		template<typename OS>
		friend
		OS& operator<<(OS& os, S const& s) {
			return os
				<< "avg: " << s.average() << "s/call"
				<< ", total: " << s.sum << 's'
				<< ", calls: " << s.n
			;
		}
	};
	template<typename Str, typename clock_t>
	struct Lock {
		typename clock_t::time_point begin;
		Str const&                   name;
		TimeStats&                   ts;
		Lock(Str const& name, TimeStats& ts)
			: begin{clock_t::now()}
			, name{name}
			, ts{ts}
		{}
		~Lock() {
			ts.add(
				  name
				, std::chrono::duration<double>{
					clock_t::now() - begin
				}.count()
			);
		}
	};
	
	std::mutex mutex;
	std::map<std::string, S> times;
	bool _enabled = false;
	
	bool enabled() {
		std::lock_guard<std::mutex> lock(mutex);
		return _enabled;
	}
	void set_enabled(bool enabled) {
		std::lock_guard<std::mutex> lock(mutex);
		_enabled = enabled;
	}
	
	void add(std::string const& name, double time) {
		std::lock_guard<std::mutex> lock(mutex);
		times[name].update(time);
	}
	
	~TimeStats() {
		if(!enabled()) {
			return;
		}
		std::vector<std::pair<std::string, S>> entries;
		entries.reserve(times.size());
		std::size_t max_name_length = 0;
		for(auto& p : times) {
			entries.push_back(p);
			max_name_length = std::max(max_name_length, p.first.size());
		}
		auto print_name = [&](auto const& p) 
			-> std::ostream&
		{
			std::cout << p.first;
			for(std::size_t n = max_name_length - p.first.size(); n;) {
				--n;
				std::cout << ' ';
			}
			return std::cout << ": ";
		};
		auto sorted_print = [&](auto const& what, auto pred) {
			std::sort(entries.begin(), entries.end(), pred);
			std::cout << "#### Times, sorted by " << what << '\n';
			for(auto& p : entries) {
				print_name(p) << p.second << '\n';
			}
		};
		sorted_print("total time",
			[](auto const& p1, auto const& p2) {
				return p1.second.sum > p2.second.sum;
			}
		);
		sorted_print("average time per call"
			, [](auto const& p1, auto const& p2) {
				return p1.second.average() > p2.second.average();
			}
		);
	}

	static TimeStats& get() {
		static TimeStats ts;
		return ts;
	}
};

template<typename Str, typename OP, typename clock_t = std::chrono::steady_clock>
auto time_this(Str const& name, OP op) 
	-> decltype(op())
{
	TimeStats& ts = TimeStats::get();
	if(!ts.enabled()) {
		return op();
	} else{ 
		TimeStats::Lock<Str,clock_t> lock(name, ts);
		return op();
	}
}
