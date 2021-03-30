<?php
function cell($d) {
	print '<td>' . htmlspecialchars($d) . '</td>';
}
function print_results_as_table($results) {
?><table><tbody><tr><th>Part #</th><th>Bin</th><th>Price</th><th>On hand</th><th>Desc</th><th></th></tr>
<?php
	foreach($results as $result) {
		print '<tr>';
		cell($result->partnumber);
		cell($result->bin);
		$dollar = intdiv($result->price, 100);
		$cents = sprintf("%02d", $result->price % 100);
		cell('$' . $dollar . '.' . $cents);
		cell($result->onhand);
		cell($result->desc);
		cell($result->extdesc);
		print '</tr>';
	}
	echo '</tbody></table>';
}
?>
