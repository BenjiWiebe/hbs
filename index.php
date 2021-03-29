<!DOCTYPE html>
<html><head><meta charset="utf-8" /> <meta name="viewport" content="width=device-width, initial-scale=1.0" /><link rel="stylesheet" href="index.css" /><title>Inventory lookup</title></head>
<body>
<p>Search by:
<a href="/?match=vendor">Vendor</a>
<a href="/">Part</a>
<form action="/" method="get">
<?php
if(isset($_REQUEST["match"]) && $_REQUEST["match"] == "vendor")
{
	$input_name = "search";
	$label_text = "Vendor number: ";
	$search_by = "vendor";
}
else
{
	$input_name = "search";
	$label_text = "Part number: ";
	$search_by = "part";
}
print '<label for="search">' . $label_text . '</label>';
print '<input type="search" id="search" name="search" autofocus></input>';
print '<input type="hidden" name="match" value="' . $search_by . '" />';
?>
</form>
<?php
$EXEPATH="/home/benji/invent --search-desc --search-extdesc";
$FILEPATH="/home/benji/INVENT";
require('text.php');
require('table.php');
function find_results($part, $search_by, $is_regex = false) {
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
	if($search_by == "vendor")
	{
		$opt = '--match-vendor';
	}
	$cmd = "${EXEPATH} --json ${opt} '${part}' ${FILEPATH}";
	//print $cmd;
	$txt = shell_exec($cmd);
	$j = json_decode($txt);
	if($j === null) {
		echo $txt;
		throw new DomainException("Invalid JSON result");
	}
	usort($j, fn($a,$b) => strcmp($a->partnumber, $b->partnumber));
	return $j;
}

isset($_REQUEST["search"]) && $rq=$_REQUEST["search"];

if(strlen($rq))
{
	try {
		if($rq[0] == '/') {
			$rq = substr($rq, 1);
			$j = find_results($rq, $search_by, true);
		} else {
			$j = find_results($rq, $search_by);
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
