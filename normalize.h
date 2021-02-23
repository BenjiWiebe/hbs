void normalize_part_number(char *a)
{
	int i2 = 0;
	for(int i = 0; a[i] != 0; i++)
	{
		if(a[i] == '-' || a[i] == ' ')
			continue;
		a[i2++] = a[i];
	}
	a[i2] = 0;
}
