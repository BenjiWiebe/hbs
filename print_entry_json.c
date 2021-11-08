#include <stdio.h>
#include <math.h>
#include "print_entry_json.h"
#include "json_stringify.h"

void print_double_two(double d)
{
	if(d == 0.0 || d == 1.0 || d == 2.0 || d == 3.0 || d == 4.0 ||
		d == 5.0 || d == 6.0 || d == 7.0 || d == 8.0 || d == 9.0)
	{
		putchar('0' + (int)d);
		return;
	}
	double intpart, fractpart;
	fractpart = modf(d, &intpart);
	if(fractpart == 0.0)
	{
		printf("%d", (int)intpart);
	}
/*	if(d == 0.0)
		putchar('0');
	else if(d == 1.0)
		putchar('1');
	else if(d == 2.0)
		putchar('2');
	else if(d == 3.0)
		putchar('3');*/
//	if(d - (int)d == 0)
//		printf("%d", (int)d);
	else
		printf("%0.2f", d);
}

void print_entry_json(struct invent_entry *entry, bool print_history)
{
#define put(s) fputs((s),stdout)
	put("{");
	put("\"partnumber\":"); put(json_stringify(entry->part_number));
	printf(",\"price\":%d", (int)entry->price);
	put(",\"onhand\":"); print_double_two(entry->on_hand);
	put(",\"bin\":"); put(json_stringify(entry->bin_location));
	put(",\"binalt1\":"); put(json_stringify(entry->bin_alt1));
	put(",\"binalt2\":"); put(json_stringify(entry->bin_alt2));
	put(",\"desc\":"); put(json_stringify(entry->desc));
	put(",\"extdesc\":"); put(json_stringify(entry->ext_desc));
	put(",\"note1\":"); put(json_stringify(entry->note1));
	put(",\"note2\":"); put(json_stringify(entry->note2));
	if(print_history)
	{
		put(",\"histmonth\":[");
		for(int i = 0; i < MONTH_HIST_LEN - 1; i++) // Subtract 1 so we have an element left...
		{
			print_double_two(entry->month_history[i].value);
			putchar(',');
		}
		print_double_two(entry->month_history[MONTH_HIST_LEN-1].value);
		putchar(']'); // ...which we print without a comma
		put(",\"histyear\":[");
		for(int i = 0; i < YEAR_HIST_LEN - 1; i++)
		{
			print_double_two(entry->year_history[i].value);
			putchar(',');
		}
		print_double_two(entry->year_history[YEAR_HIST_LEN-1].value);
		putchar(']');
	}
	put("}\n");
#undef put
}


