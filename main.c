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

	// Get true arguments via stdin
	int len;
	char *line;
	size_t n = 0;
	len = getline(&line, &n, stdin);
	if (line[len - 1] == '\n')
		line[len - 1] = '\0';
	char *saveptr, *tok;
	tok = strtok_r(line, " ", &saveptr);
	argv2[1] = tok;
	int argc2;
	for (argc2 = 2; (tok = strtok_r(NULL, " ", &saveptr)) != NULL; argc2++)
		argv2[argc2] = tok;
	
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

	if (argc > 0) {
		// Change program name (works against ps but top manages to
		// find the original name)
		// TODO: make it configurable
		char *new_name = "xxx";
		strcpy(ubp_av[0], new_name);

		// Erase following arguments to avoid blanks in the command line
		char *ptr;
		char *arg_end = ubp_av[argc - 1] + strlen (ubp_av[argc - 1]);
		for(ptr = ubp_av[0] + strlen(new_name); ptr < arg_end; ptr++)
			*ptr = '\0';
	}

	return real__libc_start_main(fake_main, argc, ubp_av, init, fini, rtld_fini,
		stack_end);
}
