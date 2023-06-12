#pragma once
#include "simple_gl/SDL_Lock.hpp"
#include <memory>
#include <mutex>
#include <cstdlib>
#include <SDL2/SDL_image.h>

struct ImageLock {
private:
	std::shared_ptr<SDL_Lock> sdl_lock;
	ImageLock()
		: sdl_lock(SDL_Lock::get())
	{
		int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_TIF;
		if(!(IMG_Init(imgFlags) & imgFlags)) {
			printf("IMG_Init: %s\n", IMG_GetError());
			std::exit(1);
		}
	}
	~ImageLock() {
		IMG_Quit();
	}
public:
	static std::shared_ptr<ImageLock> get() {
		static std::weak_ptr<ImageLock> cache;
		static std::mutex mutex;
		std::lock_guard<std::mutex> lock(mutex);
		auto sp = cache.lock();
		if(!sp) {
			struct X : ImageLock {
				using ImageLock::ImageLock;
			};
			sp = std::make_shared<X>();
			cache = sp;
		}
		return sp;
	}
};
