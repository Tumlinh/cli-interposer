#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int (*real_main) (int, char **, char **);

int fake_main(int argc, char **argv, char **env)
{
	char max_args = 64;
	char **argv2 = malloc(max_args * sizeof(char *));
	argv2[0] = argv[0];

	// Get every args from stdin
	int len;
	char *line;
	size_t n = 0;
	len = getline(&line, &n, stdin);
	if (line[len - 1] == '\n')
		line[len - 1] = '\0';
	char *saveptr, *token;
	token = strtok_r(line, " ", &saveptr);
	argv2[1] = token;
	int argc2;
	for (argc2 = 2; (token = strtok_r(NULL, " ", &saveptr)) != NULL; argc2++)
		argv2[argc2] = token;
	
	// Call real main() with the new arguments
	return real_main(argc2, argv2, env);
}

// This function calls the real __libc_start_main() with a fake main()
int __libc_start_main(
	int (*main) (int, char **, char **),
	int argc, char **ubp_av,
	void (*init) (void),
	void (*fini) (void),
	void (*rtld_fini) (void),
	void (* stack_end))
{
	int (*real__libc_start_main)(
		int (*main) (int, char **, char **),
		int argc, char **ubp_av,
		void (*init) (void),
		void (*fini) (void),
		void (*rtld_fini) (void),
		void (*stack_end)
	) = dlsym(RTLD_NEXT, "__libc_start_main");
	real_main = main;
	
	return real__libc_start_main(fake_main, argc, ubp_av, init, fini, rtld_fini,
		stack_end);
}
