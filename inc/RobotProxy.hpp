#pragma once
#include <cstdlib>
#include "robo_commands.hpp"
#include "config/Robot.hpp"
#include "client_server/client_server.hpp"
#include "serializer/DefaultPodBackend.hpp"
#include "serializer/EndianSwappingPodBackend.hpp"
#include "serializer/NativePodBackend.hpp"
#include "client_server/client_server.hpp"
#include "serializer/SerializationBuffers.hpp"
#include "serializer/PrefixSerializer.hpp"

namespace robo {

enum class SegmentState {
	  TRAVERSABLE
	, BLOCKED_BY_ROBOT
	, BLOCKED_BY_OBSTACLE
};

struct DropResult {
	enum class Result {
		  SUCCESS
		, NO_GUEST_TO_DROP
		, TOO_FAST_TO_DROP
	};
	Result result;
	int    score;
};

struct RobotProxy {
	using Serializer = PrefixSerializer<DefaultPodBackend>;
	using SerializationBuffer = DynamicSerializationBuffer<>;
	using DeserializationBuffer = DynamicDeserializationBuffer<>;
	using client_t = Client<
		  make_command_set_from_variant<robo::CommandSet>
		, Serializer
		, SerializationBuffer
		, DeserializationBuffer
	>;

	client_t client;
	RegisterRobotCommand::Response registration_response;
	
	RobotProxy(RobotProxy const&) = delete;
	RobotProxy& operator=(RobotProxy const&) = delete;

	RobotProxy(std::string const& host, int port, std::string const& name)
		: client{host, port}
		, registration_response{
			[&]() {
				RegisterRobotCommand::Request request{name};
				std::cerr << request << '\n';
				auto response = client.fatal_call<RegisterRobotCommand>(request);
				std::cerr << response << '\n';
				return response;
			}()
		}
	{}

	~RobotProxy() {
		client.fatal_call<DeregisterRobotCommand>(DeregisterRobotCommand::Request{id()});
	}

	void set_local_velocity(Vertex<double,2> const& velocity, double angular_velocity) {
		LocalVelocityCommand::Request request{id(), velocity, angular_velocity};
		auto response = client.fatal_call<LocalVelocityCommand>(request);
		if(response.result != LocalVelocityCommand::Response::Result::SUCCESS) {
			std::cerr << request << '\n';
			std::cerr << response << '\n';
			std::exit(1);
		}
	}
	void set_local_velocity_fixed_frame(Vertex<double,2> const& velocity, double angular_velocity) {
		LocalVelocityFixedFrameCommand::Request request{id(), velocity, angular_velocity};
		auto response = client.fatal_call<LocalVelocityFixedFrameCommand>(request);
		if(response.result != LocalVelocityFixedFrameCommand::Response::Result::SUCCESS) {
			std::cerr << request << '\n';
			std::cerr << response << '\n';
			std::exit(1);
		}
	}
	void set_debug_lines(std::vector<DebugLine> const& debug_lines) {
		auto response = client.fatal_call<SetDebugLinesCommand>(
			SetDebugLinesCommand::Request{id(), debug_lines}
		);
		if(response.result != SetDebugLinesCommand::Response::Result::SUCCESS) {
			std::exit(1);
		}
	}
	auto drop_guest()
		-> DropResult
	{
		auto response = client.fatal_call<DropTaxiGuestCommand>(
			DropTaxiGuestCommand::Request{id()}
		);
		if(    response.result == DropTaxiGuestCommand::Response::Result::NO_GUEST_TO_DROP
			|| response.result == DropTaxiGuestCommand::Response::Result::SUCCESS
			|| response.result == DropTaxiGuestCommand::Response::Result::TOO_FAST_TO_DROP
		) {
			DropResult::Result r =
				response.result == DropTaxiGuestCommand::Response::Result::SUCCESS
					? DropResult::Result::SUCCESS
					: response.result == DropTaxiGuestCommand::Response::Result::NO_GUEST_TO_DROP
						? DropResult::Result::NO_GUEST_TO_DROP
						: DropResult::Result::TOO_FAST_TO_DROP
			;
			return {r, response.drop_score};
		}
		std::exit(1);
	}
	auto pick_guest()
		-> std::optional<Vision::Guest>
	{
		auto response = client.fatal_call<PickTaxiGuestCommand>(
			PickTaxiGuestCommand::Request{id()}
		);
		if(    response.result == PickTaxiGuestCommand::Response::Result::NO_GUEST_IN_RANGE
			|| response.result == PickTaxiGuestCommand::Response::Result::SUCCESS
			|| response.result == PickTaxiGuestCommand::Response::Result::TOO_FAST_TO_PICK
		) {
			return response.picked_guest;
		}
		std::exit(1);
	}
	auto is_traversable(Vertex<double,2> const& line_start, Vertex<double,2> const& line_end)
		-> SegmentState
	{
		auto response = client.fatal_call<QuerySegmentTraversableCommand>(
			QuerySegmentTraversableCommand::Request{id(), line_start, line_end}
		);
		using Result = QuerySegmentTraversableCommand::Response::Result;
		switch(response.result) {
			case Result::TRAVERSABLE        : return SegmentState::TRAVERSABLE;
			case Result::BLOCKED_BY_OBSTACLE: return SegmentState::BLOCKED_BY_OBSTACLE;
			case Result::BLOCKED_BY_ROBOT   : return SegmentState::BLOCKED_BY_ROBOT;
			default: std::exit(1);
		}
	}
	auto vision()
		-> Vision
	{
		auto response = client.fatal_call<QueryVisionCommand>(
			QueryVisionCommand::Request{id()}
		);
		if(response.result != QueryVisionCommand::Response::Result::SUCCESS) {
			std::exit(1);
		}
		if(response.vision) {
			if(!response.vision->robots.empty()) {
				return *(response.vision);
			} else {
				std::cerr
					<< "Robot terminated by simulator.\n"
					<< "Good bye!\n"
				;
				std::exit(0);
			}
		} else {
			std::cerr << "LOGIC ERROR Warning: -> Bad response: " << response << '\n';
			std::exit(1);
		}
	}
	auto id() const
		-> RobotId
	{
		return registration_response.result->registration;
	}
	auto descriptor() const
		-> RobotDescriptor const&
	{
		return registration_response.result->descriptor;
	}
};

} /** namespace robo */
