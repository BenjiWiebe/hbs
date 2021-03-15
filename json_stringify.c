#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

static size_t len = 0;
static char *out = NULL;

const char hexbytes[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
// Takes input and converts it to a JSON string.
// pointer should be free()'d when done with it.
// If the input is a NULL pointer or 0-length string,
// it will be encoded as "null".
// If malloc() fails, this function returns NULL.
char *json_stringify(char *input)
{
	if(input == NULL || input[0]==0)
	{
		const char nulltxt[] = {"\"null\""};
		len = sizeof(nulltxt)+1;
		out = malloc(len);
		if(!out)
			return NULL;
	}
	if(out == NULL)
	{
		// * 6 + 3 gives us space to encode everything as \uXXXX,
		// plus space for the terminating NULL and quotation marks.
		// it prevents us from
		// having to do lots of fancy realloc()s part way through.
		len = strlen(input) * 6 + 3; // Overkill, but simplifies things
		out = malloc(len);
		if(!out)
			return NULL;
	}
	size_t inlen = strlen(input);
	if(inlen > len)
	{
		len = inlen * 6 * 3;
		free(out);
		out = malloc(len);
		if(!out)
			return NULL;
	}

	size_t outi = 0;
	out[outi++] = '"';
	for(size_t i = 0; input[i] != 0; i++)
	{
		if(input[i] == '\'' || input[i] == '"')
		{
			out[outi++] = '\\';
			out[outi++] = input[i];
		}
		else if(isprint(input[i]))
		{
			out[outi++] = input[i];
		}
		else
		{
			out[outi++] = '\\';
			out[outi++] = 'u';
			out[outi++] = '0';
			out[outi++] = '0';
			out[outi++] = hexbytes[(input[i] & 0xF0) >> 4];
			out[outi++] = hexbytes[(input[i] & 0xF)];
		}
	}
	out[outi++] = '"';
	out[outi++] = 0;
	return out;
}

void json_stringify_free(void)
{
	if(out)
	{
		free(out);
		out = NULL;
	}
}
