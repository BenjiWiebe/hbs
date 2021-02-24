#include <stdio.h>
#include <stdbool.h>
bool glob_match(char*,char*);
int main()
{
	char *a = {"hello world"};
	char *glob = {"hello*"};
	if(glob_match(a, glob))
		printf("Matched!\n");
	else
		printf("No match.\n");
	return 0;
}

bool glob_match(char *haystack, char *needle)
{
}
