<!DOCTYPE html>
<html><head><meta charset="utf-8" /> <meta name="viewport" content="width=device-width, initial-scale=1.0" /><link rel="stylesheet" href="index.css" /><title>Inventory lookup</title></head>
<body>
<form action="/" method="get">
<label for="part">Part number:</label>
<input type="search" id="search" name="part" autofocus></input>
</form>
<?php
$EXEPATH="/home/benji/hbs/invent --search-desc --search-extdesc";
$FILEPATH="/home/benji/INVENT";
require('text.php');
require('table.php');
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
	if($j === null) {
		echo $txt;
		throw new DomainException("Invalid JSON result");
	}
	return $j;
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
			print_results_as_table($j);
		} else {
			echo "No results found.";
		}
	} catch(Exception $e) {
		echo "Part lookup failed: " . $e->getMessage();
	}
}

?>
</body>
</html>
