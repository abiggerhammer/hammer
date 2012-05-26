// -*- c-basic-offset: 8; tab-width: 8 -*-
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main (int argc, char** argv) {
	// Argv[1] is the message
	// argv[2] is the name of the child
	// the rest are passed to the child
	if (argc < 3) return 1;
	printf ("\x1b[1;32m*\x1b[0m %s...", argv[1]);

	char *cbuf = malloc(4096);
	size_t cbuf_cap = 4096;
	size_t cbuf_len = 0;
	char cbuf2[4096];

	argc-=2;
	argv+=1;
	for (int i = 0; i < argc; i++)
		argv[i] = argv[i+1];
	argv[argc] = NULL;
	int fd[2];

	if (socketpair(AF_LOCAL, SOCK_STREAM, 0, fd) == -1) {
		printf ("\n");
		execvp(argv[0], argv);
	}
	int pid;
	if ((pid=fork()) == -1) {
		printf ("\n");
		execvp(argv[0], argv);
	} else if (pid == 0) { // child
		printf ("Child");
		close(fd[1]);
		dup2(fd[0],0);
		dup2(0,1);
		dup2(0,2);
		execvp(argv[0], argv);
	} else { // parent
		char* cols1 = getenv("COLUMNS");
		int cols;
		if (cols1) { cols = atoi(cols1); } else { cols = 80; }
		close(fd[0]);
		int delta = 1;
		while (delta != 0) {
			delta = read (fd[1], cbuf2, 4096);
			while ((cbuf_len + delta) >= cbuf_cap)
				cbuf = realloc(cbuf, cbuf_cap *= 2);
			memcpy(cbuf + cbuf_len, cbuf2, delta);
			cbuf_len += delta;
		}
		
		int status;
		wait(&status);
		fflush (NULL);
		if (status) {
			fprintf (stderr, "\x1b[%dG\x1b[1;34m[\x1b[1;31m!!\x1b[1;34m]\x1b[0m\n", cols-4);
		} else if (cbuf_len) {
			fprintf (stderr, "\x1b[%dG\x1b[1;34m[\x1b[0;33mWW\x1b[1;34m]\x1b[0m\n", cols-4);
		} else {
			fprintf (stderr, "\x1b[%dG\x1b[1;34m[\x1b[1;32mOK\x1b[1;34m]\x1b[0m\n", cols-4);
		}
		fflush (NULL);
		write(2, cbuf, cbuf_len);
		free(cbuf);
		fflush (NULL);
		return WEXITSTATUS(status);
	}
}
