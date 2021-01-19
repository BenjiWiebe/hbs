#define PCRE2_CODE_UNIT_WIDTH	8
#include <pcre2.h>
#include <stdio.h>
int main(int argc, char **argv)
{
	PCRE2_SPTR pattern = (PCRE2_SPTR)argv[2];
	int errorcode = 0;
	PCRE2_SIZE erroroffset = 0;
	pcre2_code *compiledre = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, PCRE2_CASELESS, &errorcode, &erroroffset, NULL);
	if(compiledre == NULL) {
		PCRE2_UCHAR buffer[256];
		pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
		printf("Regex error at offset %d: %s\n", (int)erroroffset, buffer);
		return 1;
	}
	char *text = argv[1];
	pcre2_match_data *matchdata;
	matchdata = pcre2_match_data_create_from_pattern(compiledre, NULL);
	int ret = 0;
	ret = pcre2_match(compiledre, (PCRE2_SPTR)text, PCRE2_ZERO_TERMINATED, 0, 0, matchdata, NULL);
	if(ret > 0)
	{
		printf("Matched!\n");
	}
	else if(ret == PCRE2_ERROR_NOMATCH)
	{
		printf("No match\n");
	}
	else
	{
		printf("Unknown matching error %d\n", ret);
	}
	pcre2_code_free(compiledre);
	return 0;
}
