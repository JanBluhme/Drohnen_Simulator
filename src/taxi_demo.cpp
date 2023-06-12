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
	Vertex<double,2> pos_error = reference_position - robot.position;
	Vertex<double,2> pos_error_loc = rotate(pos_error, -robot.orientation);

	double T_x = std::sqrt(2.0 * std::abs(pos_error_loc[0]) / limits.kinematics.acceleration_max_x);
	double T_y = std::sqrt(2.0 * std::abs(pos_error_loc[1]) / limits.kinematics.acceleration_max_y);
	double T = std::max(T_x, T_y);
	Vertex<double,2> v = T > 0.0
		? pos_error_loc / T
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
	};
}

auto segment_color(SegmentState state)
	-> Vertex<double, 3>
{
	switch(state) {
		default:
		case SegmentState::BLOCKED_BY_OBSTACLE: return {1.0, 0.0, 0.0};
		case SegmentState::BLOCKED_BY_ROBOT   : return {1.0, 0.0, 1.0};
		case SegmentState::TRAVERSABLE        : return {0.0, 1.0, 0.0};
	}
}

#include <queue>

auto a_star(
	  RobotProxy&                          robot
	, Vertex<double,2> const&              start
	, Vertex<double,2> const&              target
	, std::vector<DebugLine>&              debug_sink
	, std::size_t                          node_expansions_max
	, std::vector<Vertex<double,2>> const& neighbourhood
)
	-> std::vector<Vertex<double,2>>
{
	std::vector<Vertex<double,2>> position;
	std::vector<double>           g;
	std::vector<double>           h;
	std::vector<double>           f;
	std::vector<std::size_t>      predecessor_id;
	std::vector<bool>             closed;

	auto make_node = [&](Vertex<double,2> const& node_position, double node_g, std::size_t node_predecessor_id) {
		std::size_t node_id = position.size();
		position.push_back(node_position);
		g.push_back(node_g);
		h.push_back((target - node_position).length());
		f.push_back(g[node_id] + h[node_id]);
		predecessor_id.push_back(node_predecessor_id);
		closed.push_back(false);
		return node_id;
	};
	auto cmp = [&](std::size_t id_a, std::size_t id_b) {
		return f[id_a] > f[id_b];
	};

	std::vector<std::size_t> open_list;
	auto enqueue = [&](std::size_t node_id, double node_f) {
		f[node_id] = node_f;
		open_list.push_back(node_id);
		std::push_heap(open_list.begin(), open_list.end(), cmp);
	};
	auto remove_min = [&]() {
		std::pop_heap(open_list.begin(), open_list.end(), cmp);
		std::size_t result = open_list.back();
		open_list.pop_back();
		return result;
	};
	std::size_t start_node_id = make_node(
		  start
		, 0.0
		, std::numeric_limits<std::size_t>::max()
	);
	std::size_t target_node_id = make_node(
		  target
		, std::numeric_limits<double>::max()
		, std::numeric_limits<std::size_t>::max()
	);
	enqueue(start_node_id, f[start_node_id]);
	auto expand = [&](std::size_t current_node_id) {
		std::vector<size_t> successors;
		Vertex<double,2> cur_pos = position[current_node_id];
		auto append_successor = [&](Vertex<double,2> const& new_pos) {
			SegmentState state = robot.is_traversable(cur_pos, new_pos);
			if(state == SegmentState::TRAVERSABLE) {
				std::size_t new_node_id;
				auto it = std::find(position.begin(), position.end(), new_pos);
				if(it == position.end()) {
					new_node_id = make_node(new_pos, g[current_node_id] + (new_pos - cur_pos).length(), current_node_id);
				} else {
					new_node_id = std::distance(position.begin(), it);
				}
				successors.push_back(new_node_id);
			}
// 			debug_sink.push_back(
// 				DebugLine::make(
// 					cur_pos, new_pos, segment_color(state), 0.01, 0.05
// 				)
// 			);
		};
		for(auto const& n : neighbourhood) {
			append_successor(cur_pos + n);
		}
		append_successor(position[target_node_id]);

		for(std::size_t successor_id : successors) {
			if(closed[successor_id]) {
				continue;
			}
			double c = (position[successor_id] - position[current_node_id]).length();
			double tentative_g = g[current_node_id] + c;
			auto it = std::find(open_list.begin(), open_list.end(), successor_id);
			if(    it != open_list.end()
				&& tentative_g >= g[successor_id]
			) {
				continue;
			}
			predecessor_id[successor_id] = current_node_id;
			g[successor_id] = tentative_g;
			double successor_f = tentative_g + h[successor_id];
			if(it != open_list.end()) {
				f[successor_id] = successor_f;
				std::make_heap(open_list.begin(), open_list.end(), cmp);
			} else {
				enqueue(successor_id, successor_f);
			}
		}
	};

	while(!open_list.empty() && node_expansions_max) {
		std::size_t current_node_id = remove_min();
		if(current_node_id == target_node_id) {
			std::vector<Vertex<double,2>> result;
			while(current_node_id != std::numeric_limits<std::size_t>::max()) {
				result.push_back(position[current_node_id]);
				current_node_id = predecessor_id[current_node_id];
			}
			for(std::size_t i = 1; i < result.size(); ++i) {
				debug_sink.push_back(
					DebugLine::make(
						  result[i - 1]
						, result[i]
						, {0.0, 0.0, 1.0}
						, 0.03
						, 0.06
					)
				);
			}
			assert(result.front() == target);
			assert(result.back()  == start );
			return result;
		}
		closed[current_node_id] = true;
		expand(current_node_id);
		--node_expansions_max;
	}
	return {};
}

