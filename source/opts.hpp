#ifndef OPTS_HPP
#define OPTS_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char * const version;

void usage() {
	printf(
		"%s\n"
		"Interactive terminal interface for managing open-rc services.\n"
		"rc-rui <options>\n"
		"\t-v --version    : print verion and quit\n"
		"\t-h --help       : print help and quit\n",
		version
	);
}

void opts(int argc, const char * * argv) {
	(void)argc;
	for (const char * * opt = argv + 1; *opt != NULL; opt++) {
		if (not strcmp(*opt, "-h")
		or  not strcmp(*opt, "--help")) {
			usage();
			exit(0);
		}
		if (not strcmp(*opt, "-v")
		or  not strcmp(*opt, "--version")) {
			puts(version);
			exit(0);
		}

		printf("Unknown option: '%s'.\n", *opt);
		exit(1);
	}
}

#endif
