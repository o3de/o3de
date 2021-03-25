<html>
  <head>
    <title>Lumberyard Distribution Portal</title>
    <link rel="shortcut icon" href="images/Favicon.ico">
    <link rel="stylesheet" id="default_stylesheet-css" href="css/fetch.css" type="text/css" media="all">
  </head>
  <body>
<?php

  $username = $_POST['login'];
  $password = $_POST['password'];

  $config = include('config.php');

  $connection = mysql_connect($config['sql_host'], $config['sql_username'], $config['sql_pass']);
  if (!$connection) {
    die(mysql_error());
  }
  mysql_select_db($config['sql_database_name']) or die(mysql_error());

  $pass_result = mysql_query("SELECT password FROM users WHERE username=\"" . $username . "\";");
  $server_pass = mysql_fetch_array($pass_result);

  if (md5($password) != $server_pass['password']) {
    mysql_close($connection);
    exit("Invalid credentials.");
  }


  $command = escapeshellcmd("/scripts/get_signed.py --bucket=" . $config['bucket'] . " --expiry=" . $config['expiry']);
  $output = shell_exec($command);
  $result = json_decode($output, true);

  echo "<div id='content'>\n";
  echo "  <h2>Lumberyard Secure File Distribution</h2>\n";
  echo "  <div id='nicelist'>\n";
  echo "    <ul>\n";
  foreach ($result as $key => $jsons) {
    $parts = explode('/', rtrim($key, '/'));
    echo "    <li><a href='" . $result[$key]  . "'>" . $parts[3] . "</a> </li>\n";
  }
  echo "    </ul>\n";
  echo "  </div>\n";
  echo "</div>\n";

  mysql_close($connection);
?>
  </body>
</html>
