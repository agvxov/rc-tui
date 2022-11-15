#pragma once

struct Service {
	std::string name;
	std::string runlevel;
	std::string status;

	//void print(){
	//	printf("%p: %s is %s on %s\n", this, name.c_str(), status.c_str(), runlevel.c_str());
	//}
};
