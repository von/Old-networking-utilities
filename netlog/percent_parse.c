/***************************************************************************
 *
 *	percent_parse.c
 *
 *	Parse a string for percent characters and replace them with strings.
 *
 **************************************************************************/

#include <stdio.h>
#include <string.h>

/*
	replace_string is set by the calling program to point at a string
	that will replace the current percent character.
*/
char *replace_percent;

/*
	position is a place marker, holding our current position in the
	parsed string.

	current_parsing holds a pointer to the string we currently parsing
	so we know if it gets changed on us.
*/
static char *position, *current_parsing = NULL;



percent_parse(string, max_len)
	char *string;
	int max_len;
{
	int percent_char;

	if (string == NULL)
		return 0;

	if (current_parsing != string)
		/* New String */
		position = current_parsing = string;

	else {	/* Replace current percent char with string */
		int len;
		char *string_end;

		len = strlen(string) + strlen(replace_percent) - 2;

		if (len > max_len) 	/* TOO LONG */
			return -1;

		/* Save end of string */
		string_end = strdup(position + 2);

		/* Put in replacement string */
		strcpy(position, replace_percent);

		/* Advance position to end of replacement string */
		position += strlen(replace_percent);

		/* And put ending back on */
		strcat(position, string_end);

		free(string_end);
	}

	while (*position != '%' && *position != NULL)
		 position++;

	if (*position == '%')
		percent_char = *(position + 1);
	else
		percent_char = 0;

	return percent_char;
}
