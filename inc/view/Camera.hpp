#pragma once
#include "simple_gl/GL_Window.hpp"
#include "robo_commands.hpp"
#include "math/r3/Transform.hpp"
#include "math/angle_util.hpp"
#include "view/EnvironmentViewBase.hpp"
#include "config/Pitch.hpp"

#include <vector>
#include <iostream>
#include <chrono>
#include <cstdio>

template<typename OS, typename T>
OS& funny_print(OS& os, unsigned int a, unsigned int b, T const& x) {
	if constexpr(
		   std::is_same_v<T, float>
		|| std::is_same_v<T, double>
		|| std::is_same_v<T, long double>
	) {
		std::array<char, 46> fmt;
		snprintf(fmt.data(), fmt.size(), "%%%u.%uf", a + b + 1, b);
// 		std::cerr << "#### : " << fmt.data() << " ####\n";
		std::array<char, 1024> buffer;
		if(snprintf(buffer.data(), buffer.size(), fmt.data(), x) < buffer.size()) {
			os << buffer.data();
		} else {
			os << x;
		}
	} else {
		os << x;
	}
	return os;
}
template<typename OS, typename T, std::size_t N>
OS& funny_print(OS& os, unsigned int a, unsigned int b, Vertex<T, N> const& v) {
	os << '[';
	if constexpr(N) {
		funny_print(os, a, b, v[0]);
		[&]<std::size_t... Is>(std::index_sequence<0, Is...>) {
			((os << ", ", funny_print(os, a, b, v[Is])), ...);
		}(std::make_index_sequence<N>{});
	}
	os << ']';
	return os;
}
template<typename OS>
OS& funny_print(OS& os, unsigned int a, unsigned int b, Transform const& T) {
	os << "[\n";
	for(std::size_t i = 0; i < 3; ++i) {
		os << "    ";
		funny_print(os, a, b, T.D.get_D()[i]);
		os << " | ";
		funny_print(os, a, b, T.T[i]);
		os << '\n';
	}
	os << "]\n";
	return os;
}

namespace robo {

struct Camera_base {
	GL_Window& window;
	int        width;
	int        height;
	double     field_of_view_degrees = 40.0;
	double     w_height              = 1.1 * config::Pitch::height;
	bool       is_ortho              = false;
	double     perspective_z_near    = 0.01;
	double     perspective_z_far     = 100.0;
	double     ortho_z_near          = 0.01;
	double     ortho_z_far           = 100.0;
	GLint      viewport[4];
	GLdouble   modelview[16];
	GLdouble   projection[16];

	Camera_base(GL_Window& window, int width, int height)
		: window{window}
	{
		reshape(width, height);
	}
	void update_transform() {
		glGetIntegerv(GL_VIEWPORT,          viewport  );
		glGetDoublev( GL_MODELVIEW_MATRIX,  modelview );
		glGetDoublev( GL_PROJECTION_MATRIX, projection);
	}
	void reshape(int width, int height) {
		this->width = width;
		this->height = height;
		window.do_operation([&]() {
			glViewport(0,0,width,height);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			if(!is_ortho) {
				gluPerspective(
					  field_of_view_degrees
					, aspect()
					, perspective_z_near
					, perspective_z_far
				);
			} else {
				double w_width = w_height * aspect();
				double left   = -w_width / 2.0;
				double right  =  w_width / 2.0;
				double bottom = -w_height / 2.0;
				double top    =  w_height / 2.0;
				glOrtho(left, right, bottom, top, ortho_z_near, ortho_z_far);
			}
		});
	}
	auto aspect() noexcept
		-> double
	{
		return static_cast<double>(width) / static_cast<double>(height);
	}

	void fix_projection() {
		reshape(width, height);
	}

	Vertex<double,3> world_position(int screen_x, int screen_y) {
		GLfloat  winX = (float)screen_x;
		GLfloat  winY = (float)viewport[3] - (float)screen_y;
		Vertex<double,3> a;
		Vertex<double,3> b;
		// find line in world through (winX,winY)
		// lambda = 0 -> a
		// lambda = 1 -> b
		gluUnProject(winX, winY, 0, modelview, projection, viewport, &a[0], &a[1], &a[2]);
		gluUnProject(winX, winY, 1, modelview, projection, viewport, &b[0], &b[1], &b[2]);
		Vertex<double,3> u = b - a;
		// -> p(lambda) = a + lambda * u
		// lambda0, such that p[2] == 0
		double lambda0 = -a[2]/u[2];
		return a + lambda0 * u;
	}
};

struct SluggishTransform {
	double    tau = 2.0;
	Transform eye{};
	Transform ref{};

