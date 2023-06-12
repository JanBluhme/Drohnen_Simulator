#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "math/Vertex.hpp"

struct RobotDescriptor {
	struct KinematicModel {
		double const velocity_max_x;
		double const velocity_max_y;
		double const angular_velocity_max;
		double const acceleration_max_x;
		double const acceleration_max_y;
		double const acceleration_max_angular;
	};
	struct BodyModel {
		double const              mass;
		double const              radius;
		std::vector<double> const distance_sensor_angles;
	};
	struct GuestModel {
		double const max_pick_distance;
		double const max_drop_distance;
		double const max_pick_velocity;
		double const max_drop_velocity;
	};
	struct VisibilityModel {
		double const max_robot_distance;
		double const max_guest_distance;
	};
	KinematicModel const  kinematics;
	BodyModel const       body;
	GuestModel const      guest;
	VisibilityModel const visibility;
};

struct RobotState {
	std::size_t const      id;
	std::string const      name;
	Vertex<double,2> const position;
	Vertex<double,2> const velocity;
	double const           orientation;
	double const           angular_velocity;
	int const              score;
};

struct GuestState {
	Vertex<double, 2> const position;
	Vertex<double, 2> const target_position;
	int               const score_on_arrival;
};

struct Vision {
	std::vector<RobotState> const   robots;
	std::vector<GuestState> const   available_guests;
	std::vector<double> const       distance_sensor_values;
	std::optional<GuestState> const guest;
};

enum struct SegmentState {
	  TRAVERSABLE
	, BLOCKED_BY_ROBOT
	, BLOCKED_BY_OBSTACLE
};

struct DropResult {
	enum struct Result {
		  SUCCESS
		, NO_GUEST_TO_DROP
		, TOO_FAST_TO_DROP
	};
	Result const result;
	int const    score;
};

struct DebugLine {
	Vertex<double,3> start;
	Vertex<double,3> end;
	Vertex<double,3> color_start;
	Vertex<double,3> color_end;
	double           thickness;

	static auto make(
		  Vertex<double,2> const& start
		, Vertex<double,2> const& end
		, Vertex<double,3> const& color
		, double const            width
		, double const            height
	)
		-> DebugLine
	;
};

class Environment;

class Robot {
	friend class Environment;
	Environment& environment;
	Robot(Environment& environment);
public:
	void set_local_velocity(
		  Vertex<double,2> const& velocity
		, double const            angular_velocity
	);

	void set_local_velocity_fixed_frame(
		  Vertex<double,2> const& velocity
		, double const            angular_velocity
	);

	auto drop_guest()
		-> DropResult
	;

	auto pick_guest()
		-> std::optional<GuestState>
	;

	auto id() const
		-> std::size_t
	;

	auto descriptor() const
		-> RobotDescriptor
	;
};

class Environment {
	class Hidden;
	using hidden_ptr_t = std::unique_ptr<Hidden, void(*)(Hidden*)>;
	hidden_ptr_t implementation;
	friend void delete_hidden(Environment::Hidden* h);
	friend class Robot;
public:
	Environment(std::string const& robot_name);

	auto robot() const
		-> Robot&
	;

	auto is_traversable(
		  Vertex<double,2> const& segment_start
		, Vertex<double,2> const& segment_end
	) const
		-> SegmentState
	;

	auto vision() const
		-> Vision
	;

	void set_debug_lines(std::vector<DebugLine> const& debug_lines);
};
