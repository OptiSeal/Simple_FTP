/* File: token.h
 * Authors: Jarryd Kaczmarczyk & Daniel Dobson
 * Date: 19/10/2021
 * Purpose: Token header file
 *
 */

#define MAX_NUM_TOKENS  1000
#define tokenSeparators " \t\n"    // Characters that separate tokens

/* Tokenises a string  - Uses " \t\n" as characters that separate tokens
*	
*	Pre: line array must not be NULL, token pointer array must be provided by caller,
*		 tokenSeperators must be predefined
*	Post: At least 1 token returned in token pointer array
*	Return: (VOID)
*/
void tokenise (char line[], char *token[]);
