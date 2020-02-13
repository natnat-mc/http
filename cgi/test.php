<!DOCTYPE html>
<html>
	<head>
		<meta charset="UTF-8" />
		<title>Test PHP</title>
	</head>
	<body>
		<ul>
			<?php for($i=0; $i<10; $i++) { ?>
				<li><?= $i ?></li>
			<?php } ?>
		</ul>
		<pre><?php var_dump($_GET); ?></pre>
	</body>
</html>