	void update(Transform const& T) {
		ref = T;
		eye = interpolate(eye, ref, std::exp(-tau));
	}
	void instantly() {
		eye = ref;
	}
};

// look backward to position
struct CameraArm {
	Vertex<double, 3> position;
	double            length;
	double            azimuth;
	double            elevation;

	// like gluLookAt, but reverses adaption for screen coordinates...
	static auto make_look_at_transform(
		  Vertex<double,3> const& eye
		, Vertex<double,3> const& center
		, Vertex<double,3> const& up
	)
		-> Transform
	{
		return (
			  Transform::rotate_x_pi_halfs< 1>()
			* Transform::rotate_y_pi_halfs<-1>()
			* Transform::look_at(eye, center, up)
		).invert();
	}
	static auto make_camera_pose(
		  Vertex<double, 3> const& center
		, double distance
		, double azimuth
		, double elevation
	)
		-> Transform
	{
		Vertex<double,3> local_eye{
			  distance * std::cos(elevation) * std::cos(azimuth)
			, distance * std::cos(elevation) * std::sin(azimuth)
			, distance * std::sin(elevation)
		};
		Vertex<double,3> up {
			  -std::sin(elevation) * std::cos(azimuth)
			, -std::sin(elevation) * std::sin(azimuth)
			,  std::cos(elevation)
		};
		return make_look_at_transform(center + local_eye, center, up);
	}

