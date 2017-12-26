/* 
Ernest Viola
Professor John Carroll
CS570
September 18, 2017
*/

#include <stdio.h>
#include <string.h>


int getword(char *w)
{
	int iochar;
	int counter = 0; //flag for counter
	int backslash = 0; //flag for backslash
	int apostrophe = 0; //flag for apostrophe
	int greater = 0; //flag for greater
	
	while ( ( iochar = getchar() ) != EOF)
	{
		if (counter == 254)
		{
			w[counter] = '\0';
			//pushes extra character back to stdin
			//since character that doesn't belong to current string needs to be
			//placed back onto stdin to be read into next string
			ungetc(iochar, stdin); 
			break;
		}
		//checks backslash flag
		if (backslash == 1)
		{
			// '\n' ends a backslash
			if (iochar == '\n')
			{
				ungetc(iochar,stdin);
				break;
			}
			//handles special case where backslash is found in apostrophe string
			else if (apostrophe == 1 && iochar != '\'')
			{
				backslash = 0;
				w[counter] = '\\';
				counter++;
				w[counter] = iochar;
				counter++;
				continue;
			}
			backslash = 0;
			w[counter] = iochar;
			counter++;
			continue;
		}
		
		if (apostrophe == 1)
		{
			if (iochar == '\n' || iochar == ';')
			{
				// ungetc(iochar,stdin);
				break;
			}
			
			// sends to backslash to handle
			if (iochar == '\\')
			{
				backslash = 1;
				continue;
			}
			
			// handles all other characters
			if (iochar == '\'')
			{
				apostrophe = 0;
				continue;
			}
			else
			{
				w[counter] = iochar;
				counter++;
				continue;
			}
		}
		
		//=======================FLAG SETTERS===================//
		//sets backslash flag
		if (iochar == '\\')
		{
			backslash = 1;
			continue;
		}
		
		// sets apostrophe flag
		if (iochar == '\'')
		{
			apostrophe = 1;
			continue;
		}
		
		//=====================STORAGE HANDLER=================//
		w[counter] = iochar;
		
		//checks to see if \t or \32 at beginning of line, if so then skip
		if (w[0] == '\t' || w[0] == ' ')
		{
			continue;
		}
		
		//checks for metacharacters '<','|',\&
		if ((w[0] == '<'  || w[0] == '|' || w[0] == '&' ) && counter == 0)
		{
			counter++;
			break;
		}
		
		//handles '>!' and '>'
		if (greater == 1)
		{
			greater = 0;
			if (w[1] == '!')
			{
				w[counter+1] = '\0';
				return 2;
			}
			ungetc(iochar,stdin);
			w[counter] = '\0';
			return counter;
		}
		
		// if (iochar == '\n')
		// {
		// 	printf("GOT IT Counter: %d backslash: %d apostrophe: %d greater: %d err: %d character: %c\n", counter, backslash,apostrophe,greater,err, iochar );
		// }
		
		//sets the greater flag, and sends to greater to check for '!'
		if (w[0] == '>' && counter == 0)
		{
			greater = 1;
			counter++;
			continue;
		}
		
		//checks for metacharacter '\''
		if (w[0] == '\'')
		{
			apostrophe = 1;
			continue;
		}
		
		// if found newline at beginning then return [] and 0
		if (w[0] == '\n' || w[0] == ';')
		{
			w[counter] = '\0';
			return 0;
		}
		
		//breaks at all tabs and spaces
		if (iochar == '\t' || iochar == ' ')
		{
			w[counter] = '\0';
			break;
		}
		
		//handles non sepcial metacharacters
		if (iochar == '<' || iochar == '|' || iochar == '&' )
		{
			ungetc(iochar,stdin); //puts iochar back onto stack to be handled
			w[counter] = '\0';
			return counter;
		}
		
		// checks to see if newline or special metacharacters, if so then return \n to stdin using ungetc and end current word
		if (iochar == '\n' || iochar == ';' || iochar == '>')
		{
			w[counter] = '\0';
			ungetc(iochar,stdin); //puts iochar back onto stack to be handled
			break;
		}
		counter++;
	} // end while
	
	if (apostrophe == 1)
	{
		w[counter] = '\0';
		return -2;
	}
	
	// handles EOF correct returns
	if (counter > 0)
	{
		w[counter] = '\0';
		return counter;
	}
	// output for non metacharacter, EOF, \t, and spaces
	else 
	{
		w[counter] = '\0';
		return -1;
	}
}
