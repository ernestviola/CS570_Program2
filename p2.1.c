/* 
Ernest Viola
Professor John Carroll
CS570
Program 2
October 11, 2017
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
char* newargv[MAXITEM];
int i, length, ctr, amp, inCount, outCount, pip, pipLocation, fileIn, fileOut, errCode;
char* output;
char* input;
char* pipRight;

//===================START FUNCTIONS===================//


//===================START SIGNAL HANDLER FUNCTION===================//
// Handles signal for sigterm at kill
void myhandler(int signum)
{
}
//====END FUNCTION===//

//===================START PRINT PROGRAM FUNCTION===================//
void printP()
{
	printf("p2: ");
}
//====END FUNCTION===//


//===================START MAIN===================//


main()
{	
	int kidpid, grandkid;
	char* home = getenv("HOME");
	char* cd = {"cd"};
	char* lsF = {"ls-F"};
	
	
	setpgid(0,getpid()); // pid of current process for terminal
	signal(SIGTERM, myhandler); // Is used to send the termination request
	
	for(;;)
	{
		i = length = ctr = amp = inCount = outCount = pip = pipLocation = fileIn = fileOut = errCode = 0;
		printP(); // function to print prompt

		if (parse() == -1) //EOF exit condition
		{
			break;
		}
		
		if (length == -2) // Special error message for unclosed parenthesis
		{
			fprintf(stderr, "No closing parenthesis\n");
			continue;
		}
		if ((inCount == 1 || outCount == 1) && ctr <= 2) // Checks to see if enough arguments for redirection
		{
			fprintf(stderr,"Redirect: Not enough arguments\n");
			continue;
		}
		if (errCode != 0) //Handles all errors in redirection and parsing from parse() function
		{
			switch(errCode)
			{
				case 1:
					fprintf(stderr,"Redirect: > is invalid\n");
					continue;
				case 2:
					fprintf(stderr,"Redirect: < is invalid\n");
					continue;
				case 3:
					fprintf(stderr,"Pipe: | is invalid\n");
					continue;
				case 4:
					fprintf(stderr,"Background: & is invalid\n");
					continue;
				case 5:
					fprintf(stderr,"Syntax: invalid\n");
					continue;
			}
		}
		
		if (ctr == 1) // Empty line or new line condition: starts over
		{
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
		
		if (ctr >= 3) //ls-F more than one directory
		{
			if (strcmp(newargv[0],lsF) == 0)
			{
				i = 0;
				struct stat sb;
				DIR* d;
				struct stat statbuf;
				struct dirent* dir;
				int status;
				if ((d = opendir(newargv[1])) != NULL)
				{
					while ((dir = readdir(d)) != NULL)
					{
						if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) 
						continue;
					
						printf("%s\n", dir->d_name);
					}
					closedir(d);
					continue;
				}
				else // uses stat in order to check if file instead of directory
				{
					if (stat(newargv[1],&sb) == -1)
					{
					}
					int statfile = sb.st_mode & S_IFMT; // Code to see if regular file instead
					if (statfile == S_IFREG)
					{
						printf("%s\n", newargv[1] );
						continue;
					}
					perror("ls-F");
					continue;
				}
			}
		}
		
		// start cd
		if (ctr == 2) // CD for cd by itself sends to HOME of user
		{
			if (strcmp(newargv[0],cd) == 0) //Checks to see if CD is called
			{
				if (chdir(home) == -1) // Unable to open HOME
				{
					perror("Directory not found");
					continue;
				}
				continue;
			}
		}
		if (ctr >= 3) // CD with arguments
		{
			if (strcmp(newargv[0],cd) == 0) //Checks to see if CD is called
			{
				if (chdir(newargv[1]) == -1) // Invalid Directory
				{
					perror("Directory not found");
					continue;
				}
				if (ctr >= 4) 
				{
					fprintf(stderr, "Too many arguments\n");
				}
				continue;
			}
		}
		// end cd

		fflush( stdin );
		fflush( stderr );
		// forking using & and execvp
		if(-1 == (kidpid=fork())) // Fork is used to create a second process to handle execvp
		{
			perror("Cannot fork");
			exit(1);
		} 
		else if (0 == kidpid) // child
		{
			
			if (pip == 1) // Piping
			{
				int fildes[2];
				pipe(fildes); // initialize pipe
				
				// Nothing to be pipped to, unable to continue, missing arguments
				if (pipRight == NULL)
				{
					fprintf(stderr, "Cannot pipe\n");
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
					//Changes STDOUT of Pipe to STDOUT of File
					dup2(fildes[1], STDOUT_FILENO);
					close(fildes[0]);
					execvp(newargv[0], newargv);
					perror("execvp");
					_exit(1);

				}
				else // child this is TR
				{
					//Changes STDIN of Pipe to STDIN of File
					dup2(fildes[0], STDIN_FILENO);
					close(fildes[1]);
					execvp(pipRight, newargv + pipLocation + 1);
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
		else// parent checks for '&' if there is then skip and let child run @ same time 
		{
			if (amp != 1) //Waits for child unless bg process is called
			while (wait(NULL) != kidpid);
			if (amp == 1) // If bg process then print cmd and pid
			{
				printf("%s [%d]\n",newargv[0], kidpid );
			}
		}		// end fork
	}
	// start kill
	killpg(getpgrp(), SIGTERM);
	printf("p2 terminated.\n");
	exit(0);
}

//===================START PARSE FUNCTION===================//
int parse()
{
	char* w = s;
	outCount = inCount = 0;
	output = NULL;
	input = NULL;
	int meta = 0;
	for(;;) 
	{
		// printf("Start get word\n");
		length = getword(w);
		// printf("%s\n",w );
		if (length == -2)
		{
			break;
		}
		// printf("%s %d \n", w, length);
		if (meta == 1) // Checks for invalid syntax when a metacharacter is previously called
		{
			if (*w == '>' || *w == '<' || *w == '|' || *w == '&')
			{
				errCode = 5;
				meta --;
			}
		}
		else if (*w == '|' && length == 1) // stdout
		{
			pipLocation = ctr;
			newargv[ctr] = '\0'; // sets place where pipe is supposed to be to null
			ctr++;
			pip++;
			w = w + length + 1; //next metacharacter
			pipRight = w; // gets address of next word
		}
		else // if not a metacharacter
		{
			newargv[ctr] = w;
			w = w + length + 1; // pointer to beginning of next word
			ctr++; // ctr for each cell in newargv
		}
		
		if ( length <= 0 ) break; // if \n, EOF, or unclosed parenthesis breaks to get handled
	} // end for
	
	
	newargv[ctr - 1] = '\0'; // Finishes newargv with null pointer for execvp
	return length;
}
//====END FUNCTION===//

