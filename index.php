<!DOCTYPE html>
<html><head><meta charset="utf-8" /> <meta name="viewport" content="width=device-width, initial-scale=1.0" /><title>Inventory lookup</title></head>
<body>
<form action="/json.php" method="get">
<label for="part">Part number:</label>
<input type="text" name="part"></input>
</form>
<pre>
<?php
$EXEPATH="/home/benji/hbs/invent";
$FILEPATH="/home/benji/INVENT";
function find_results($part, $is_regex = false) {
	$p=escapeshellarg($part);
	global $EXEPATH;
	global $FILEPATH;
	if($is_regex)
	{
		$opt = '--regex';
	}
	else
	{
		$opt = '--find';
	}
	$cmd = "${EXEPATH} --json ${opt} '${part}' ${FILEPATH}";
	$txt = shell_exec($cmd);
	$j = json_decode($txt);
	if($j == null) {
		echo $txt;
		throw new DomainException("Invalid JSON result");
	}
	return $j;
}
function print_results_as_text($results) {
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
		if(count($entry->desc) == 0)
			print $result->desc . "\n";
		if(count($entry->extdesc) == 0)
			print $result->extdesc . "\n";
		print "\n";
	}
}
$rq=$_REQUEST["part"];
if(strlen($rq))
{
	try {
		if($rq[0] == '/') {
			$rq = substr($rq, 1);
			$j = find_results($rq, true);
		} else {
			$j = find_results($rq);
		}
		if(count($j)) {
			print_results_as_text($j);
		} else {
			echo "No results found.";
		}
	} catch(Exception $e) {
		echo "Part lookup failed: " . $e->getMessage();
	}
}

?>
</pre>
</body>
</html>
