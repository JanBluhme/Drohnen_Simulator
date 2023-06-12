#include "serializer/DefaultPodBackend.hpp"
#include "client_server/client_server.hpp"
#include "serializer/SerializationBuffers.hpp"
#include "serializer/PrefixSerializer.hpp"
#include "util/CommandLineArguments.hpp"
#include "Environment.hpp"
#include "EnvironmentView.hpp"
#include "simulator_gui.hpp"
#include <csignal>
#include <atomic>
#include <thread>

void simloop(robo::Environment& environment, std::atomic<bool> & is_running) {
	name_this_thread("simloop");
	while(is_running) {
		using clock_t = std::chrono::steady_clock;
		auto loop_enter = clock_t::now();
		time_this("simulation", [&]() {environment.update(false);});
		auto deadline  = loop_enter + std::chrono::duration<double>(1.0/environment.get_fps_simulation());
		std::this_thread::sleep_until(deadline);
	}
}

std::atomic<bool> is_running{true};
void signal_handler(int /*signal*/) {
  is_running = false;
}

using Serializer = PrefixSerializer<DefaultPodBackend>;
using SerializationBuffer = DynamicSerializationBuffer<>;
using DeserializationBuffer = DynamicDeserializationBuffer<>;

int main(int argc, char** argv) {
	std::signal(SIGINT,  signal_handler);
	std::signal(SIGTERM, signal_handler);
	CommandLineArguments cla(argc, argv);
	double fps_gui    = cla.get<double>("--fps=", 60);
	double fps_sim    = cla.get<double>("--fps_sim=",    1.0/robo::config::Simulation::delta_t_sim);
	double fps_vision = cla.get<double>("--fps_vision=", 60.0);
	int speed_scale   = cla.get<double>("--speed_scale=", 0);
// 	bool has_imgui = !cla.has_prefix("--no_imgui");
	bool has_imgui = true;
	if(cla.has_prefix("--sync")) {
		fps_gui = fps_sim;
	}
	robo::Environment environment{1.0/fps_vision, 1.0/fps_sim, speed_scale};

	Server<
		  robo::Environment
		, Serializer
		, SerializationBuffer
		, DeserializationBuffer
	> server(environment, robo::config::Simulator::default_port, false);
	
	if(cla.has_prefix("--stats")) {
		TimeStats::get().set_enabled(true);
	} else {
		TimeStats::get().set_enabled(false);
	}
	
	auto do_sim = [&]() {
		if(!cla.has_prefix("--no_simloop")) {
		    simloop(environment,is_running);
		}
	};
	
	if(cla.has_prefix("--no_gui")) {
		do_sim();
		environment.kill();
	} else {
		std::thread sim_thread(do_sim);
		event_loop(environment,1.0/fps_gui,is_running, has_imgui);
		environment.kill();
		sim_thread.join();
	}
	return 0;
}
