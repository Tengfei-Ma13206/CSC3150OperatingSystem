#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	char *sig[] = {NULL, "SIGHUP", "SIGINT", "SIGQUIT",
			"SIGILL", "SIGTRAP", "SIGABRT", "SIGBUS",
			"SIGFPE", "SIGKILL", NULL, "SIGSEGV",
			NULL, "SIGPIPE", "SIGALRM", "SIGTERM"};
	int status;
	pid_t pid, wait_pid;

	/* fork a child process */
	printf("Process start to fork\n");
	pid = fork();
	
	/* execute test program */ 
	if (pid == -1)
	{
		perror("fork error");
		exit(1);
	}
	else if(pid == 0)//child process
	{
		char *argument[argc];
		for (int i = 0; i < (argc - 1); i++)
		{
			argument[i] = argv[i + 1];
		}
		argument[argc - 1] = NULL;

		printf("I'm the Child Process, my pid = %d\n",getpid());
		printf("Child process start to execute test program:\n");

		execve(argument[0],argument,NULL);

		printf("Continue to run original child process\n");
		perror("execve");
		exit(EXIT_FAILURE);
	}
	else//parent process
	{
		printf("I'm the Parent Process, my pid = %d\n", getpid());
	/* wait for child process terminates */
		wait_pid = waitpid(pid, &status, WUNTRACED);//WUNTRACED
	/* check child process'  termination status */
		if(wait_pid == -1)
		{
			perror("waitpid");
			exit(EXIT_FAILURE);
		}

		printf("Parent process receives SIGCHLD signal\n");//SIGCHLD

		if (WIFEXITED(status))
		{
			printf("Normal termination with EXIT STATUS = %d\n", WEXITSTATUS(status));
			exit(EXIT_SUCCESS);
		}
		else if (WIFSIGNALED(status))
		{
			printf("child process get %s signal\n", sig[WTERMSIG(status)]);
			exit(EXIT_SUCCESS);
		}
		else if (WIFSTOPPED(status))
		{
			printf("child process get SIGSTOP signal\n");
		}
		else
		{
			printf("Child process continues to run\n");
		}
	}
	return 0;
}

