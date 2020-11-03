<!DOCTYPE html>
<html><head><meta charset="utf-8" /> <meta name="viewport" content="width=device-width, initial-scale=1.0" /><title>Inventory lookup</title></head>
<body>
<pre>
<?php
$REGEX_STIHL = "/\d{4}[- ]*\d{3}[- ]*\d{4}/";
$REGEX_STIHL_BAR = "/\d{4}[- ]*\d{4}/";
$SHPATH="/home/benji/find_part.sh";
function get_part($part) {
	$p=escapeshellarg($part);
	global $SHPATH;
	system("${SHPATH} ${p}");
}
$rq=$_REQUEST["part"];
//if(preg_match($REGEX_STIHL, $rq))
if(strlen($rq))
{
	$rq=strtoupper($rq);
	get_part($rq);
}
?>
</pre>
<form action="/" method="get">
<label for="part">Part number:</label>
<input type="text" name="part"></input>
</form>
</body>
</html>
