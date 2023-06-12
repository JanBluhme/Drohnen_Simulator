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
SHELL_COLOR_DEPENDENCIES        := $(SHELL_BLUE)
SHELL_COLOR_COMPILE             := $(SHELL_CYAN)
SHELL_COLOR_LINK                := $(SHELL_YELLOW)
SHELL_COLOR_COMPILE_ENVIRONMENT := $(SHELL_MAGENTA)
SHELL_COLOR_COMPILE_COMMANDS    := $(SHELL_GREEN)

COLOR_ECHO = function F() { \
        COLOR=$${1} ; \
        shift ; \
        echo -e "$${COLOR}$(SHELL_BOLD)"$$*: $@"$(SHELL_RESET)" ; \
        echo -e "$${COLOR}"\\tcaused by: $?"$(SHELL_RESET)" ; \
} ;

# $${*} : CXXFLAGS
# -M    : all headers
# -MM   : no system headers
# -MT X : make target (full filename in target list)
# -MF X : outputfile, no output on error
# -MP   : "???phony???targets" for all headers???
#         an empty rule is generated for each header
#         should fix problems when moving headers arround...
COMPILE_DEP = function F() { \
	$(COLOR_ECHO) F "$(SHELL_COLOR_DEPENDENCIES)" CREATE EXTERNAL RULE ; \
	mkdir -p $(dir $(AUTOMATIC_DEPENDENCY))                            ; \
	rm -f $(AUTOMATIC_DEPENDENCY)                                      ; \
	   $(CXXC) $${*} -MM     -MT $@ $<                              \
	&& $(CXXC) $${*} -M  -MP -MT $@ $< -MF $(AUTOMATIC_DEPENDENCY); \
} ;
ECHOED_COMMAND = function F() {   \
	echo "$${*}"            ; \
	$${*}                   ; \
} ;
COMPILE = function F() {                                    \
	$(COMPILE_DEP) F $${*}                            ; \
	$(COLOR_ECHO) F "$(SHELL_COLOR_COMPILE)" COMPILE  ; \
	mkdir -p $(dir $@)                                ; \
	$(ECHOED_COMMAND) F $(CXXC) $${*} -c -o $@ $<     ; \
} ;
COMPILE_SO = function F() {                                                                                  \
	$(COMPILE_DEP) F $${*}                                                                             ; \
	$(COLOR_ECHO) F "$(SHELL_COLOR_LINK)" COMPILE AND LINK                                             ; \
	mkdir -p $(dir $@)                                                                                 ; \
	$(ECHOED_COMMAND) F $(CXXC) -shared $${*} -o $@ -pthread $< $(addprefix $(SRC)/,$(SOCKET_SOURCES)) ; \
} ;
UPDATE_GIT_HASH = function F() {                                               \
	SINK=git.hash;                                                         \
	if [ ! -f "$${SINK}" ] ; then                                          \
		touch "$${SINK}" ;                                             \
	fi ;                                                                   \
	DIRTY="" ;                                                             \
	PORCELAIN_COUNT=$$(git status --porcelain | wc -l) ;                   \
	if [ $${PORCELAIN_COUNT} -gt 0 ] ; then                                \
		UNTRACKED_COUNT=$$(git status --porcelain | grep ?? | wc -l) ; \
		if [ $${UNTRACKED_COUNT} -ne $${PORCELAIN_COUNT} ] ; then      \
			DIRTY="-ytrid" ;                                       \
		fi ;                                                           \
	fi ;                                                                   \
	HEAD_HASH=$$(echo "\"$$(git rev-parse --short HEAD)$${DIRTY}\"");      \
	if diff $${SINK} <(echo "$${HEAD_HASH}") > /dev/null; then             \
		echo "$${SINK} up to date" ;                                   \
	else                                                                   \
		echo "$${HEAD_HASH}" > $${SINK};                               \
	fi ;                                                                   \
} ;

CLEAN_RESOURCE_FILENAMES = function F() {                               \
	INPUT_DIR=$${1} ;                                               \
	mapfile -t FILES < <(find $${INPUT_DIR} -type f) ;              \
	for INPUT_FILE in "$${FILES[@]}" ; do                           \
		S=$$(echo "$${INPUT_FILE}" | tr ' ' '_' | tr '-' '_') ; \
		if [[ "$${S:0:1}" == ?([0-9]) ]] ; then                 \
			S=X_$${S} ;                                     \
		fi ;                                                    \
		if [[ "$${INPUT_FILE}" != "$${S}" ]] ; then             \
			echo "mv \"$${INPUT_FILE}\" -> \"$${S}\"" ;     \
			mv "$${INPUT_FILE}" $${S} ;                     \
		fi ;                                                    \
	done ;                                                          \
} ;
