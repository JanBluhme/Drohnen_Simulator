#include "RobotProxy.hpp"
#include "config/Simulator.hpp"

using namespace robo;

void draw_cross(
	  std::vector<DebugLine>& debug_sink
	, Vertex<double,2> const& pos
	, Vertex<double,3> const& color
	, double size
	, double stroke_width
	, double height
) {
	size /= 2.0;
	Vertex<double,2> A{pos[0], pos[1] - size};
	Vertex<double,2> B{pos[0], pos[1] + size};
	Vertex<double,2> C{pos[0] - size, pos[1]};
	Vertex<double,2> D{pos[0] + size, pos[1]};
	debug_sink.push_back(DebugLine::make(A, B, color, stroke_width, height));
	debug_sink.push_back(DebugLine::make(C, D, color, stroke_width, height));
}

void draw_square(
	  std::vector<DebugLine>& debug_sink
	, Vertex<double,2> const& pos
	, Vertex<double,3> const& color
	, double size
	, double stroke_width
	, double height
) {
	size /= 2.0;
	Vertex<double, 2> A{Vertex<double,2>{pos[0] - size, pos[1] - size}};
	Vertex<double, 2> B{Vertex<double,2>{pos[0] + size, pos[1] - size}};
	Vertex<double, 2> C{Vertex<double,2>{pos[0] + size, pos[1] + size}};
	Vertex<double, 2> D{Vertex<double,2>{pos[0] - size, pos[1] + size}};
	debug_sink.push_back(DebugLine::make(A, B, color, stroke_width, height));
	debug_sink.push_back(DebugLine::make(B, C, color, stroke_width, height));
	debug_sink.push_back(DebugLine::make(C, D, color, stroke_width, height));
	debug_sink.push_back(DebugLine::make(D, A, color, stroke_width, height));
}

auto find_robot(RobotProxy& robo, Vision const& vision)
	-> RobotView const*
{
	for(auto const& r : vision.robots) {
		if(r.id == robo.id()) {
			return &r;
		}
	}
	return nullptr;
}

struct ReferenceVelocity {
	Vertex<double, 2> velocity;
	double            angular_velocity;
	bool              use_fixed_frame;
};

// let
//    t   : time
//    a   : maximum acceleration
//    v0  : velocity at t = 0
//    s(t): distance covered at time t
//    v(t): velocity at time t
//
// s(t) := a/2 * t*t + v0 * t
// v(t) :=              a * t + v0
//
// find T, such that:
//     s(T) = D // cover distance D in time T
//     v(T) = 0 // arrive with velocity 0
//     -> find maximum velocity v0 from wich we can safely brake into velocity 0
//        v(T) = a * T + v0
//           0 = a * T + v0
//          v0 = -a * T
//
// T_1 =-sqrt(2 * D / -a) -> negative time not usefull
// T_2 = sqrt(2 * D / -a) -> change sign of a because we deccelerate
//
// -> T  = sqrt(2 * D / a)
// -> v0 = a * sqrt(2 * D / a)
// -> v0 =     sqrt(2 * D * a * a / a)
// -> v0 =     sqrt(2 * D * a)
auto velocity_by_distance_impl(double acceleration_max, double distance)
	-> double
{
	double x = 2.0 * distance * acceleration_max;
	// fix signs for negative distances (when we use it with angles...)
	return x < 0
		? -std::sqrt(-x)
		:  std::sqrt( x)
	;
}

// fix numerical issues for small square roots...
auto velocity_by_distance(double thresh, double acceleration_max, double distance)
	-> double
{
	if(distance < thresh) {
		return distance * velocity_by_distance_impl(acceleration_max, thresh) / thresh;
	}
	return velocity_by_distance_impl(acceleration_max, distance);
}

bool always_fixed = false;
bool never_fixed = false;

