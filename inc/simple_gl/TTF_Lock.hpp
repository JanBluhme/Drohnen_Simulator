#pragma once
#include "simple_gl/SDL_Lock.hpp"
#include <memory>
#include <mutex>
#include <cstdlib>
#include <SDL2/SDL_ttf.h>

struct TTF_Lock {
private:
	std::shared_ptr<SDL_Lock> sdl_lock;
	TTF_Lock()
		: sdl_lock(SDL_Lock::get())
	{
		if(TTF_Init() < 0) {
			printf("TTF_Init: %s\n", TTF_GetError());
			std::exit(1);
		}
	}
	~TTF_Lock() {
		TTF_Quit();
	}
public:
	static std::shared_ptr<TTF_Lock> get() {
		static std::weak_ptr<TTF_Lock> cache;
		static std::mutex mutex;
		std::lock_guard<std::mutex> lock(mutex);
		auto sp = cache.lock();
		if(!sp) {
			struct X : TTF_Lock {
				using TTF_Lock::TTF_Lock;
			};
			sp = std::make_shared<X>();
			cache = sp;
		}
		return sp;
	}
};
