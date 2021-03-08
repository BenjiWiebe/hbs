<!DOCTYPE html>
<html><head><meta charset="utf-8" /> <meta name="viewport" content="width=device-width, initial-scale=1.0" /><title>Inventory lookup</title></head>
<body>
<form action="/" method="get">
<label for="part">Part number:</label>
<input type="text" name="part"></input>
</form>
<pre>
<?php
$SHPATH="/home/benji/find_part.sh";
function get_part($part) {
//	if($part[0] != '/' && strpos($part, '*') !== false) {
//		print "processed ${part} to ";
//		$part2 = '/' . substr_replace('*', '.*', $part);
//		print "${part2}\n";
//		exit;
//	}
	$p=escapeshellarg($part);
	global $SHPATH;
	system("${SHPATH} ${p}");
}
$rq=$_REQUEST["part"];
if(strlen($rq))
{
	get_part($rq);
}
?>
</pre>
</body>
</html>
