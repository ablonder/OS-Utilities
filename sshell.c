#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16

// the complete prompt that was called
char prompt[CMDLINE_MAX];

/*
 * Simple queue to manage a pipeline of jobs
 */
struct queue{
	char *command[ARGS_MAX];
	struct queue *next;
	int result;
};
struct queue *head;
struct queue *tail;

/*
 * Runs non built-in commands using the fork exec wait cycle with pipes
 */
int run(char **command, int *pipein, int *pipeout){
	int child = fork();
	if(child == 0){
		if(pipein){
			close(pipein[1]);
			dup2(pipein[0], STDIN_FILENO);
			close(pipein[0]);
		}
		if(pipeout){
			close(pipeout[0]);
			dup2(pipeout[1], STDOUT_FILENO);
			close(pipeout[1]);
		}
		int exitval = execvp(command[0], command);
		printf("Error: execution of %s failed!\n", command[0]);
		exit(exitval);
	} else if(child == -1) {
		printf("Process failed to fork: Insufficient resources available!\n");
		return -1;
	}
	/* The parent returns the child pid */
	return child;
}

/*
 * Handles running processes from the queue and printing out the result
 */
void runPipe(char *target){
	/* Set up the target file if provided */
	int tf;
	if(target){
		if((tf = open(target, O_WRONLY | O_CREAT, 0666)) == -1){
			fprintf(stderr, "Error: file %s failed to open!\n", target);
		}
	}
	/* Run the pipeline of processes */
	struct queue *job = head;
	int *pin = NULL;
	int *pout = NULL;
	while(job){
		/* Builtin command cd (never part of a pipeline) */
		if(!strcmp(job->command[0], "cd")){
			if(job->command[1]){
				job->result = chdir(job->command[1]);
			}else{
				printf("Error: Missing target directory!\n");
				job->result = -1;
			}
		}else{
			/* Handles non-builtin commands with piping */
			pin = pout;
			if((job->next)||(tf)){
				pout = malloc(2*sizeof(int));
				pipe(pout);
				if(!job->next)
					pout[1] = tf;
			}
			job->result = run(job->command, pin, pout);
			if(pin){
				close(pin[0]);
				close(pin[1]);
				free(pin);
			}
		}
		job = job->next;
	}
	/* Close the target file */
	if(tf){
		close(tf);
		free(pout);
		target = NULL;
	}
	/* Collect and then print out the results */
	job = head;
	while(job){
		if(strcmp(job->command[0], "cd") + (job->result != -1)){
			waitpid(job->result, &(job->result), 0);
		}
		job = job->next;
	}
	printf("Return status value of '%s':", prompt);
	while(head){
		printf(" [%d]", head->result);
		struct queue *tmp = head;
		head = head->next;
		tmp->next = NULL;
		free(tmp);
	}
	printf("\n");
}



/*
 * Parse input into a pipeline of commands each of which is broken into an array of arguments
 */
void parse(char *cmd){
	/* Split the input into a pipeline of commands */
	head = malloc(sizeof(struct queue));
	head->command[0] = strtok(cmd, "|");
	tail = head;
	char* c;
	while((c = strtok(NULL, "|"))){
		tail->next = malloc(sizeof(struct queue));
		tail = tail->next;
		tail->command[0] = c;
	}
	/* And split each command into arguments (starting with the name of the command) */
	struct queue *job = head;
	while(job){
		job->command[0] = strtok(job->command[0], " ");
		int argc = 1;
		while((job->command[argc++] = strtok(NULL, " ")));
		while(argc < ARGS_MAX)
			job->command[argc++] = NULL;
		job = job->next;
	}
}

#define BOLD_TEXT "\e[1m"
#define RESET_TEXT "\e[m"

/*
 * Handles basic input loop
 */
int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;

                /* Print prompt */
                printf(BOLD_TEXT "sshell$ " RESET_TEXT);
                fflush(stdout);

                /* Get command line */
                fgets(cmd, CMDLINE_MAX, stdin);

                /* Print command line if stdin is not provided by terminal */
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }

                /* Remove trailing newline from command line */
                nl = strchr(cmd, '\n');
                if (nl)
                        *nl = '\0';

                /* Builtin command exit */
                if (!strcmp(cmd, "exit")) {
                        printf("Bye!\n");
                        break;
                }
                
                /* Copy over the prompt */
                strcpy(prompt, cmd);
                
                /* Builtin command pwd */
                if(!strcmp(cmd, "pwd")){
                	char buff[CMDLINE_MAX];
                	printf("Current working directory: %s\n", getcwd(buff, CMDLINE_MAX));
                	continue;
                }
                
                /* For all other commands, parse into target and command(s) with arguments and run */
                strtok(cmd, ">");
                char *target = strtok(NULL, ">");
                parse(cmd);
                runPipe(target);
        }

        return EXIT_SUCCESS;
}
