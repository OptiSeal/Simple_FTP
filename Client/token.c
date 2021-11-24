/* File: token.c
 * Authors: Jarryd Kaczmarczyk & Daniel Dobson
 * Date: 19/10/2021
 * Purpose: Tokenise a string
 *
 */

#include <stdio.h>
#include <string.h>
#include "token.h"

/* Tokenises a string  - Uses " \t\n" as characters that separate tokens
*	
*	Pre: line array must not be NULL, token pointer array must be provided by caller,
*		 tokenSeperators must be predefined
*	Post: At least 1 token returned in token pointer array
*	Return: (VOID)
*/
void tokenise (char line[], char *token[]){
    char *tk;
    int i = 0;
	
	// Tokenise first argument
    tk = strtok(line, tokenSeparators);
    token[i] = tk;
	
	// Tokenise other arguments until no other arguments exist
    while (tk != NULL) {
		i++;
		
		//if MAX_NUM_TOKENS reached, break and return
        if (i >= MAX_NUM_TOKENS) {
            i = -1;
            break;
        }
		
		// Tokenise argument
        tk = strtok(NULL, tokenSeparators);
        token[i] = tk;
    }

    return;
}
