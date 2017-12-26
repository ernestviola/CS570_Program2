/* 
Ernest Viola
Professor John Carroll
CS570
Program 4
December 6, 2017
Handled '< >' and '> <' as errors as it was not specified to what functionality was supposed to be given
*/

// all includes


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
char* pipPtr[100]; //pointers to each location of pipe start
int pipLocation[100]; //finds the nulls for every pipe in newargv

int i,j, place, length, amp, inCount, outCount, fileIn, fileOut, errCode;

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

		if (length == -2) // Special error message for unclosed parenthesis
		{
			fprintf(stderr, "No closing parenthesis\n");
			continue;
		}

		if (ctr == 2) // ls-F with current directory
		{
			if (strcmp(newargv[0],lsF) == 0)
			{
				i = 0;
				DIR* d;
				struct stat statbuf;
				struct dirent* dir;
				int status;
				if ((d = opendir(".")) == NULL) // Opens current directory else error
				{
					perror("Failed to open");
					continue;
				}
				if (d)
				{
					while ((dir = readdir(d)) != NULL) // Reads all in directory until end
					{
						printf("%s\n", dir->d_name);
					}
			
					closedir(d);
					continue;
				}
			}
		}



		fflush( stdin );
		fflush( stderr );
		if(-1 == (kidpid=fork())) // Fork is used to create a second process to handle execvp
		{
			perror("Cannot fork");
			exit(1);
		}
		if (0 == kidpid) {
			if (pipCtr > 0) // Piping
			{
				int fildes[pipCtr][2];
				pipe(fildes[0]); // initialize pipe
				
				// Nothing to be pipped to, unable to continue, missing arguments
				if (pipPtr[0] == NULL)
				{
					fprintf(stderr, "Nothing to pipe to\n");
					_exit(1);
				}
				
				// Unable to fork in order to handle piping
				if(-1 == (grandkid=fork())) //error for piping output of LHS is connected to input of RHS
				{
					perror("Cannot pipe");
					_exit(1);
				} 
				else if (0 == grandkid) // go into grandchild of child to initialize left side fork this is LS
				{
					for (i = 0; i <= pipCtr - 1; i++) {
						fflush( stdin );
						fflush( stderr );

						if( i != pipCtr - 1) pipe(fildes[i+1]);

						if (fork() == 0) {
							if (i == pipCtr - 1) //exit on the extra fork
							_exit(1);
						}
						else {
							if (i == pipCtr - 1) { // handle most left
								dup2(fildes[i][1], STDOUT_FILENO);
								close(fildes[i][0]);
								execvp(newargv[0], newargv);
								perror("execvp");
								_exit(1);
							}
							else { // handle middle
								dup2(fildes[i+1][0], STDIN_FILENO);
								dup2(fildes[i][1], STDOUT_FILENO);
								for (j = i; j <= i+1; j++){
									close(fildes[j][1]);
									close(fildes[j][0]);
								}
								execvp(pipPtr[pipCtr - 2 - i], newargv + pipLocation[pipCtr - 2 - i]+1);
								perror("execvp");
								_exit(1);
							}
						}
					}		
							// else {
							// 	printf("CMD: %s %d\n", newargv[0], i);
							// 	printf("Loc: %d \n", newargv + 
				}
				else // child this furthest right command
				{
					//Changes STDIN of Pipe to STDIN of File
					dup2(fildes[0][0], STDIN_FILENO);
					close(fildes[0][1]);
					execvp(pipPtr[pipCtr-1], newargv + pipLocation[pipCtr-1] + 1);
					perror("execvp");
					_exit(1);
				}
				continue;
			} // END PIPE
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
	}	// start kill
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
			w = w + length + 1; //next char after pipe
			pipPtr[pipCtr] = w; //address of next char after pipe
			pipCtr++;
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