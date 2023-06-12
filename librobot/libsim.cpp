#include "libsim.hpp"
#include "RobotProxy.hpp"
#include "config/Simulator.hpp"

struct Environment::Hidden
	: robo::RobotProxy
{
	Robot robot;

	Hidden(std::string const& name, Environment& environment)
		: robo::RobotProxy{
			  "localhost"
			, robo::config::Simulator::default_port
			, name
		}
		, robot{environment}
	{}
};

void delete_hidden(Environment::Hidden* h) {
	delete h;
}

static auto guest_state(robo::Vision::Guest const& x)
	-> GuestState
{
	return {
		  x.position
		, x.target_position
		, x.score_on_arrival
	};
}

static auto robot_state(robo::RobotView const& x)
	-> RobotState
{
	return {
		  x.id.value
		, x.name
		, x.position
		, x.velocity
		, x.orientation
		, x.angular_velocity
		, x.score
	};
}

static auto drop_result_result(robo::DropResult const& x)
	-> DropResult
{
	DropResult::Result r = x.result == robo::DropResult::Result::NO_GUEST_TO_DROP
			? DropResult::Result::NO_GUEST_TO_DROP
			: x.result == robo::DropResult::Result::TOO_FAST_TO_DROP
				? DropResult::Result::TOO_FAST_TO_DROP
				: DropResult::Result::SUCCESS
	;
	return {
		r, x.score
	};
}

static auto segment_state(robo::SegmentState x)
	-> SegmentState
{
	return x == robo::SegmentState::TRAVERSABLE
		? SegmentState::TRAVERSABLE
		: x == robo::SegmentState::BLOCKED_BY_ROBOT
			? SegmentState::BLOCKED_BY_ROBOT
			: SegmentState::TRAVERSABLE
	;
}

static auto robot_descriptor(robo::RobotDescriptor const& x)
	-> RobotDescriptor
{
	return {
		RobotDescriptor::KinematicModel {
			  x.kinematics.velocity_max_x
			, x.kinematics.velocity_max_y
			, x.kinematics.angular_velocity_max
			, x.kinematics.acceleration_max_x
			, x.kinematics.acceleration_max_y
			, x.kinematics.acceleration_max_angular
		}
		, RobotDescriptor::BodyModel {
			  x.body.mass
			, x.body.radius
			, x.body.distance_sensor_angles
		}
		, RobotDescriptor::GuestModel {
			  x.guest.max_pick_distance
			, x.guest.max_drop_distance
			, x.guest.max_pick_velocity
			, x.guest.max_drop_velocity
		}, RobotDescriptor::VisibilityModel {
			  x.visibility.max_robot_distance
			, x.visibility.max_guest_distance
		}
	};
}

static auto debug_line(DebugLine const& x)
	-> robo::DebugLine
{
	return {
		  x.start
		, x.end
		, x.color_start
		, x.color_end
		, x.thickness
	};
}

auto DebugLine::make(
	  Vertex<double,2> const& start
	, Vertex<double,2> const& end
	, Vertex<double,3> const& color
	, double const            width
	, double const            height
)
	-> DebugLine
{
	 return {
		  Vertex<double,3>{start[0], start[1], height}
		, Vertex<double,3>{end[0]  , end[1]  , height}
		, color
		, color
		, width
	};
}

Environment::Environment(std::string const& robot_name)
	: implementation{
		  new Hidden{robot_name, *this}
		, delete_hidden
	}
{}

auto Environment::robot() const
	-> Robot&
{
	return implementation->robot;
}

auto Environment::is_traversable(
	  Vertex<double,2> const& segment_start
	, Vertex<double,2> const& segment_end
) const
	-> SegmentState
{
	return segment_state(implementation->is_traversable(segment_start, segment_end));
}

auto Environment::vision() const
	-> Vision
{
	auto v = implementation->vision();
	auto robots = [&]() {
		std::vector<RobotState> robots;
		robots.reserve(v.robots.size());
		for(auto const& x : v.robots) {
			robots.push_back(robot_state(x));
		}
		return robots;
	};
	auto guests = [&]() {
		std::vector<GuestState> visible_guests;
		visible_guests.reserve(v.available_guests.size());
		for(auto const& x : v.available_guests) {
			visible_guests.push_back(guest_state(x));
		}
		return visible_guests;
	};
	return {
		  robots()
		, guests()
		, v.distance_sensor_values
		, v.guest ? guest_state(*v.guest) : std::optional<GuestState>{}
	};
}

Robot::Robot(Environment& environment)
	: environment{environment}
{}

void Robot::set_local_velocity(
	  Vertex<double,2> const& velocity
	, double const            angular_velocity
) {
	environment.implementation->set_local_velocity(velocity, angular_velocity);
}

void Robot::set_local_velocity_fixed_frame(
	  Vertex<double,2> const& velocity
	, double const            angular_velocity
) {
	environment.implementation->set_local_velocity_fixed_frame(velocity, angular_velocity);
}

void Environment::set_debug_lines(std::vector<DebugLine> const& debug_lines) {
	std::vector<robo::DebugLine> x;
	x.reserve(debug_lines.size());
	for(auto const& y : debug_lines) {
		x.push_back(debug_line(y));
	}
	implementation->set_debug_lines(x);
}

auto Robot::drop_guest()
	-> DropResult
{
	return drop_result_result(environment.implementation->drop_guest());
}

auto Robot::pick_guest()
	-> std::optional<GuestState>
{
	if(auto r = environment.implementation->pick_guest(); r) {
		return guest_state(*r);
	}
	return {};
}

auto Robot::id() const
	-> std::size_t
{
	return environment.implementation->id().value;
}

auto Robot::descriptor() const
	-> RobotDescriptor
{
	return robot_descriptor(environment.implementation->descriptor());
}