	// as seen from camera...
	auto local() const
		-> Rotation
	{
		return Rotation::rotation_z(azimuth);
	}
	auto pose() const
		-> Transform
	{
		return make_camera_pose(position, length, azimuth, elevation);
	}
};

struct Camera
	: Camera_base
{
	enum class State {
		  BIRDS_EYE
		, BIRDS_EYE_PAN
		, BIRDS_EYE_PAN_ROBOT
		, FOLLOW_ROBOT_FIXED_ORIENTATION
		, FOLLOW_ROBOT
		, FOLLOW_ROBOT_ROTATING
		, USER
		, END
	};

	friend
	State& operator++(State& s) {
		using S = State;
		switch(s) {
			default:
			case S::BIRDS_EYE                      : return s = S::BIRDS_EYE_PAN;
			case S::BIRDS_EYE_PAN                  : return s = S::BIRDS_EYE_PAN_ROBOT;
			case S::BIRDS_EYE_PAN_ROBOT            : return s = S::USER;
			case S::USER                           : return s = S::FOLLOW_ROBOT_FIXED_ORIENTATION;
			case S::FOLLOW_ROBOT_FIXED_ORIENTATION : return s = S::FOLLOW_ROBOT;
			case S::FOLLOW_ROBOT                   : return s = S::FOLLOW_ROBOT_ROTATING;
			case S::FOLLOW_ROBOT_ROTATING          : return s = S::BIRDS_EYE;
		}
	}

	static std::string to_string(State state) {
		switch(state) {
			case State::BIRDS_EYE: {
				return "BIRDS_EYE";
			}
			case State::BIRDS_EYE_PAN: {
				return "BIRDS_EYE_PAN";
			}
			case State::BIRDS_EYE_PAN_ROBOT: {
				return "BIRDS_EYE_PAN_ROBOT";
			}
			case State::FOLLOW_ROBOT_FIXED_ORIENTATION: {
				return "FOLLOW_ROBOT_FIXED_ORIENTATION";
			}
			case State::FOLLOW_ROBOT: {
				return "FOLLOW_ROBOT";
			}
			case State::FOLLOW_ROBOT_ROTATING: {
				return "FOLLOW_ROBOT_ROTATING";
			}
			case State::USER: {
				return "USER";
			}
			case State::END: {
				return "";
			}
		}
		return "";
	}
	struct CameraOnArm {
		CameraArm arm;
		Transform camera_pose = arm.pose();

		void on_update() {
			camera_pose = arm.pose();
		}
	};

	State             state = State::BIRDS_EYE;
	SluggishTransform camera_pose{3.0};
	SluggishTransform local_robot_sluggish_camera_pose{4.0};
	RobotId           tracked_robot{1};

	CameraOnArm local_robot_camera{
		CameraArm{
			  Vertex<double, 3>{0.0, 0.0, 0.0}
			, 3.0
			, M_PI
			, 0.5
		}
	};
	CameraOnArm user_camera{
		CameraArm{
			  Vertex<double, 3>{0.0, 0.0, 0.0}
			, 20.0
			, M_PI + 0.5
			, 1.2
		}
	};

	Vertex<double, 3> last_tracked_robot_position{};
	double            last_tracked_robot_orientation{};
	double            t  = 0.0;
	double            dt = 0.01;
	Vertex<double, 3> birds_eye_pan_eye{0.0, 0.0, 10.0};
	Vertex<double, 3> birds_eye_pan_robot_eye{0.0, 0.0, 10.0};
	Vertex<double, 3> current_cam_base{};

	auto camera_base_posion() const
		-> Vertex<double, 3>
	{
		return current_cam_base;
	}

	Camera(GL_Window& window, int width, int height)
		: Camera_base{window, width, height}
	{}

	CameraOnArm* current_camera() {
		if(    state == State::FOLLOW_ROBOT
			|| state == State::FOLLOW_ROBOT_FIXED_ORIENTATION
		) {
			return &local_robot_camera;
		}
		if(state == State::USER) {
			return &user_camera;
		}
		return nullptr;
	}

	void rotate_arm(double d_azimuth, double d_height) {
		if(CameraOnArm* cam = current_camera()
			; cam
		) {
			cam->arm.azimuth   += d_azimuth;
			cam->arm.elevation += d_height;
			on_update_camera_arm();
		}
	}
	void scale_arm_length(double scale) {
		if(CameraOnArm* cam = current_camera()
			; cam
		) {
			cam->arm.length *= scale;
			on_update_camera_arm();
		} else if(state == State::BIRDS_EYE_PAN) {
			birds_eye_pan_eye[2] *= scale;
			camera_pose.instantly();
		} else if(state == State::BIRDS_EYE_PAN_ROBOT) {
			birds_eye_pan_robot_eye[2] *= scale;
			camera_pose.instantly();
		}
	}

	void on_update_camera_arm() {
		if(CameraOnArm* cam = current_camera()
			; cam
		) {
			cam->on_update();
			camera_pose.instantly();
			local_robot_sluggish_camera_pose.instantly();
		}
	}
	void move_camera(Vertex<double, 3> const& delta) {
		if(CameraOnArm* cam = current_camera()
			; cam
		) {
			cam->arm.position += cam->arm.local() * delta;
			on_update_camera_arm();
		} else if(state == State::BIRDS_EYE_PAN) {
			birds_eye_pan_eye[0] += delta[1];
			birds_eye_pan_eye[1] -= delta[0];
			camera_pose.instantly();
		} else if(state == State::BIRDS_EYE_PAN_ROBOT) {
			birds_eye_pan_robot_eye[0] += delta[1];
			birds_eye_pan_robot_eye[1] -= delta[0];
			camera_pose.instantly();
		}
	}

	void move_forward(double x) {
		move_camera(Vertex<double,3>{x, 0.0, 0.0});
	}
	void move_left(double y) {
		move_camera(Vertex<double,3>{0.0, y, 0.0});
	}
	void move_up(double z) {
		move_camera(Vertex<double,3>{0.0, 0.0, z});
	}
	void move_backward(double x) {
		move_forward(-x);
	}
	void move_right(double y) {
		move_left(-y);
	}
	void move_down(double z) {
		move_up(-z);
	}

	void next() {
		++state;
	}
	void look_at(Transform const& T) {
		Vertex<double,3> eye    = T.position();
		Vertex<double,3> center = T.position( Vertex<double,3>{1.0, 0.0, 0.0});
		Vertex<double,3> up     = T.direction(Vertex<double,3>{0.0, 0.0, 1.0});

		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixd(Transform::look_at(eye,center,up).as_gl_matrix().data());
	}

	void next_robot(std::vector<Robot> const& robots) {
		if(robots.empty()) {
			tracked_robot.value = 1; // reset
		} else {
			auto it = std::find_if(
				  robots.begin()
				, robots.end()
				, [&](Robot const& r) {
					return r.id == tracked_robot;
				}
			);
			if(it == robots.end()) {
				// current id not found and robots not empty
				// -> reset
				tracked_robot = robots.front().id;
			} else {
				// advance
				++it;
				if(it == robots.end()) {
					it = robots.begin();
				}
				tracked_robot = it->id;
			}
		}
	}

	void setup(std::vector<Robot> const& robots) {
		auto correct_ortho = [&](Vertex<double,3> const& birds_eye_eye) {
			if(is_ortho) {
				double field_of_view_y = field_of_view_degrees * M_PI / 180.0;
				w_height = birds_eye_eye[2] * (2.0 * tan(field_of_view_y / 2.0));
				fix_projection();
			}
		};

		auto moving_look_at = [&](Transform const& T) {
			Vertex<double,3> c0 = T.position( Vertex<double,3>{0.0, 0.0, 0.0});
			Vertex<double,3> c1 = T.position( Vertex<double,3>{1.0, 0.0, 0.0});
			double m = c1[2] - c0[2];
			double n = c0[2];
			double lambda = -n / m;
			current_cam_base = (c1 - c0) * lambda + c0;
			camera_pose.update(T);
			look_at(camera_pose.eye);
		};
		auto birds_eye_camera = [&]() {
			double field_of_view_y = field_of_view_degrees * M_PI / 180.0;
			double scale = std::max(
				  config::Pitch::width  / width
				, config::Pitch::height / height
			);
			double height_y = scale * height;
			double height_z = 1.1 * height_y / (2.0 * tan(field_of_view_y / 2.0));
			Vertex<double,3> eye{0.0, 0.0, height_z};
			Transform T
				= Transform::translate(eye)
				* Transform::rotate_z_pi_halfs<1>()
				* Transform::rotate_y_pi_halfs<1>()
			;
			correct_ortho(eye);
			current_cam_base = eye;
			moving_look_at(T);
		};
		auto birds_eye_pan_camera = [&]() {
			Transform T
				= Transform::translate(birds_eye_pan_eye)
				* Transform::rotate_z_pi_halfs<1>()
				* Transform::rotate_y_pi_halfs<1>()
			;
			correct_ortho(birds_eye_pan_eye);
			current_cam_base = birds_eye_pan_eye;
			moving_look_at(T);
		};
		auto user_camera = [&]() {
			moving_look_at(this->user_camera.camera_pose);
		};

		auto follow_robot = [&](auto local_op) {
			auto it = std::find_if(
				  robots.begin()
				, robots.end()
				, [&](Robot const& r) {
					return r.id == tracked_robot;
				}
			);
			if(it == robots.end()) {
				if(robots.empty()) {
// 					state = State::BIRDS_EYE;
					return birds_eye_camera();
				}
				it = robots.begin();
			}
			moving_look_at(local_op(*it) * local_robot_camera.camera_pose);
		};
		auto follow_robot_birds_eye = [&]() {
			auto it = std::find_if(
				  robots.begin()
				, robots.end()
				, [&](Robot const& r) {
					return r.id == tracked_robot;
				}
			);
			if(it == robots.end()) {
				if(robots.empty()) {
// 					state = State::BIRDS_EYE;
					return birds_eye_camera();
				}
				it = robots.begin();
			}
			auto const& pos = it->kinematics.position;
			Transform T
				= Transform::translate(
					  birds_eye_pan_robot_eye[0] + pos[0]
					, birds_eye_pan_robot_eye[1] + pos[1]
					, birds_eye_pan_robot_eye[2]
				)
				* Transform::rotate_z_pi_halfs<1>()
				* Transform::rotate_y_pi_halfs<1>()
			;
			correct_ortho(birds_eye_pan_robot_eye);
			moving_look_at(T);
		};
		auto robot_fixed_orientation = [&](Robot const& robot) {
			Robot::Kinematics const& kin = robot.kinematics;
			return Transform::translate(kin.position[0], kin.position[1], 0.0);
		};
		auto robot_sluggish_orientation = [&](Robot const& robot) {
			Robot::Kinematics const& kin = robot.kinematics;
			local_robot_sluggish_camera_pose.update(Transform::rotate_z(kin.orientation));
			return
				  Transform::translate(kin.position[0], kin.position[1], 0.0)
				* local_robot_sluggish_camera_pose.eye
			;
		};
		auto rotate_arround_robot = [&](Robot const& robot) {
			Robot::Kinematics const& kin = robot.kinematics;
			t = std::fmod(t + dt, 2.0 * M_PI);
			return
				  Transform::translate(kin.position[0], kin.position[1], 0.0)
				* Transform::rotate_z(t)
			;
		};

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		switch(state) {
			default:
			case State::USER                          : { return user_camera();                           }
			case State::BIRDS_EYE                     : { return birds_eye_camera();                      }
			case State::BIRDS_EYE_PAN                 : { return birds_eye_pan_camera();                  }
			case State::BIRDS_EYE_PAN_ROBOT           : { return follow_robot_birds_eye();                }
			case State::FOLLOW_ROBOT_FIXED_ORIENTATION: { return follow_robot(robot_fixed_orientation );  }
			case State::FOLLOW_ROBOT_ROTATING         : { return follow_robot(rotate_arround_robot );     }
			case State::FOLLOW_ROBOT                  : { return follow_robot(robot_sluggish_orientation);}
		}
	}
};

} /** namespace robo */