auto plan_backward(
	  RobotProxy&                          proxy
	, RobotView const&                     robot
	, Vertex<double,2> const&              target
	, std::vector<Vertex<double,2>> const& neighbourhood
	, std::vector<DebugLine>&              debug_sink
)
	-> std::optional<Vertex<double,2>>
{
	std::vector<Vertex<double,2>> path = a_star(
		  proxy
		, target
		, robot.position
		, debug_sink
		, 10000
		, neighbourhood
	);
	if(path.size() > 1) {
		return path[1];
	}
	return {};
}

int main() {
	RobotProxy robo{"localhost", config::Simulator::default_port, "foobar"};
	std::array scout_points{
		  Vertex<double,2>{-4.5, -0.5}
		, Vertex<double,2>{-4.0, -4.25}
		, Vertex<double,2>{ 3.5, -2.5}
		, Vertex<double,2>{-1.0, -1.0}
		, Vertex<double,2>{ 4.5,  3.0}
	};
	std::size_t i = 0;
	std::vector<DebugLine> debug_sink;
	std::vector<Vertex<double,2>> neighbourhood{
		  Vertex<double,2>{-0.25,  0.0 }
		, Vertex<double,2>{ 0.25,  0.0 }
		, Vertex<double,2>{ 0.0 , -0.25}
		, Vertex<double,2>{ 0.0 ,  0.25}
	};
	std::optional<Vision::Guest> guest;
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
		double           scale = 1.0;
		Vision           vision      = robo.vision();
		RobotView const* robot_state = find_robot(robo, vision);
		Vertex<double,2> target = robot_state->position;
		if(vision.guest) {
			guest = vision.guest;
			Vertex<double,2> d = guest->target_position - robot_state->position;
			scale = std::clamp(d.length(), 0.4, 1.0);
			if(d*d < 0.05 * 0.05) {
				auto r = robo.drop_guest();
				if(r.result == DropResult::Result::SUCCESS) {
					continue;
				}
				if(r.result == DropResult::Result::NO_GUEST_TO_DROP) {
					guest = {};
					continue;
				}
				target = guest->target_position;
			} else {
				target = guest->target_position;
			}
		} else if(!vision.available_guests.empty()) {
			guest = {};
			double dist2 = std::numeric_limits<double>::max();
			for(auto const& g : vision.available_guests) {
				Vertex<double,2> d = g.position - robot_state->position;
				if(d*d < dist2) {
					guest = g;
					dist2 = d*d;
				}
			}
			Vertex<double,2> d = guest->position - robot_state->position;
			scale = std::clamp(d.length(), 0.4, 1.0);
			if(d*d < 0.05 * 0.05) {
				auto g = robo.pick_guest();
				if(g) {
					target = guest->target_position;
				} else {
					target = guest->position;
				}
			} else {
				target = guest->position;
			}
		} else {
			if((scout_points[i] - robot_state->position).length() < 5e-2) {
				i = (i + 1) % scout_points.size();
			}
			target = scout_points[i];
		}

		draw_cross(debug_sink, target, {0.0, 1.0, 1.0}, 0.5, 0.03, 0.07);
		auto o_ref_pos = plan_backward(robo, *robot_state, target, neighbourhood, debug_sink);
		if(!o_ref_pos) {
			robo.set_local_velocity(Vertex<double,2>{0.0, 0.0}, 0.0);
			continue;
		}
		Vertex<double,2> ref_pos = *o_ref_pos;

		draw_cross(debug_sink, ref_pos, {1.0, 0.0, 1.0}, 0.5, 0.03, 0.07);
		debug_sink.push_back(
			DebugLine::make(
				  robot_state->position
				, ref_pos
				, Vertex<double,3>{1.0, 0.0, 1.0}
				, 0.03
				, 0.07
			)
		);
		Vertex<double, 2> pos_error = ref_pos - robot_state->position;
		double            ref_ori = orientation(pos_error);
		ReferenceVelocity vel = position_control(ref_pos, ref_ori, *robot_state, robo.descriptor(), scale);
		robo.set_local_velocity(
			vel.velocity, vel.angular_velocity
		);
	}
}
