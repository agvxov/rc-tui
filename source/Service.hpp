#pragma once
#include <string>
#include <vector>
#include <map>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define SERVICE_MAX_STATUS_LEN		10
#define SERVICE_MAX_RUNLEVEL_LEN	23
extern size_t SERVICE_max_name_len;

struct Service {
	static constexpr const
		char** cmd[] = {
			(const char *[]){"restart", "stop", NULL},
			(const char *[]){"start", NULL}
		};

	static std::map<Service*, const int> chld_table;

	static void chld(int ignore){
		pid_t pid;
		while((pid = waitpid(-1, NULL, 0)) > 0){
			auto i = Service::chld_table.begin();
			while(i != Service::chld_table.end()){
				if(i->second == pid){
					(*i).first->locked = false;
					Service::chld_table.erase(i);
					break;
				}
				++i;
			}
		}
	}

	std::string name;
	std::string runlevel;
	std::string status;
	bool locked = false;

	void change_status(const int &st){
		if(this->locked){ return; }
		char *const to_cmd = strdup(((char**)cmd[(bool)strcmp(this->status.c_str(), "[started]")])[st]);
		char *const name = strdup(this->name.substr(1, this->name.size()-2).c_str());
		char* argv[] = {"rc-service", name, to_cmd, NULL};

		const int cp = fork();
		if(cp == -1){ return; }
		this->locked = true;
		if(cp == 0){
			// do a pause to avoid inter process race conditions
			fclose(stdout);
			fclose(stderr);
			int iw = open("debug/debug.log", O_WRONLY | O_CREAT, 0644);
			int iv = open("debug/debug.log", O_WRONLY | O_CREAT, 0644);
			execvpe(argv[0], argv, NULL);
		}else{
			Service::chld_table.emplace(this, cp);
		}
		//raise(SIGWINCH);
	}

	//void print(){
	//	printf("%p: %s is %s on %s\n", this, name.c_str(), status.c_str(), runlevel.c_str());
	//}
};

extern std::vector<Service*> services;
