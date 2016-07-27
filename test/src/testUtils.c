/*
 testUtils.c

 Copyright (C) 2011 Belledonne Communications, Grenoble, France
 Author : Johan Pascal
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "typedef.h"

void printUsage(char *command)
{
	printf ("Usage:\n %s -p|<input file name>\n\n This executable request one argument either:\n  -p : display the computation type(fixed or floating) and exit\nor\n  <input file name> : process the input file and write the output in a file with the same prefix a .out extension\n\n",command);
	exit (-1);

}

int getArgument(int argc, char *argv[], char** filePrefix)
{
	/* We have only one argument wich can be either the input filename or -p which will answer either floating or fixed according */
	/* computation mode being fixed or floating point */ 
	if (argc != 2) {
		printUsage(argv[0]);
		exit (-1);
	}

	if (argv[1][0] == '-') {
		if (argv[1][1] == 'p') { /* -p switch, return fixed or floating and exit */
#ifdef FLOATING_POINT
			printf ("floating\n");
#else /* ifdef FLOATING_POINT */
			printf ("fixed\n");
#endif /* ifdef FLOATING_POINT */
			exit(0);
		} else { /* unknow switch, display usage and exit */
			printUsage(argv[0]);
			exit (-1);
		}
	} else { /* argument is the input file */
		/* get the input file prefix */
  		int i = strlen(argv[1])-1;
		int pos = 0;
		while (pos==0) {
			if (argv[1][i]=='.') {
			pos = i;
			}
			i--;
			if (i==0) { 
				printf("%s - Error input file  %s doesn't contain any ., impossible to extract prefix\n", argv[0], argv[1]);
				exit(-1);
			}
  		}
		*filePrefix = malloc((pos+3)*sizeof(char));
		strncpy(*filePrefix, argv[1], pos);
		(*filePrefix)[pos]='\0';
	}

	return 0;
}

int getArgumentsMultiChannel(int argc, char *argv[], char *filePrefix[])
{
	while (argc>1) { /* loop over all the argument which shall be input file names */
		argc--;
		/* get the input file prefix */
  		int i = strlen(argv[argc])-1;
		int pos = 0;
		while (pos==0) {
			if (argv[argc][i]=='.') {
			pos = i;
			}
			i--;
			if (i==0) { 
				printf("%s - Error input file  %s doesn't contain any ., impossible to extract prefix\n", argv[0], argv[argc]);
				exit(-1);
			}
  		}
		
		filePrefix[argc-1] = malloc((pos+3)*sizeof(char));

		strncpy(filePrefix[argc-1], argv[argc], pos);
		filePrefix[argc-1][pos]='\0';
	}

	return 0;
}
