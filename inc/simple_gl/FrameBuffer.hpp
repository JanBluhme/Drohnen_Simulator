#pragma once
#include "simple_gl/Texture.hpp"
#include "simple_gl/Font.hpp"
#include "simple_gl/GL_Window.hpp"
#include <iostream>
#include <cstdlib>

struct DepthBuffer {
	GL_Window const* parent;
	unsigned int     depth_buffer;

	DepthBuffer(DepthBuffer const&) = delete;
	DepthBuffer& operator=(DepthBuffer const&) = delete;
	
	DepthBuffer(DepthBuffer&& other)
		: parent(other.parent)
		, depth_buffer(other.depth_buffer)
	{
		other.depth_buffer = 0;
	}
	
	DepthBuffer& operator=(DepthBuffer&& other) {
		if(&other != this) {
			assert(other.parent == parent && "DepthBuffer moved to wrong GL_Context");
			this->~DepthBuffer();
			depth_buffer = other.depth_buffer;
			other.depth_buffer= 0;
		}
		return *this;
	}
	
	DepthBuffer(GL_Window const* parent, int width, int height)
		: parent(parent)
	{
		parent->do_operation(
			[&]() {
				glGenRenderbuffers(1, &depth_buffer);
				bind();
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
			}
		);
	}
	
	~DepthBuffer() {
		if(depth_buffer) {
			parent->do_operation(
				[&]() {
					glDeleteRenderbuffers(1, &depth_buffer);
				}
			);
		}
	}
	
	void bind() const {
		if(depth_buffer) {
			glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
		}
	}
};


struct FrameBuffer {
	GL_Window const* parent;
	Texture          texture;
	DepthBuffer      depth_buffer;
	unsigned int     framebuffer;
	int              width;
	int              height;
	
	FrameBuffer(FrameBuffer const&) = delete;
	FrameBuffer& operator=(FrameBuffer const&) = delete;
	
	FrameBuffer(FrameBuffer&& other                 )
		: parent(      other.parent                 )
		, texture(     std::move(other.texture)     )
		, depth_buffer(std::move(other.depth_buffer))
		, framebuffer( other.framebuffer            )
		, width(       other.width                  )
		, height(      other.height                 )
	{
		other.parent = nullptr;
		other.framebuffer = 0;
	}
	
	FrameBuffer& operator=(FrameBuffer&& other) {
		if(&other != this) {
			assert(other.parent == parent && "FrameBuffer moved to wrong GL_Context");
			this->~FrameBuffer();
			texture      = std::move(other.texture);
			depth_buffer = std::move(other.depth_buffer);
			framebuffer  = other.framebuffer;
			width        = other.width;
			height       = other.height;
			other.parent      = nullptr;
			other.framebuffer = 0;
		}
		return *this;
	}
	
	FrameBuffer(GL_Window const* parent, int width, int height)
		: parent(parent)
		, texture(parent, width, height, nullptr)
		, depth_buffer(parent, width, height)
		, width(width)
		, height(height)
	{
		parent->do_operation(
			[&]() {
				glGenFramebuffers(1, &framebuffer);
				bind();
				glFramebufferTexture(
					  GL_FRAMEBUFFER
					, GL_COLOR_ATTACHMENT0
					, texture.texture
					, 0
				);
				glFramebufferRenderbuffer(
					  GL_FRAMEBUFFER
					, GL_DEPTH_ATTACHMENT
					, GL_RENDERBUFFER
					, depth_buffer.depth_buffer
				);
				GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
				glDrawBuffers(1, draw_buffers);
				if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
					std::cerr << "Horrible: can't create framebuffer\n";
					std::exit(1);
				}
				unbind();
			}
		);
	}
	~FrameBuffer() {
		if(framebuffer) {
			parent->do_operation(
				[&]() {
					glDeleteFramebuffers(1,&framebuffer);
				}
			);
		}
	}
	
	void bind() const {
		if(framebuffer) {
			glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
		}
	}
	void unbind() const {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	
	struct Lock {
		FrameBuffer const* parent;
		int old_viewport[4];
		
		Lock(FrameBuffer const* parent)
			: parent(parent)
		{
			assert(parent->framebuffer && "FrameBuffer::Lock() on moved FrameBuffer");
			glGetIntegerv(GL_VIEWPORT, old_viewport);
			parent->bind();
			glViewport(0, 0, parent->width, parent->height);
		}
		~Lock() {
			assert(parent->framebuffer && "FrameBuffer::~Lock() on moved FrameBuffer");
			parent->unbind();
			glViewport(
				  old_viewport[0]
				, old_viewport[1]
				, old_viewport[2]
				, old_viewport[3]
			);
		}
	};
	auto lock() const {
		return Lock{this};
	}
};