auto position_control(
	  Vertex<double, 2> const& reference_position
	, double                   reference_orientation
	, RobotView const&         robot
	, RobotDescriptor const&   limits
	, double                   total_scale
)
	-> ReferenceVelocity
{
	auto min_scale = [](double& scale, double v, double max) {
		if(std::abs(v) > max) {
			scale = std::min(scale, max / std::abs(v));
		}
	};

	double phi_error = sm::normalize_angle_relative(
		  sm::normalize_angle_relative(reference_orientation)
		- sm::normalize_angle_relative(robot.orientation)
	);
	double omega = velocity_by_distance( 25.0 * sm::to_radians, limits.kinematics.acceleration_max_angular, phi_error);
	double T = phi_error / omega;
	Vertex<double,2> pos_error = reference_position - robot.position;
	Vertex<double,2> pos_error_loc = rotate(pos_error, -robot.orientation);
	ReferenceVelocity result;
	if(never_fixed || (!always_fixed && std::abs(phi_error) < 10.0 * M_PI/180.0)) {
		auto calc_T = [](double abs_e, double max_a) {
			return std::sqrt(2.0 * abs_e / max_a);
		};
		auto calc_T_lin = [&](double e_lin, double e, double max_a) {
			e = std::abs(e);
			if(e > e_lin) {
				return calc_T(e, max_a);
			} else {
				double v_lin = velocity_by_distance(e_lin, max_a, e);
				double v_lin_mean = v_lin / 2.0;
				if(v_lin_mean < 1e-3) {
					return v_lin_mean;
				}
				return e / v_lin_mean;
			}
		};


// 		double T_x = std::sqrt(2.0 * std::abs(pos_error_loc[0]) / limits.kinematics.acceleration_max_x);
// 		double T_y = std::sqrt(2.0 * std::abs(pos_error_loc[1]) / limits.kinematics.acceleration_max_y);
		double T_x = calc_T_lin(0.25, pos_error_loc[0], limits.kinematics.acceleration_max_x);
		double T_y = calc_T_lin(0.25, pos_error_loc[1], limits.kinematics.acceleration_max_y);
		double T = std::max(T_x, T_y);
		Vertex<double,2> v = T > 0.0
			? 2.0 * pos_error_loc / T
			: pos_error_loc * 0.0
		;
		double scale = 1.0;
		min_scale(scale, v[0] , limits.kinematics.velocity_max_x      );
		min_scale(scale, v[1] , limits.kinematics.velocity_max_y      );
		min_scale(scale, omega, limits.kinematics.angular_velocity_max);
		scale *= total_scale;
		return {
			  scale * v
			, scale * omega
			, false
		};
	} else {
		double v_max = std::min(limits.kinematics.velocity_max_x    , limits.kinematics.velocity_max_y    );
		double a_max = std::min(limits.kinematics.acceleration_max_x, limits.kinematics.acceleration_max_y);
		double vel = velocity_by_distance(0.25, a_max, pos_error_loc.length());
		Vertex<double,2> v = pos_error_loc * (vel / pos_error_loc.length());
		double scale = 1.0;
		min_scale(scale, v[0] , v_max                      );
		min_scale(scale, v[1] , v_max                      );
		min_scale(scale, omega, limits.kinematics.angular_velocity_max);
		scale *= total_scale;
		return {
			  scale * v
			, scale * omega
			, true
		};
	}
}

int main(int argc, char** argv) {
	std::string name = "foobar";
	for(int i = 0; i < argc; ++i) {
		std::cerr << "param: " << i << ": " << argv[i] << '\n';
	}
	if(argc > 1) {
		std::cerr << argv[1] << '\n';
		if(std::string_view{"--always-fixed"} == std::string_view{argv[1]}) {
			always_fixed = true;
			name = "always_fixed";
		}
		if(std::string_view{"--never-fixed"} == std::string_view{argv[1]}) {
			never_fixed = true;
			name = "never_fixed";
		}
	}
	RobotProxy robo{"localhost", config::Simulator::default_port, name};
	std::vector<DebugLine> debug_sink;
	Vertex<double,2> wps[]{
		  Vertex<double,2>{ 1.0,  1.0}
		, Vertex<double,2>{-1.0,  1.0}
		, Vertex<double,2>{-1.0, -1.0}
		, Vertex<double,2>{ 1.0, -1.0}
	};
	if(never_fixed) {
		for(auto& wp : wps) {
			wp[1] += 1.5;
		}
	}
	if(always_fixed) {
		for(auto& wp : wps) {
			wp[1] -= 1.5;
		}
	}
	std::size_t i_wp = 0;

	while(true) {
		struct DBG_Lock{
			RobotProxy&             robo;
			std::vector<DebugLine>& debug_sink;
			DBG_Lock(RobotProxy& robo, std::vector<DebugLine>& debug_sink)
				: robo{robo}
				, debug_sink{debug_sink}
			{
				debug_sink.clear();
			}
			~DBG_Lock() {
				robo.set_debug_lines(debug_sink);
			}
		} lock{robo, debug_sink};
		Vision            v     = robo.vision();
		RobotView const*  state = find_robot(robo, v);
// 		if(!state) return 0;
		if((wps[i_wp] - state->position).length() < 0.01) {
			i_wp = (i_wp + 1) % std::size(wps);
		}
		Vertex<double,2> ref_pos    = wps[i_wp];
		Vertex<double, 2> pos_error = ref_pos - state->position;
		double            ref_ori   = orientation(pos_error);
		ReferenceVelocity vel       = position_control(ref_pos, ref_ori, *state, robo.descriptor(), 1.0);
		if(vel.use_fixed_frame) {
			robo.set_local_velocity_fixed_frame(
				vel.velocity, vel.angular_velocity
			);
		} else {
			robo.set_local_velocity(
				vel.velocity, vel.angular_velocity
			);
		}
		for(std::size_t i = 0; i < std::size(wps); ++i) {
			debug_sink.push_back(
				DebugLine::make(wps[i], wps[(i + 1) % std::size(wps)], {0.4, 0.4, 0.4}, 0.01, 0.01)
			);
		}
		draw_square(
			  debug_sink
			, ref_pos
			, Vertex<double,3>{1.0, 1.0, 1.0}
			, 0.25
			, 0.02
			, 0.03
		);

	}
}
