MAKE_INCLUDES  =
MAKE_INCLUDES += make/makefile.config
MAKE_INCLUDES += make/makefile.bash
MAKE_INCLUDES += make/makefile.patterns
include make/makefile.config
include make/makefile.bash

GENERATED_HEADERS=
GENERATED_HEADERS+=robo_environment_template.hpp
GENERATED_HEADERS+=robo_commands.hpp
ENVIRONMENT_INCLUDES=
ENVIRONMENT_INCLUDES+=irobo_commands.hpp
ENVIRONMENT_INCLUDES+=iclient_server/make_command_set.hpp

SOCKET_SOURCES=
SOCKET_SOURCES+=socket/Address.cpp
SOCKET_SOURCES+=socket/FileDescriptor.cpp
SOCKET_SOURCES+=socket/Socket_impl.cpp

DP_LIB_SOURCES=
DP_LIB_SOURCES+=dp_lib/util/FileDescriptor.cpp
DP_LIB_SOURCES+=dp_lib/util/priority.cpp
DP_LIB_SOURCES+=dp_lib/util/Process.cpp

IMGUI_SOURCES=
IMGUI_SOURCES+=imgui/imgui.cpp
IMGUI_SOURCES+=imgui/imgui_draw.cpp
IMGUI_SOURCES+=imgui/imgui_tables.cpp
IMGUI_SOURCES+=imgui/imgui_widgets.cpp
IMGUI_SOURCES+=imgui/imgui_impl_sdl.cpp
IMGUI_SOURCES+=imgui/imgui_impl_opengl2.cpp

SOURCES = $(SOCKET_SOURCES)
SOURCES+= $(DP_LIB_SOURCES)

RESOURCES=$(shell cd resources ; find data/ -type f | sed -e 's/^data\///')
RESOURCES_SOURCES=$(RESOURCES:%=%.cpp)
RESOURCES_HEADERS=$(RESOURCES:%=%.hpp)

BINARY_SOURCES=
BINARY_SOURCES+=simulator.cpp
BINARY_SOURCES+=demo.cpp
BINARY_SOURCES+=taxi_demo.cpp
BINARY_SOURCES+=velo.cpp

LIBRARY_SOURCES=
LIBRARY_SOURCES+=librobot/libsim.cpp

OBJECTS          = $(SOURCES:%.cpp=%.o)
IMGUI_OBJECTS    = $(IMGUI_SOURCES:%.cpp=%.o)
RESOURCES_OBJECTS= $(RESOURCES_SOURCES:%.cpp=%.o)
DEPS =
DEPS += $(BINARY_SOURCES:%.cpp=%.dep)
DEPS += $(LIBRARY_SOURCES:%.cpp=%.dep)
DEPS += $(SOURCES:%.cpp=%.dep)
DEPS += $(IMGUI_SOURCES:%.cpp=%.dep)

include make/makefile.patterns

.PHONY: default
default: $(GENERATOR)
default: $(addprefix $(GEN)/,$(GENERATED_HEADERS))
default: pre_simulator
default: $(addprefix $(BIN)/, $(BINARY_SOURCES:%.cpp=%))
default: $(BIN)/simulator
default: libs

.PHONY: pre_simulator
pre_simulator: $(addprefix resources/generated_src/, $(RESOURCES_SOURCES))
pre_simulator: $(addprefix resources/generated_src/, $(RESOURCES_HEADERS))

$(BIN)/simulator: $(addprefix $(OBJ)/,$(OBJECTS) $(IMGUI_OBJECTS))
$(BIN)/simulator: $(addprefix $(OBJ)/resources/,$(RESOURCES_OBJECTS))

.PHONY: libs
libs: $(addprefix $(BIN)/, $(LIBRARY_SOURCES:%.cpp=%.so))

.PHONY: git_hash
git_hash:
	@$(UPDATE_GIT_HASH) F

git.hash: git_hash

.PHONY: deb
deb: default
	make_deb/run_checkinstall

resources/makebinary: resources/makebinary.cpp
	g++ -std=c++17 $(^) -o $(@)

-include $(addprefix $(DEP)/,$(DEPS))

.PHONY: all
all:
	time $(MAKE) default CXXC=g++
	time $(MAKE) default CXXC=clang++

.PHONY: sanitize_resource_names
sanitize_resource_names:
	@$(CLEAN_RESOURCE_FILENAMES) F resources/data

.PHONY: clean
clean:
	rm -rf $(OBJ) $(DEP) $(GEN) resources/generated_src
	cd recreated_goon && $(MAKE) clean

.PHONY: distclean
distclean:
	rm -rf $(OBJ) $(DEP) $(GEN) $(BIN)
	cd recreated_goon && $(MAKE) distclean

.PHONY: check-and-reinit-submodules
check-and-reinit-submodules:
	@if git submodule status | egrep -q '^[-]|^[+]' ; then \
		$(COLOR_ECHO) F "$(SHELL_COLOR_DEPENDENCIES)" "INFO: Need to reinitialize git submodules"; \
		git submodule update --init; \
	fi

$(GENERATOR): check-and-reinit-submodules
	cd recreated_goon && $(MAKE)
	#cd recreated_goon && git checkout master
	#cd recreated_goon && $(MAKE)

$(addprefix $(GEN)/,$(GENERATED_HEADERS)): robo.idl $(GENERATOR)

.PHONY: kate
kate:
	kate -n                                        \
		$(shell                                \
			find    -type f                \
				-name "*.cpp"          \
				-or -name "*.hpp"      \
				-or -name "*makefile*" \
			| grep -v generated_src        \
			| grep -v ./imgui/src          \
			| grep -v ./inc/dp_lib/        \
			| grep -v ./src/dp_lib/        \
			| grep -v recreated_goon/      \
		)                                      \
		compile_flags.txt                      \
		.git*                                  \
		robo.idl
