#pragma once
#include <string>
#include <memory>
#include <vector>
#include <iostream>
#include <cstdlib>
#include "simple_gl/TTF_Lock.hpp"
#include "simple_gl/Texture.hpp"
#include "simple_gl/GL_Window.hpp"
#include "simple_gl/Mesh.hpp"
struct Font {
	struct Glyph {
		Texture const texture;
		Mesh<MeshVertex<float,true,true,false>> mesh;
		double width;
		double height;
		
		Glyph(GL_Window const* parent, SDL_Surface* surface, double text_height)
			: texture(parent, surface)
			, mesh(parent)
			, width(surface->w * text_height / surface->h)
			, height(text_height)
		{
			std::vector<uint16_t> indices{0,1,2,0,2,3};
			std::vector<MeshVertex<float,true,true,false>> vertices(4);
			auto& A = vertices[0];
			auto& B = vertices[1];
			auto& C = vertices[2];
			auto& D = vertices[3];
			A.position = {                   0.0f  , static_cast<float>(-height), 0.0f};
			B.position = {static_cast<float>(width), static_cast<float>(-height), 0.0f};
			C.position = {static_cast<float>(width),                     0.0f   , 0.0f};
			D.position = {                   0.0f  ,                     0.0f   , 0.0f};
			A.normal = {0.0f, 0.0f, 1.0f};
			B.normal = A.normal;
			C.normal = A.normal;
			D.normal = A.normal;
			A.tex = {0.0f, 1.0f};
			B.tex = {1.0f, 1.0f};
			C.tex = {1.0f, 0.0f};
			D.tex = {0.0f, 0.0f};
			mesh.load(vertices, indices, GL_STATIC_DRAW);
		}
	};
	double text_height;
	std::vector<std::unique_ptr<Glyph>> glyphs;
	
	template<typename Config>
	std::shared_ptr<TTF_Font> loadFont(Config const& config, int size) {
		if(!config.isInMemory) {
			return {
				  TTF_OpenFont(config.nameOrData, size)
				, TTF_CloseFont
			};
		} else {
			return {
				  TTF_OpenFontRW(
					  SDL_RWFromConstMem(config.nameOrData, config.size)
					, 1
					, size
				  )
				, TTF_CloseFont
			};
		}
	}

	template<typename Config>
	Font(GL_Window const*const parent, Config const& config, int size, double text_height)
		: text_height(text_height)
	{
		auto ttf_lock =  TTF_Lock::get();
		std::shared_ptr<TTF_Font> font = loadFont(config, size);
		if(!font) {
			std::cerr << "Error: could not load font\n";
			std::exit(1);
		}
		glyphs.resize(256);
		std::shared_ptr<SDL_PixelFormat> format{
			  SDL_AllocFormat(SDL_PIXELFORMAT_RGBA32)
			, SDL_FreeFormat
		};
		for(int i = 1; i < 256; ++i) {
			SDL_Color color{255, 255, 255, 255};
			std::shared_ptr<SDL_Surface> surface(
				  TTF_RenderGlyph_Blended(font.get(), i, color)
				, SDL_FreeSurface
			);
			if(!surface) {
				if(std::isprint(i)) {
					std::cerr << "Warning: could not render glyph: (char: " << i << ")\n";
				}
			} else {
				std::shared_ptr<SDL_Surface> surface_2{
					  SDL_ConvertSurface(surface.get(), format.get(), 0)
					, SDL_FreeSurface
				};
				if(surface_2 == nullptr) {
					std::cerr << "Unable to convert image ! SDL Error: " << SDL_GetError() << '\n';
				}
				glyphs[i] = std::make_unique<Glyph>(parent, surface_2.get(), text_height);
			}
		}
	}

	double text_width(std::string const& text) const {
		double w = 0.0;
		for(auto& c : text) {
			auto& g = glyphs[c];
			if(g) {
				w += g->width;
			}
		}
		return w;
	}
	
	double render(
		  std::string const& text
		, double x
		, double y
		, double z
	) const {
		glEnable(GL_TEXTURE_2D);
		for(auto& c : text) {
			auto& g = glyphs[c];
			if(g) {
				scoped_draw([&]() {
					glTranslated(x,y,z);
					g->texture.bind();
					g->mesh.lock().draw();
				});
				x += g->width;
			}
		}
		glDisable(GL_TEXTURE_2D);
		return x;
	}
};
