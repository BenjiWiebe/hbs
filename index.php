<!DOCTYPE html>
<html><head><meta charset="utf-8" /> <meta name="viewport" content="width=device-width, initial-scale=1.0" /><link rel="stylesheet" href="index.css" /><title>Inventory lookup</title></head>
<body>
<p>Search by:
<a href="/?match=vendor">Vendor</a>
<a href="/">Part</a>
</p>
<form action="/" method="get" id="searchform">
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
	?><input type="checkbox" form="searchform" name="partialm" id="partialm" <?=$_REQUEST["partialm"] == 'on' ? "checked" : ""?>></input><label style="text-decoration: underline;user-select:none;padding:" for="partialm">Partial match</label><?php
}
	$all_checked = "";
	$normal_checked = "";
	$onhand_checked = "";
	$out_checked = "";
	switch($_REQUEST['stock']) {
		case 'onhand': $onhand_checked = ' checked'; break;
		case 'normalstock': $normal_checked = ' checked'; break;
		case 'out': $out_checked = ' checked'; break;
		case 'all':
		default:
			$all_checked = ' checked';
	}
?>
<input type="radio" form="searchform" name="stock" id="all" value="all"<?=$all_checked?>></input><label for="all">All parts</label>
<input type="radio" form="searchform" name="stock" id="normalstock" value="normalstock"<?=$normal_checked?>></input><label for="normalstock">Normally stocked</label>
<input type="radio" form="searchform" name="stock" id="out" value="out"<?=$out_checked?>></input><label for="out">Out of stock</label>
<input type="radio" form="searchform" name="stock" id="onhand" value="onhand"<?=$onhand_checked?>></input><label for="onhand">On hand</label>
<?php
print '<p></p>';
print '<label for="search">' . $label_text . '</label>';
$search_value = $_REQUEST['search'];
// on this input field, we preload it with whatever search term this page is a result of
// also, we select it so they can just type to replace it, however we only want to select it
//   right after page load before they choose a different search term, hence the onfocus=null
if($search_by == "vendor") print '<input type="search" id="search" name="search" value="' . $search_value . '" autofocus onfocus="this.select();this.onfocus=null"></input>';
if($search_by == "part") print '<input type="search" id="search" name="search" value="' . $search_value . '" autofocus onfocus="this.select();this.onfocus=null"></input>';
print '<input type="hidden" name="match" value="' . $search_by . '" />';
?>
</form>
<?php
$EXEPATH="/home/benji/hbs/invent";
$FILEPATH="/home/benji/INVENT";
require('text.php');
require('table.php');
function find_results($part, $search_by, $is_regex = false) {
	$p=escapeshellarg($part);
	global $EXEPATH;
	global $FILEPATH;
	if($is_regex)
	{
		$opt = '--match-regex-all';
	}
	else
	{
		$opt = '--match-partno';
	}
	if($search_by == "vendor")
	{
		$opt = '--match-vendor';
	}
	$cmd = "${EXEPATH} --print json ${opt} '${part}' ${FILEPATH}";
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
		// start the timer on the search
		$starttime = microtime(true);

		// check if its a regex match of plain match (starts with '/')
		// and get the results as an array
		if($rq[0] == '/' || $_REQUEST['partialm'] == 'on') {
			if($rq[0] == '/')
				$rq = substr($rq, 1);
			$rawresults = find_results($rq, $search_by, true);
		} else {
			$rawresults = find_results($rq, $search_by);
		}

		function check_stock_oh($i) {
			return $i->onhand > 0;
		}
		function check_stock_normal($i) {
			return $i->bin != 'SP ORD';
		}
		function check_stock_out($i) {
			if($i->bin == 'SP ORD') // special order stuff won't be stocked and that's fine
				return false;
			if($i->onhand > 0)
				return false;
			return true;
		}
		if($_REQUEST['stock'] == 'onhand') {
			$j = array_filter($rawresults, "check_stock_oh");
		} else if($_REQUEST['stock'] == 'normalstock') {
			$j = array_filter($rawresults, "check_stock_normal");
		} else if($_REQUEST['stock'] == 'out') {
			$j = array_filter($rawresults, "check_stock_out");
		} else {
			$j = $rawresults;
		}

		// calculate the time it took, and the number of results
		// then print that plus the results-as-a-table
		$endtime = microtime(true);
		$c = count($j);
		if($c) {
			if($c > 1)
				print '<a>Found ' . $c . ' results (' . number_format($endtime-$starttime, 2, '.', ',') . ' seconds)</a>';
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
