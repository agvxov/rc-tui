#pragma once
#include <string>
#include <vector>
#include <string.h>

#define MAX_STATUS_LEN		10
#define MAX_RUNLEVEL_LEN	23

struct Service {
	static constexpr const char** cmd[] = {(const char *[]){"restart", "stop"}, (const char *[]){"start"}};

	std::string name;
	std::string runlevel;
	std::string status;

	void change_status(const char *const st){

	}

	//void print(){
	//	printf("%p: %s is %s on %s\n", this, name.c_str(), status.c_str(), runlevel.c_str());
	//}
};

extern std::vector<Service*> services;
