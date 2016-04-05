#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>

int main() {
	pid_t child = fork();
	char *argv[] = {"cat", "/tmp/optional", NULL};
	char *env[] = {NULL};
	if (!child) {
		fputs("[tracee] requesting trace\n", stderr);
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		raise(SIGSTOP);
		execve("/bin/cat", argv, env);
		fputs("[tracee] exiting\n", stderr);
		return 0;
	}
	int child_status;
	fputs("[tracer] waiting for tracees SIGSTOP\n", stderr);
	pid_t wait_result = waitpid(child, &child_status, 0);

	if(!WIFSTOPPED(child_status) || WSTOPSIG(child_status) != SIGSTOP) {
		fputs("[tracer] unexpected error 2, TODO\n", stderr);
	}

	fputs("[tracer] tracee ready to be traced\n", stderr);
	long options = PTRACE_O_TRACEEXEC | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEFORK | PTRACE_O_TRACESYSGOOD;
	fprintf(stderr, "[tracer] ptrace options: %lx\n", options);
	long ptrace_result = ptrace(PTRACE_SETOPTIONS, child, 0, (void*)options);
	if (ptrace_result < 0) {
		fputs("[tracer] unexpected error 3, TODO\n", stderr);
		exit(1);
	}
	ptrace(PTRACE_SYSCALL, child, 0, 0);

	int expecting_syscall_exit_stop = 0;
	while (1) {
		fputs("[tracer] loop\n", stderr);
		wait_result = waitpid(child, &child_status, 0);
		fputs("[tracer] got child event\n", stderr);
		if (wait_result < 0) {
			fputs("[tracer] unexpected error 1, TODO\n", stderr);
			exit(1);
		}
		if(WIFEXITED(child_status)) {
			fprintf(stderr, "[tracer] child exited with %d\n", WEXITSTATUS(child_status));
			break;
		}
		if (WIFSIGNALED(child_status)) {
			fprintf(stderr, "[tracer] child terminated by signal %d\n", WTERMSIG(child_status));
			break;
		}
		if (WIFSTOPPED(child_status)) {
			int sig = WSTOPSIG(child_status);
			if (sig == (SIGTRAP | 0x80)) {
				if(expecting_syscall_exit_stop) {
					fputs("[tracer] observed syscall exit stop\n", stderr);
					expecting_syscall_exit_stop = 0;
					ptrace(PTRACE_SYSCALL, child, 0, 0);
				} else {
					fputs("[tracer] observed syscall enter stop\n", stderr);
					expecting_syscall_exit_stop = 1;
					ptrace(PTRACE_SYSCALL, child, 0, 0);
				}
			} else {
				if (sig == SIGSTOP) {
					fputs("[tracer] stray SIGSTOP!\n", stderr);
					ptrace(PTRACE_SYSCALL, child, 0, 0);

				} else {
					fprintf(stderr, "[tracer] unhandled signal %d intercepted, forwarding.\n", sig);
					ptrace(PTRACE_SYSCALL, child, 0, sig);
				}
			}
		} else if (WIFCONTINUED(child_status)) {
			fputs("[tracer] child is continuing\n", stderr);
		} else {
			fprintf(stderr, "[tracer] unexpected child status %d\n", child_status);
		}
	}
	return 0;
}
