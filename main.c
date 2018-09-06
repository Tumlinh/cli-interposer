#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_STDIN_ARGS 65536
#define ARGS_VIA_STDIN 0

static int (*real_main) (int, char **, char **);

int fake_main(int argc, char **argv, char **env)
{
	char **argv2 = malloc(MAX_STDIN_ARGS * sizeof(*argv2));
	int argc2 = 1;
	argv2[0] = argv[0];

	if (ARGS_VIA_STDIN) {
		// Get true arguments via stdin
		ssize_t len;
		char *line;
		size_t n = 0;
		len = getline(&line, &n, stdin);
		if (line[len - 1] == '\n')
			line[len - 1] = '\0';

		char *saveptr, *tok;
		tok = strtok_r(line, " ", &saveptr);
		if (tok != NULL) {
			argv2[1] = tok;
			for (argc2 = 2; tok != NULL; argc2++) {
				argv2[argc2] = tok;
				tok = strtok_r(NULL, " ", &saveptr);
			}
		}
	} else {
		// Hide original arguments (legacy mode)
		argc2 = argc;
		int i;
		for (i = 1; i < argc; i++) {
			// Copy argument
			argv2[i] = malloc((strlen(argv[i]) + 1) * sizeof(char));
			strcpy(argv2[i], argv[i]);

			// Erase argument
			size_t len = strlen(argv[i]);
			char *ptr = argv[i];
			while (ptr < argv[i] + len)
				*(ptr++) = '\0';
		}
	}

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
		size_t old_name_len = strlen(ubp_av[0]);
		char *new_name = "xxx";
		strcpy(ubp_av[0], new_name);

		char *ptr = ubp_av[0] + strlen(new_name);
		while (ptr < ubp_av[0] + old_name_len)
			*(ptr++) = '\0';
	}

	return real__libc_start_main(fake_main, argc, ubp_av, init, fini,
		rtld_fini, stack_end);
}
