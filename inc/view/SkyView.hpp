#pragma once
#include "config/Pitch.hpp"
#include "simple_gl/make_simple_cone.hpp"
#include "simple_gl/make_checker_board.hpp"
#include "simple_gl/Texture.hpp"
#include "math/PointLine.hpp"
#include "config/View.hpp"
#include <vector>
#include "view/EnvironmentViewBase.hpp"
#include "config/Textures.hpp"

namespace robo {

struct SkyView {
	using mesh_data_t_plane_cube = mesh_data_by_tags_t<has_normal_tag>;
	using mesh_data_t_plane      = mesh_data_by_tags_t<has_normal_tag, has_tex_tag>;
	using gl_mesh_t_plane_cube   = mesh_data_t_plane_cube::gl_mesh_t;
	using gl_mesh_t_plane        = mesh_data_t_plane::gl_mesh_t;

	EnvironmentViewBase&   parent;
	std::array<Texture,15> textures{
		  Texture{&parent.window, config::Textures::X_2k_stars_milky_way }
		, Texture{&parent.window, config::Textures::X_2k_stars           }
		, Texture{&parent.window, config::Textures::X_2k_earth_clouds    }
// 		, Texture{&parent.window, config::Textures::X_2k_earth_daymap    }
// 		, Texture{&parent.window, config::Textures::X_2k_earth_nightmap  }
		, Texture{&parent.window, config::Textures::X_2k_jupiter         }
		, Texture{&parent.window, config::Textures::X_2k_mars            }
		, Texture{&parent.window, config::Textures::X_2k_mercury         }
		, Texture{&parent.window, config::Textures::X_2k_moon            }
		, Texture{&parent.window, config::Textures::X_2k_neptune         }
		, Texture{&parent.window, config::Textures::X_2k_saturn          }
		, Texture{&parent.window, config::Textures::X_2k_sun             }
		, Texture{&parent.window, config::Textures::X_2k_uranus          }
		, Texture{&parent.window, config::Textures::X_2k_venus_atmosphere}
		, Texture{&parent.window, config::Textures::X_2k_venus_surface   }
// 		, Texture{&parent.window, config::Textures::X_8081_earthlights2k }
// 		, Texture{&parent.window, config::Textures::X_8081_earthmap2k    }
		, Texture{&parent.window, config::Textures::moonmap2k            }
		, make_checker_board(
			&parent.window
			, 128, 128, 512, 512
			, Vertex<double, 4>{0.0, 0.0, 1.0, 0.7}
			, Vertex<double, 4>{0.7, 0.4, 0.0, 0.4}
		)
	};
	std::size_t             current_texture = 0;
	mesh_data_t_plane const mesh_data_fence;
	gl_mesh_t_plane const   mesh_fence{     mesh_data_fence.mesh(     &parent.window)};
	bool                    enabled = false;

	SkyView(EnvironmentViewBase& parent)
		: parent{parent}
		, mesh_data_fence{[](){
// 			double scale = 1.1;
// 			double x = scale * config::Pitch::width;
// 			double y = scale * config::Pitch::height;
// 			double z = 5.0;
// 			Transform rot_x = Transform::rotate_x_pi_halfs<1>();
// 			Transform rot_y = Transform::rotate_y_pi_halfs<1>();
// 			return
// 				  (Transform::translate(0.0     ,  x / 2.0, z / 2.0 -0.3) * rot_x * make_square_xy<true,false>(y, z, 0.0))
// 				+ (Transform::translate(0.0     , -x / 2.0, z / 2.0 -0.3) * rot_x * make_square_xy<true,false>(y, z, 0.0))
// 				+ (Transform::translate( y / 2.0, 0.0     , z / 2.0 -0.3) * rot_y * make_square_xy<true,false>(z, x, 0.0))
// 				+ (Transform::translate(-y / 2.0, 0.0     , z / 2.0 -0.3) * rot_y * make_square_xy<true,false>(z, x, 0.0))
// 			;
			return make_simple_sherical_segment<true, false>(23.0, -M_PI_2, M_PI_2, 0.0, 2.0*M_PI, 24, 24);
		}()}
	{}

	void next_texture() {
		++current_texture;
		current_texture %= textures.size();
	}

	void show() {
		if(!enabled) {
			return;
		}
		glEnable(GL_TEXTURE_2D);
		textures[current_texture].bind();
		parent.set_color(Vertex<double,4>{1.0,1.0,1.0,1.0});
		auto lock = mesh_fence.lock();
		lock.draw();
		glDisable(GL_TEXTURE_2D);
	}
};

} /** namespace robo */
