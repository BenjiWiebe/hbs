<?php
$EXEPATH="/home/benji/hbs/invent";
//$EXEPATH="/home/benji/invent";
$FILEPATH="/home/benji/INVENT";
$p=escapeshellarg($_REQUEST['part']);
$cmd = "${EXEPATH} -p json --match-partno ${p} ${FILEPATH}";
print(shell_exec($cmd));
?>
