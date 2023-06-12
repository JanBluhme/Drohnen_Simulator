#pragma once
#include <memory>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <SDL2/SDL.h>

struct SDL_Lock {
private:
	SDL_Lock() {
		if(SDL_Init( SDL_INIT_VIDEO ) < 0) {
			printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
			std::exit(1);
		}
	}
	~SDL_Lock() {
		SDL_Quit();
	}
public:
	static std::shared_ptr<SDL_Lock> get() {
		static std::weak_ptr<SDL_Lock> cache;
		static std::mutex mutex;
		std::lock_guard<std::mutex> lock(mutex);
		auto sp = cache.lock();
		if(!sp) {
			struct X : SDL_Lock {
				using SDL_Lock::SDL_Lock;
			};
			sp = std::make_shared<X>();
			cache = sp;
		}
		return sp;
	}
};
