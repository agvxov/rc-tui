#include <vector>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <ncurses.h>

#include "tui.hpp"
#include "rc_lexer.h"

#include "opts.hpp"
#include "Service.hpp"

using namespace std; 

inline const char * const version =
# include "version.inc"
;

inline std::vector<Service*> services;
inline std::vector<Service*> service_results;

std::map<Service*, const int> Service::chld_table;
char **Service::runlevels;
size_t SERVICE_MAX_NAME_LEN = 0;

bool is_root;

bool init();
[[ noreturn ]] void quit([[ maybe_unused ]] int ignore);

// ###

signed main(const int argc, const char* argv[]){
	opts(argc, argv);

	bool running = init();
	if(not running){ return 1; }

	tui_redraw();
	while(true){
		if(tui_control(wgetch(stdscr))){
			tui_draw();
		}
	}

	return 0;
}

bool init(){
	// ### Get root status
	is_root = (getuid() == 0);

	// ### Read available runlevels ###
	/* XXX: This operation could be safer by allocating the first time enties are read,
	 *       because now if the directory is modified between the 2 loops the application
	 *       may not pick up all run leves / crash.
	 *      However, its such an edge case that i cant be bothered. DIY.
	 */
	#define runleveld "/etc/runlevels/"
	DIR * d = opendir(runleveld);
	if (not d) {
		fputs("Error reading runlevel list from '" runleveld "'.\n", stderr);
	}
	dirent * entry;
	int rlc = -2;	// NOTE: "." and ".." will always be present
	while ((entry = readdir(d)) != NULL) {
		++rlc;
	}
	Service::runlevels = (char**)malloc((sizeof(char*) * rlc) + 1);
	rewinddir(d);
	for (int i = 0; (entry = readdir(d)) != NULL;) {
		if (not strcmp(entry->d_name, ".")
		or  not strcmp(entry->d_name, "..")) {
			continue;
		}
		Service::runlevels[i] = strdup(entry->d_name);
		++i;
	}
	Service::runlevels[rlc] = NULL;
	closedir(d);

	// ### Read rc-status ###
	int fd[2];
	int cp;
	char buf;
	const char * const argv[] = {"rc-status", "--all", "-f", "ini", NULL};
	string rcoutput;

	if(pipe(fd) == -1){ return false; }
	cp = fork();
	if(cp == -1){ return false; }
	if(cp == 0){
		dup2(fd[1], STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		execvpe(argv[0], (char**)argv, NULL);
	}else{
		close(fd[1]);
		while(read(fd[0], &buf, 1)){
			rcoutput += buf;
		}
		wait(NULL);
	}

	// ### Lex rc-status output ###
	auto buffer = yy_scan_string(rcoutput.c_str());
	yylex();
	yy_delete_buffer(buffer);

	service_results.resize(services.size());
	copy(services.begin(), services.end(), service_results.begin());

	// ### Handle signals ###
	signal(SIGABRT, quit);
	signal(SIGINT,  quit);
	signal(SIGSEGV, quit);
	signal(SIGTERM, quit);
	signal(SIGCHLD, Service::chld);
	//signal(SIGWINCH, tui_redraw);

	// ### Init ncurses ###
	if(not tui_init()){ return false; }

	return true;
}

[[ noreturn ]] void yyerror(char* str){
	printf("\nFlex scanner jammed on: %s\n", str);
	raise(SIGABRT);
}

[[ noreturn ]] void quit([[ maybe_unused ]]int ignore){
	tui_quit();
	exit(1);
}
