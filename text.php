<?php
function print_results_as_text($results) {
	print "<pre>";
	foreach($results as $result) {
		print $result->partnumber;
		print ": ";
		if($result->bin) {
			print $result->bin;
			print ", ";
		}
		print "$";
		print intdiv($result->price, 100) . '.';
		printf("%02d", $result->price % 100);
		print " - " . $result->onhand . " on hand";
		print "\n";
		if(!empty($result->desc))
			print $result->desc . "\n";
		if(!empty($result->extdesc))
			print $result->extdesc . "\n";
		print "\n";
	}
	print "</pre>";
}
?>
