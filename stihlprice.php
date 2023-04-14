<?php

// Remove dashes and spaces and put them in the right place to match the DB
$part=$_REQUEST['part'];
$part_nosep = str_replace(array(' ','-'), '', $part);
if(preg_match('/^\d{11}$/', $part_nosep))
{
	// we've got a typical 11 char part number
	$a = substr($part_nosep, 0, 4);
	$b = substr($part_nosep, 4, 3);
	$c = substr($part_nosep, 7, 4);
	$part = $a . ' ' . $b . ' ' . $c;
}
else
{
	$part = str_replace('-', ' ', $part);
}

$db = new SQLite3('stihlparts.db');
$stm = $db->prepare('SELECT * FROM parts WHERE Item = ?');
$stm->bindParam(1, $part);
$res = $stm->execute();
$row = $res->fetchArray(SQLITE3_ASSOC);
print json_encode($row);
$stm->close();
$db->close();
?>
