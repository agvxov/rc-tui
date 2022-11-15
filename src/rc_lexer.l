%{
	#include <string>
	#include <vector>
	#include "Service.hpp"
	static std::string runlevel;
	extern std::vector<Service*> services;
	void yyerror(char* str);
%}
%option noyywrap
%option nodefault
%x STATUS
%%

"["([[:alpha:]]|" "|"/")+"]"	{
									//printf("new runlevel: %s\n", yytext);
									runlevel = yytext;
								}
^([[:alnum:]]|"-"|"_")+			{
									//printf("service found: %s", yytext);
									services.push_back(new Service);
									services.back()->name = yytext; 
									services.back()->name = '[' + services.back()->name + ']'; 
									services.back()->runlevel = runlevel;
								}
=								{ BEGIN STATUS;
								}
<STATUS>[[:alpha:]]+ 			{ BEGIN INITIAL;
									//printf(" whichs status is: %s\n", yytext);
									services.back()->status = yytext;
									services.back()->status = '[' + services.back()->status + ']'; 
								}
\n								{  }
<INITIAL,STATUS>[ \t]*			{  }
<INITIAL,STATUS>.				{ yyerror(yytext); }

%%
