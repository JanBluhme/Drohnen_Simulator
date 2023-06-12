#pragma once
#include "simple_gl/SDL_Lock.hpp"
#include <string>
#include <cstdlib>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

struct GL_Window {
	std::shared_ptr<SDL_Lock> lock;
	SDL_Window* const         window;
	SDL_GLContext             gl_context;
	uint32_t                  window_id;
	
	~GL_Window() {
		SDL_GL_DeleteContext(gl_context);
		SDL_DestroyWindow(window);
	}
	GL_Window(std::string const& title, int width, int height)
		: lock(SDL_Lock::get())
		, window(
			SDL_CreateWindow(
				  title.c_str()
				, SDL_WINDOWPOS_UNDEFINED
				, SDL_WINDOWPOS_UNDEFINED
				, width
				, height
				,     SDL_WINDOW_OPENGL
					| SDL_WINDOW_SHOWN
					| SDL_WINDOW_RESIZABLE
			)
		)
	{
		if(!window) {
			printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
			std::exit(1);
		}
		window_id = SDL_GetWindowID(window);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
		
		/** BEGIN from https://github.com/ocornut/imgui/blob/master/examples/example_sdl_opengl2/main.cpp */
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		/** END from https://github.com/ocornut/imgui/blob/master/examples/example_sdl_opengl2/main.cpp */
		
		gl_context = SDL_GL_CreateContext(window);
		if(!gl_context) {
			printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
			std::exit(1);
		}
		SDL_GL_MakeCurrent(window, gl_context);
		static bool glew_is_initialized = false;
		if(!glew_is_initialized) {
			glewExperimental = GL_TRUE;
			GLenum glewError = glewInit();
			if(glewError != GLEW_OK) {
				printf( "Error initializing GLEW! %s\n", glewGetErrorString( glewError ) );
				std::exit(1);
			}
			glew_is_initialized = true;
		}
		//Use Vsync
		if(SDL_GL_SetSwapInterval(1) < 0) {
			printf( "Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError() );
		}
	}
	
	template<typename OP, typename... As>
	auto do_operation(OP op, As&&... as) const {
		SDL_GL_MakeCurrent(window, gl_context);
		return op(std::forward<As>(as)...);
	}
	template<typename OP, typename... As>
	void draw(OP op, As&&... as)  const {
		do_operation(
			[&op,this](As&&... as) {
				glClearDepth(20.0);
				glClearColor(0.13, 0.12, 0.13, 1.0);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				glMatrixMode(GL_MODELVIEW);
				glLoadIdentity();
				op(std::forward<As>(as)...);
				SDL_GL_SwapWindow(window);
			}
			, std::forward<As>(as)...
		);
	}
	void set_title(std::string const& title) {
		SDL_SetWindowTitle(window, title.c_str());
	}
};

template<typename DrawOp>
void scoped_draw(DrawOp op) {
	glPushMatrix();
	op();
	glPopMatrix();
}

template<typename DrawOp>
void scoped_ortho_draw(
	  double left
	, double right
	, double bottom
	, double top
	, double near
	, double far
	, DrawOp op
) {
	glMatrixMode(GL_PROJECTION);
	scoped_draw([&]() {
		glLoadIdentity();
		glOrtho(left, right, bottom, top, near, far);
		glMatrixMode(GL_MODELVIEW);
		scoped_draw([&]() {
			glLoadIdentity();
			op();
		});
		glMatrixMode(GL_PROJECTION);
	});
	glMatrixMode(GL_MODELVIEW);
}
