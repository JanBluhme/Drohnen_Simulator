SHELL=/bin/bash
MAKEFILE = makefile
SRC = src
GEN = generated
INC =
INC+= inc
INC+= $(GEN)
OBJ = obj
BIN = bin
DEP = dep
CXXC= g++

TARGET_SOURCE_A=generator.cpp

SOURCES_A = $(TARGET_SOURCE_A)
GEN_SOURCES = Scanner.cpp
GEN_SOURCES+= Parser.cpp

BISON_HEADERS  =
BISON_HEADERS += Parser.hpp
BISON_HEADERS += location.hh
BISON_HEADERS += position.hh
BISON_HEADERS += stack.hh


CXXFLAGS = $(addprefix -I,$(INC))
CXXFLAGS+= -std=c++20
CXXFLAGS+= -O3
CXXFLAGS+= -march=native
CXXFLAGS+= -flto
CXXFLAGS+= -pedantic
CXXFLAGS+= -Wfatal-errors

LIBS =

LFLAGS = $(addprefix -l,$(LIBS))
LFLAGS+=

TARGET_A = $(TARGET_SOURCE_A:%.cpp=%)
OBJECTS_A= $(SOURCES_A:%.cpp=%.o)
OBJECTS_A+=$(GEN_SOURCES:%.cpp=%.o)
DEPS_A   = $(SOURCES_A:%.cpp=%.dep)
DEPS_A   +=$(GEN_SOURCES:%.cpp=%.dep)

####    bash colorcodes
SHELL_RESET   := \e[39m\e[0m
SHELL_BLACK   := \e[30m
SHELL_RED     := \e[31m
SHELL_GREEN   := \e[32m
SHELL_YELLOW  := \e[33m
SHELL_BLUE    := \e[34m
SHELL_MAGENTA := \e[35m
SHELL_CYAN    := \e[36m
SHELL_BOLD    := \e[1m
####    use those colors
SHELL_COLOR_DEPENDENCIES := $(SHELL_BLUE)
SHELL_COLOR_COMPILE      := $(SHELL_CYAN)
SHELL_COLOR_LINK         := $(SHELL_YELLOW)
SHELL_COLOR_FLEX         := $(SHELL_MAGENTA)
SHELL_COLOR_BISON        := $(SHELL_GREEN)

COLOR_ECHO = function F() { \
        mkdir -p $(dir $@) ; \
        COLOR=$${1} ; \
        shift ; \
        echo -e "$${COLOR}$(SHELL_BOLD)"$$*: $@"$(SHELL_RESET)" ; \
        echo -e "$${COLOR}"\\tcaused by: $?"$(SHELL_RESET)" ; \
} ;

$(addprefix $(OBJ)/,%.o): AUTOMATIC_DEPENDENCY=$(@:$(OBJ)/%.o=$(DEP)/%.dep)
$(addprefix $(OBJ)/,%.o): $(addprefix $(SRC)/,%.cpp) $(addprefix $(GEN)/,$(BISON_HEADERS)) $(MAKEFILE)
	@$(COLOR_ECHO) F "$(SHELL_COLOR_DEPENDENCIES)" CREATE EXTERNAL RULE
	@mkdir -p $(dir $(AUTOMATIC_DEPENDENCY))
	$(CXXC) $(CXXFLAGS) -MM -MT $@ $< > $(AUTOMATIC_DEPENDENCY)
	@grep : $(AUTOMATIC_DEPENDENCY) > /dev/null \
		&& cat $(AUTOMATIC_DEPENDENCY) \
		|| rm -f $(AUTOMATIC_DEPENDENCY) \
	;
	@$(COLOR_ECHO) F "$(SHELL_COLOR_COMPILE)" COMPILE
	@mkdir -p $(dir $@)
	$(CXXC) $(CXXFLAGS) -c -o $@ $<

$(addprefix $(OBJ)/,%.o): $(addprefix $(GEN)/,%.cpp) $(MAKEFILE)
	@$(COLOR_ECHO) F "$(SHELL_COLOR_DEPENDENCIES)" CREATE EXTERNAL RULE
	@mkdir -p $(dir $(AUTOMATIC_DEPENDENCY))
	$(CXXC) $(CXXFLAGS) -MM -MT $@ $< > $(AUTOMATIC_DEPENDENCY)
	@grep : $(AUTOMATIC_DEPENDENCY) > /dev/null \
		&& cat $(AUTOMATIC_DEPENDENCY) \
		|| rm -f $(AUTOMATIC_DEPENDENCY) \
	;
	@$(COLOR_ECHO) F "$(SHELL_COLOR_COMPILE)" COMPILE
	@mkdir -p $(dir $@)
	$(CXXC) $(CXXFLAGS) -c -o $@ $<

.PHONY: default
default: $(BIN)/$(TARGET_A)

-include $(addprefix $(DEP)/,$(DEPS_A))

$(BIN)/$(TARGET_A): $(addprefix $(OBJ)/,$(OBJECTS_A))
	@$(COLOR_ECHO) F "$(SHELL_COLOR_LINK)" LINK
	@mkdir -p $(dir $@)
	$(CXXC) $(CXXFLAGS) -o $@ $^ $(LFLAGS)

.PHONY: clean
clean:
	rm -rf $(OBJ) $(DEP) $(GEN)

.PHONY: distclean
distclean:
	rm -rf $(OBJ) $(DEP) $(GEN) $(BIN)
	
.PHONY: flex
flex: $(GEN)/Scanner.cpp

.PHONY: lex
lex: flex

.PHONY: bison
bison: $(addprefix $(GEN)/,Parser.cpp $(BISON_HEADERS))

.PHONY: yacc
yacc: bison

$(GEN)/Scanner.cpp: Scanner.l
	@$(COLOR_ECHO) F "$(SHELL_COLOR_FLEX)" FLEX
	@mkdir -p $(dir $@)
	flex -o $@ Scanner.l

$(addprefix $(GEN)/,Parser.cpp $(BISON_HEADERS)): Parser.y
	@$(COLOR_ECHO) F "$(SHELL_COLOR_BISON)" BISON
	@mkdir -p $(dir $@)
	bison -o $(GEN)/Parser.cpp Parser.y

