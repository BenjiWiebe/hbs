#!/usr/bin/env bash
if ! test -r DealerPricing.csv; then
	echo Please put DealerPricing.csv in the current directory and re-run.
	exit 1;
fi

echo 'drop table if exists parts' | sqlite3 stihlparts.db
echo '.import --csv DealerPricing.csv parts' | sqlite3 stihlparts.db
