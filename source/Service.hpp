#ifndef SERVICE_HPP
#define SERVICE_HPP
#include <string>
#include <vector>
#include <map>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "tui.hpp"

#define SERVICE_MAX_STATUS_LEN		10
#define SERVICE_MAX_RUNLEVEL_LEN	23
extern size_t SERVICE_MAX_NAME_LEN;

struct Service {
	static constexpr const
		char** cmd[] = {
			(const char *[]){"restart", "stop", NULL},
			(const char *[]){"start", NULL}
		};
	static char **runlevels;

	static std::map<Service*, const int> chld_table;

	static void chld([[ maybe_unused ]] int ignore){
		pid_t pid;
		while((pid = waitpid(-1, NULL, 0)) > 0){
			auto i = Service::chld_table.begin();
			while(i != Service::chld_table.end()){
				if(i->second == pid){
					(*i).first->locked = false;
					Service::chld_table.erase(i);
					tui_draw();
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

	void change_status(const int st){
		if(this->locked){ return; }
		const char * const to_cmd = strdup(((char**)cmd[(bool)strcmp(this->status.c_str(), "[started]")])[st]);
		const char * const argv[] = {"rc-service", this->name.c_str(), to_cmd, NULL};

		const int cp = fork();
		if(cp == -1){ return; }
		this->locked = true;
		if(cp == 0){
			// XXX: do a pause to avoid inter process race conditions
			fclose(stdout);
			fclose(stderr);
			#if DEBUG == 1
			[[ maybe_unused ]] int iw = open("debug/debug.log", O_WRONLY | O_CREAT, 0644);
			[[ maybe_unused ]] int iv = open("debug/debug.log", O_WRONLY | O_CREAT, 0644);
			#endif
			execvpe(argv[0], (char**)argv, NULL);
		}else{
			Service::chld_table.emplace(this, cp);
		}
	}

	void change_runlevel(const int rl) {
		const char * const argv[] = {"rc-update", "add", (char*)this->name.c_str(), Service::runlevels[rl], NULL};

		const int cp = fork();
		if(cp == -1){ return; }
		if(cp == 0){
			fclose(stdout);
			fclose(stderr);
			#if DEBUG == 1
			[[ maybe_unused ]] int iw = open("debug/debug.log", O_WRONLY | O_CREAT, 0644);
			[[ maybe_unused ]] int iv = open("debug/debug.log", O_WRONLY | O_CREAT, 0644);
			#endif
			execvpe(argv[0], (char**)argv, NULL);
		}else{
			Service::chld_table.emplace(this, cp);
		}
	}

#if 0
	void print(){
		printf("%p: %s is %s on %s\n", this, name.c_str(), status.c_str(), runlevel.c_str());
	}
#endif
};

extern std::vector<Service*> services;
extern std::vector<Service*> service_results;

typedef void (Service::*menu_callback_t)(int);

#endif
