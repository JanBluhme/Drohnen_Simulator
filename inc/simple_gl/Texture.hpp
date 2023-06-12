#pragma once
#include <cassert>
#include <string>
#include <iostream>
#include <GL/glew.h>
#include <GL/glut.h>
// #include <GL/freeglut_ext.h> // glutSolidCylinder
#include "simple_gl/GL_Window.hpp"
#include "simple_gl/ImageLock.hpp"

struct Texture {
	GL_Window const* parent;
	unsigned int     texture;
	int              width;
	int              height;

	Texture(Texture const&) = delete;
	Texture& operator=(Texture const&) = delete;
	
	Texture(Texture&& other)
		: parent( other.parent )
		, texture(other.texture)
		, width(  other.width  )
		, height( other.height )
	{
		other.texture = 0;
	}
	Texture& operator=(Texture&& other) {
		if(&other != this) {
			assert(other.parent == parent && "Texture moved to wrong GL_Context");
			this->~Texture();
			texture = other.texture;
			width   = other.width;
			height  = other.height;
			other.texture = 0;
		}
		return *this;
	}
	
	Texture(GL_Window const* parent)
		: parent(parent)
		, width(0)
		, height(0)
	{
		parent->do_operation(
			[&]() {
				glGenTextures(1, &texture);
				bind();
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
		);
	}
	
	Texture(GL_Window const* parent, int width, int height, unsigned char* data)
		: Texture(parent)
	{
		load(width, height, data);
	}
	
	Texture(GL_Window const* parent, SDL_Surface const* surface)
		: Texture(
			parent, surface->w, surface->h, (unsigned char*)surface->pixels
		)
	{}
	Texture(GL_Window const* parent, SDL_Surface* surface)
		: Texture(
			parent, surface->w, surface->h, (unsigned char*)surface->pixels
		)
	{}

	Texture(GL_Window const* parent, std::string const& filename)
		: Texture(parent)
	{
		auto lock = ImageLock::get();
		SDL_Surface* surface_1 = IMG_Load(filename.c_str());
		if(surface_1 == nullptr) {
			std::cerr << "Unable to load image "<< filename << "! SDL_image Error: " << IMG_GetError() << '\n';
		}
		SDL_PixelFormat* format    = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
		SDL_Surface*     surface_2 = SDL_ConvertSurface(surface_1, format, 0);
		if(surface_2 == nullptr) {
			std::cerr << "Unable to convert image " << filename << "! SDL Error: " << SDL_GetError() << '\n';
		}
		SDL_FreeFormat(format);
		SDL_FreeSurface(surface_1);
		load(surface_2->w, surface_2->h, (unsigned char*)surface_2->pixels);
		SDL_FreeSurface(surface_2);
	}
	template<typename Config>
	Texture(GL_Window const* parent, Config const& config)
		: Texture(parent)
	{
		auto lock = ImageLock::get();
		SDL_Surface* surface_1;
		if(!config.isInMemory) {
			surface_1 = IMG_Load(config.nameOrData);
		} else {
			surface_1 = IMG_Load_RW(SDL_RWFromConstMem(config.nameOrData,config.size) ,1);
		}

		if(surface_1 == nullptr) {
			std::cerr << "Unable to load image ! SDL_image Error: " << IMG_GetError() << '\n';
		}
		SDL_PixelFormat* format    = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32);
		SDL_Surface*     surface_2 = SDL_ConvertSurface(surface_1, format, 0);
		if(surface_2 == nullptr) {
			std::cerr << "Unable to convert image ! SDL Error: " << SDL_GetError() << '\n';
		}
		SDL_FreeFormat(format);
		SDL_FreeSurface(surface_1);
		load(surface_2->w, surface_2->h, (unsigned char*)surface_2->pixels);
		SDL_FreeSurface(surface_2);
	}
	
	~Texture() {
		if(texture) {
			parent->do_operation(
				[&]() {
					glDeleteTextures(1, &texture);
				}
			);
		}
	}
	
	void load(int width, int height, unsigned char* data) {
		if(texture) {
			parent->do_operation(
				[&]() {
					bind();
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
				}
			);
			this->width  = width;
			this->height = height;
		}
	}
	
	void bind() const {
		if(texture) {
			glBindTexture(GL_TEXTURE_2D, texture);
		}
	}
};
