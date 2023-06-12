#pragma once
#include <pthread.h>
#include <string>

inline void name_this_thread(std::string const& name) {
	//extern const char* __progname;
	//std::string progname(__progname);
	std::string progname("sim");
	progname+="::";
	progname+=name;
	//pthread_setname_np(pthread_self(), (progname + "::" + name).c_str());
	pthread_setname_np(pthread_self(), progname.c_str());
}
