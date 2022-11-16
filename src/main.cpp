#include <vector>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include "rc_lexer.h"
#include "Service.hpp"
#include "tui.hpp"


using namespace std; 


bool init();
[[ noreturn ]] void quit(int ignore);

inline std::vector<Service*> services;
bool running = init();

signed main(int argc, char* argv[]){
	if(not running){ return 1; }

	tui_redraw();
	while(true){
		if(tui_control(getch())){
			tui_draw();
		}
	}

	return 0;
}

bool init(){
	// ### Read rc-status ###
	int fd[2];
	int cp;
	char buf;
	char* argv[] = {"rc-status", "-f", "ini", NULL};
	string rcoutput;

	if(pipe(fd) == -1){ return false; }
	cp = fork();
	if(cp == -1){ return false; }
	if(cp == 0){
		dup2(fd[1], STDOUT_FILENO);
		close(fd[0]);
		close(fd[1]);
		execvpe(argv[0], argv, NULL);
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

	// ### Handle signals ###
	signal(SIGABRT, quit);
	signal(SIGINT, quit);
	signal(SIGSEGV, quit);
	signal(SIGTERM, quit);

	// ### Init ncurses ###
	if(not tui_init()){ return false; }

	return true;
}

[[ noreturn ]] void yyerror(char* str){
	printf("\nFlex scanner jammed on: %s\n", str);
	raise(SIGABRT);
}

[[ noreturn ]] void quit(int ignore){
	tui_quit();
	exit(1);
}
