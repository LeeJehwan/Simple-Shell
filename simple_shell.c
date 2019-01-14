#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>	
#include <fcntl.h>	

void removeBSN(char str[]);					//remove back slash n
int getMode(char* str);						//get command mode
void modeOne(char buf[]);					//only one command(not included '>' or '|')
void modePipe_re(char text[], int fd[]);	//chain pipe
void modePipe(char comm[]);					//pipe mode
void modeRedirection(char comm[]);			//redirection mode

enum mode{ONE, PIPE, REDIRECTION};

void removeBSN(char str[]){
    str[strlen(str)-1] = '\0';
}

int getMode(char* str) {
	if (strchr(str, '|') != NULL)
		return PIPE;
	else if (strchr(str, '>') != NULL)
		return REDIRECTION;
	else
		return ONE;
}

void modeOne(char buf[]) {
	pid_t pid;
	char* comm[20];
	comm[0] = strtok(buf, " ");

	int k;
	for (k = 1; ; k++) {
		comm[k] = strtok(NULL, " ");
		if (comm[k] == NULL)
			break;
	}
	comm[k] = 0;

	if (strncmp(comm[0], "exit", 4) == 0) {
		printf("bye\n");
		exit(0);
	}
	int isBack = 0;
	if (strncmp(comm[k - 1], "&", 1) == 0) {
		comm[k - 1] = 0;
		isBack = 1;
	}
	if (strncmp(comm[0], "cd", 2) == 0)
		chdir(comm[1]);
	else {
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Fork Error");
		}
		else if (pid == 0) {
			if (execvp(comm[0], comm) == -1)
				fprintf(stderr, "Command Not Found\n\n");
			exit(0);
		}
		else if (pid > 0) {
			if (isBack == 0) {
				int status;
				waitpid(pid, &status, WUNTRACED);
			}
		}
	}
}

void modePipe_re(char text[], int fd[]) {
	char* comm[10];
	pid_t pid;

	comm[0] = strtok(text, " ");
	int k;
	for (k = 1; ; k++) {
		comm[k] = strtok(NULL, " ");
		if (comm[k] == NULL)
			break;
	}
	comm[k] = '\0';

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork Error");
	}
	else if (pid == 0) {
		close(0);
		dup(fd[0]);
		close(fd[0]);
		close(fd[1]);

		if (execvp(comm[0], comm) == -1)
			fprintf(stderr, "Command Not Found\n\n");
		exit(0);
	}
	else if (pid > 0) {
		close(fd[0]);
		close(fd[1]);

		int status;
		waitpid(pid, &status, WUNTRACED);
	}
}

void modePipe(char comm[]) {
	char* text1 = NULL;
	char* text2 = NULL;
	char* text3 = NULL;
	char* comm1[10];
	char* comm2[10];
	int fd[2];
	int fd2[2];
	pid_t pid;

	text1 = strtok(comm, "|");
	text2 = strtok(NULL, "\n");
	strcat(text1, "\0");

	int mode = -1;
	mode = getMode(text2);
	if (mode == PIPE) {
		text2 = strtok(text2, "|");
		text3 = strtok(NULL, "\n");
		strcat(text2, "\0");
		strcat(text3, "\0");
	}
	else if (mode == ONE)
		strcat(text2, "\0");

	comm1[0] = strtok(text1, " ");
	int k;
	for (k = 1; ; k++) {
		comm1[k] = strtok(NULL, " ");
		if (comm1[k] == NULL)
			break;
	}
	comm1[k] = '\0';

	comm2[0] = strtok(text2, " ");
	for (k = 1; ; k++) {
		comm2[k] = strtok(NULL, " ");
		if (comm2[k] == NULL)
			break;
	}
	comm2[k] = '\0';

	pipe(fd);
	if (mode == PIPE)
		pipe(fd2);

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork Error");
	}
	else if (pid == 0) {
		close(1);
		dup(fd[1]);
		close(fd[0]);
		close(fd[1]);

		if (execvp(comm1[0], comm1) == -1)
			fprintf(stderr, "Command Not Found\n\n");
		exit(0);
	}
	else if (pid > 0) {
		int status;
		waitpid(pid, &status, WUNTRACED);
	}

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork Error");
	}
	else if (pid == 0) {
		close(0);
		dup(fd[0]);
		if (mode == PIPE) {
			close(1);
			dup(fd2[1]);
		}
		close(fd[0]);
		close(fd[1]);
		
		if (mode == PIPE) {
			close(fd2[0]);
			close(fd2[0]);
		}

		if (execvp(comm2[0], comm2) == -1)
			fprintf(stderr, "Command Not Found\n\n");
		exit(0);
	}
	else if (pid > 0) {
		close(fd[0]);
		close(fd[1]);

		int status;
		waitpid(pid, &status, WUNTRACED);
	}
	if (mode == PIPE)
		modePipe_re(text3, fd2);
}

void modeRedirection(char comm[]) {
	char* text1 = NULL;
	char* text2 = NULL;
	char* comm1[10];
	char* comm2[10];
	pid_t pid;
	int fd;

	text1 = strtok(comm, ">");
	text2 = strtok(NULL, ">");

	int k;
	comm1[0] = strtok(text1, " ");
	for (k = 1; ; k++) {
		comm1[k] = strtok(NULL, " ");
		if (comm1[k] == NULL)
			break;
	}
	comm1[k] = '\0';

	comm2[0] = strtok(text2, " ");
	for (k = 1; ; k++) {
		comm2[k] = strtok(NULL, " ");
		if (comm2[k] == NULL)
			break;
	}
	comm2[k] = '\0';

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "Fork Error");
	}
	else if (pid == 0) {
		fd = open(comm2[0], O_RDWR | O_CREAT | O_TRUNC, 00777);
		dup2(fd, 1);
		close(fd);
		if (execvp(comm1[0], comm1) == -1)
			fprintf(stderr, "Command Not Found\n\n");
		exit(0);
	}
	else if (pid > 0) {
		int status;
		waitpid(pid, &status, WUNTRACED);
	}
}

int main() {
	char buf[200];
	printf("JeHwan shell start!\n");
	while (1) {
		printf("LeeJeHwan$ ");
		fgets(buf, sizeof(buf), stdin);
		if (buf[0] == '\n')
			continue;
		removeBSN(buf);
		int mode = getMode(buf);

		if (mode == ONE)
			modeOne(buf);
		else if (mode == PIPE)
			modePipe(buf);
		else if (mode == REDIRECTION)
			modeRedirection(buf);
	}
}