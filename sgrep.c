/*
 * Simple grep to read through provided files and print out lines that contain the provided strings
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/* Linked List */

// linked list for holding the strings to search for
typedef struct node{
	char *term;
	struct node *next;
} node;

typedef struct list{
	// pointers to the head and tail for access and removal
	node *head, *tail;
} list;

// add a node to a linked list
void addNode(list *patterns, char *pattern){
	// if there is a tail, that's where to add this node)
	if(patterns->tail){
		// create some space for the new node
		patterns->tail->next = malloc(sizeof(node));
		// save it as the new tail
		patterns->tail = patterns->tail->next;
		// and give the pattern as its value
		patterns->tail->term = pattern;
	}else{
		// otherwise, create a fresh list with this node as its head and tail
		patterns->head = malloc(sizeof(node));
		patterns->head->term = pattern;
		patterns->tail = patterns->head;
	}
}


/* Configuration */

// the name of the program
static const char *progname;

// struct to hold the configuration information
typedef struct config{
	// flags for whether the search term is colored and line are numbered
	char col, num;
	// the linked list of patterns
	list *patterns;
} config;

// usage instructions for call with no arguments
void usage(){
	printf("Usage: %s [-n] [-c] [-h] [-p PATTERN]... FILE...\n", progname);
}

// more detailed usage instructions on help (and for a remidner to myself)
void help(){
	usage();
	printf("\nPrints lines in the provided files that match the provided patterns.\n");
	printf(" -c\tuse color to highlight matches\n");
	printf(" -h\thelp\n");
	printf(" -n\tinclude line numbers\n");
	printf(" -p\tpattern to match, can be included multiple times to check for multiple patterns\n");
	printf("Each PATTERN argument should be a single string to match, and each FILE argument should be the path to a file to check for the pattern.\n");
}

// unused flag warning
void warning(char *flag){
	usage();
	printf("\nWarning: unused flag %s!\n\n", flag);
}


/* Grep */

// to color the pattern (red) and return to normal
#define COLOR "\e[0;31m"
#define RESET "\e[0;0m"

// goes through each line to check if it contains each pattern
static void grepLine(config *conf, char *line, int lnum){
	// grab the first node in the linked list as the current pattern to look at
	node *pattern = conf->patterns->head;
	// check the line for each pattern
	while(pattern){
		// initialize the location of each match
		char *match;
		// and grab the length of the pattern
		int plen = strlen(pattern->term);
		// check if there actually is a match
		if(match = strstr(line, pattern->term)){
			// if the flag to print out the line number is on, start with that
			if(conf->num){
				printf("%d: ", lnum);
			}
			// if the flag to color the matching words is on, do that
			if(conf->col){
				// initialize the start of the rest of the line
				char *lstart = line;
				// then print out each match in the line
				while(match){
					// start with the beginning of the line up to the match 
					printf("%.*s", (int)(match-lstart), lstart);
					// then print the word
					printf(COLOR "%s" RESET, pattern->term);
					// then look at the rest of the line for another match
					lstart = match + plen;
					match = strstr(lstart, pattern->term);
				}
				// and finally put in the rest of the line
				printf("%s", lstart);
			} else {
				// otherwise, just print out the line
				printf("%s", line);
			}
		}
		pattern = pattern->next;
	}
}

// the buffer length
#define BUF_LEN 1024

// goes through all the files, checks that they exist, and calls grep on each line
static void grepFiles(config *conf, char **files){
	// initialize a string to hold each line of each file
	char bufline[BUF_LEN];
	// loop through the files
	while(*files){
		// open the file
		FILE *f = fopen(*files, "r");
		// if the file isn't real, print out an error message and exit
		if(!f){
			usage();
			printf("\nInvalid file %s!\n", *files);
			exit(2);
		}
		// otherwise, loop through the file (starting from line 1)
		int line = 1;
		while(fgets(bufline, BUF_LEN, f)){
			grepLine(conf, bufline, line++);
		}
		// then close the file and increment to the next one
		fclose(f);
		*files++;
	}
}


/* Main */

// main function that recieves all command line arguments
int main(int argc, char *argv[]){
	// first grab the program name
	progname = argv[0];
	// if no arguments have been provided, call usage and quit
	if(argc < 2){
		usage();
		exit(0);
	}
	// otherwise, it's time to initialize the config information
	config *conf = malloc(sizeof(config));
	// with an empty linked list
	conf->patterns = malloc(sizeof(list));
	// then go through the arguments (starting at the second one)
	argv++;
	// using a while loop because some values will be accessed together
	while(*argv && *argv[0] == '-'){
		// look for each of the flags
		if(!strcmp(*argv, "-h")){
			// if one of the flags is help, start by printing out the help page
			help();
			// with an extra space after it
			printf("\n");
		}else if(!strcmp(*argv, "-c")){
			// set the color flag to true
			conf->col = 1;
		}else if(!strcmp(*argv, "-n")){
			// set the number flag to true
			conf->num = 1;
		}else if(!strcmp(*argv, "-p")){
			// grab the pattern - first check if it's really there
			if(++argv == NULL){
				// if it's not a pattern, print an error message and exit
				usage();
				printf("\nMissing pattern!\n");
				exit(1);
			}
			// otherwise, add the pattern to the list
			addNode(conf->patterns, *argv);
		}else{
			// all other flags are invalid
			warning(*argv);
		}
		// and finally increment to the next arg
		argv++;
	}
	// if there are no arguments remaining - no file names, exit with an error
	if(argv == NULL){
		usage();
		printf("\nMissing file(s)!\n");
		exit(1);
	}
	// if everything went smoothly, go through the files
	grepFiles(conf, argv);
	
	return 0;
}
