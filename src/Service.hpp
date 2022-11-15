#pragma once
#include <string>
#include <vector>
#include <string.h>

#define MAX_STATUS_LEN		10
#define MAX_RUNLEVEL_LEN	23

struct Service {
	std::string name;
	std::string runlevel;
	std::string status;

	//void print(){
	//	printf("%p: %s is %s on %s\n", this, name.c_str(), status.c_str(), runlevel.c_str());
	//}

	char* pretty_render(size_t width){
		char* r;

		asprintf(
					&r,
					"%s%*s%*s%*s",
					this->name.c_str(),
					(int)(width-(this->name.size()+MAX_RUNLEVEL_LEN+MAX_STATUS_LEN+1)),
					" ",
					MAX_RUNLEVEL_LEN,
					this->runlevel.c_str(),
					MAX_STATUS_LEN+1,
					this->status.c_str()
				);

		return r;
	}
};

extern std::vector<Service*> services;
