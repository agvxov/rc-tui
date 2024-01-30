%{
/* This lexer parses INI format output by rc-status
    ( rc-status -f ini --all )
*/
	#include <string>
	#include <vector>
	#include "Service.hpp"
	static std::string runlevel;
	extern std::vector<Service*> services;
	void yyerror(char* str);
%}
%option noyywrap
%option nodefault
%x STATUS SWALLOW

identifier	[[:alnum:]]([[:alnum:]]|[-_ /])*[[:alnum:]]

%%

"["{identifier}"]"				{
									//printf("new runlevel: %s\n", yytext);
									runlevel = yytext;
								}
^{identifier}					{
									//printf("service found: %s", yytext);
									services.push_back(new Service);
									services.back()->name = yytext; 
									services.back()->runlevel = runlevel;
								}
=								{ BEGIN STATUS;
								}
\n								{ ; }
<STATUS>[[:alpha:]]+			{ BEGIN SWALLOW;
									//printf(" whichs status is: %s\n", yytext);
									services.back()->status = yytext;
									services.back()->status = '[' + services.back()->status + ']'; 
								}
<SWALLOW>.						{ ; }
<SWALLOW>\n						{ BEGIN INITIAL; }
<INITIAL,STATUS>[ \t]*			{ ; }
<INITIAL,STATUS>.				{ yyerror(yytext); }

%%
