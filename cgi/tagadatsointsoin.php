<!DOCTYPE html>
<html>
	<head>
		<meta charset="UTF-8" />
		<title>Test PHP</title>
	</head>
	<body>
		<pre>
<?php
	$tagada="TAGADA TSOIN TSOIN";
	if(isset($_GET['text'])) $tagada=$_GET['text'];
	$len=strlen($tagada);
	$orig=$len;
	$off=0;
	while($len>0) {
		echo str_repeat(' ', $off);
		echo substr($tagada, $off, $len);
		echo "\n";
		$off++;
		$len-=2;
	}
?>
		</pre>
	</body>
</html>
