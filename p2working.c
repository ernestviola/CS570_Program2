#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <time.h>
#include <semaphore.h>
#include <assert.h>

//#include "CHK.h"
#include "p2.h"
#include "getword.h"

char s[STORAGE];
char* newargv[MAXITEM]; //pointers to each cmd
char* pipPtr[10]; //pointers to each location of pipe start
int pipLocation[10]; //finds the nulls for every pipe in newargv

int i,j, length, amp, inCount, outCount, fileIn, fileOut, errCode;

int ctr;
int pipCtr;

char* output;
char* input;

//===================START FUNCTIONS===================//


//===================START SIGNAL HANDLER FUNCTION===================//
// Handles signal for sigterm at kill
void myhandler(int signum) {
}
//====END FUNCTION===//

//===================START PRINT PROGRAM FUNCTION===================//
void printP() {
	printf("p2: ");
}
//====END FUNCTION===//

//===================START MAIN===================//


main() {	

	int kidpid, grandkid, children;
	char* home = getenv("HOME");
	char* cd = {"cd"};
	char* lsF = {"ls-F"};

	setpgid(0,getpid()); // pid of current process for terminal
	signal(SIGTERM, myhandler); // Is used to send the termination request

	for(;;) {

		i = length = ctr = amp = pipCtr = inCount = outCount = fileIn = fileOut = errCode = 0;
		printP();


		if (parse() == -1) { //EOF exit condition 
			break;
		}
		fflush( stdin );
		fflush( stderr );
		fflush( stdout );
		if(-1 == (kidpid=fork())) // Fork is used to create a second process to handle execvp
		{
			perror("Cannot fork");
			exit(1);
		} 
		else if (0 == kidpid) // child
		{
			int fildes[pipCtr][2];
			if (pipCtr != 0)
			{
				pipe(fildes[0]);
				fflush( stdin );
				fflush( stderr );
				fflush( stdout );
				if(-1 == (grandkid=fork())) //error for piping output of LHS is connected to input of RHS
				{
					perror("Cannot pipe");
					_exit(1);
				}
				if (grandkid == 0) {
					for (i = 0; i < pipCtr; i++) {
						fflush( stdin );
						fflush( stderr );
						fflush( stdout );
						if ( i == 0) {
						} else pipe(fildes[i]);
						children = fork();
						if (children = 0) {
						}
						else {
							if (i == 0) {
								dup2(fildes[i][1],STDOUT_FILENO);
								close(fildes[i][0]);
								execvp(newargv[0], newargv);
								perror("execvp");
								_exit(1);
							}
							else {
								dup2(fildes[i-1][1],STDOUT_FILENO);
								dup2(fildes[i][0],STDIN_FILENO);
								for (j = 0; j < i; j++)
								{
									close(fildes[j][1]);
									close(fildes[j][1]);
								}
								execvp(newargv[pipLocation[i]+1],newargv+pipLocation[i]+1);
							}
						}
					}
				}
				wait(NULL);
				dup2(fildes[0][0],STDIN_FILENO);
				close(fildes[0][1]);
				execvp(newargv[pipLocation[pipCtr-1]+1],newargv+pipLocation[pipCtr-1]+1);
				perror("execvp");
				_exit(1);
				continue;
			}// END PIPE
			// execvp takes in an array of pointers and increments until reaches null
			execvp(newargv[0],newargv);
			printf("Unknown command\n"); // is never reached unless execvp fails
			exit(9);
		}
		else {
			// parent checks for '&' if there is then skip and let child run @ same time {
			if (amp != 1) //Waits for child unless bg process is called
			while (wait(NULL) != kidpid);
			if (amp == 1) { // If bg process then print cmd and pid
				printf("%s [%d]\n",newargv[0], kidpid );
			}
		}		// end fork
	}
		// start kill
	killpg(getpgrp(), SIGTERM);
	printf("p2 terminated.\n");
	exit(0);


}

int parse() {
	char* w = s;
	
	for(;;) {

		length = getword(w);
		// printf("%s\n",w );
		if (length == -2) break;

		if (*w == '|' && length == 1) { // we have a pipe
			pipLocation[pipCtr] = ctr;
			newargv[ctr] = '\0'; //sets pipe in newargv to null
			ctr++;
			pipCtr++;
			w = w + length + 1; //next char after pipe
			pipPtr[pipCtr] = w; //address of next char after pipe
		}
		else { // we have a non-metacharacter
			newargv[ctr] = w;
			w = w + length + 1;
			ctr++;
		}

		if (length <= 0) break; //breaks loop when EOF is found
	} //end loop

	newargv[ctr -1] = '\0';
	return length;
}