<html>
  <head>
    <title>Lumberyard Distribution Portal</title>
    <link rel="shortcut icon" href="images/Favicon.ico">
    <link rel="stylesheet" id="default_stylesheet-css" href="css/style.css" type="text/css" media="all">
  </head>
  <body>
  
<?php
  $bucket = htmlspecialchars($_GET["bucket"]);
  $expiry = htmlspecialchars($_GET["expiry"]);
  $unlockpass = htmlspecialchars($_GET["password"]);
  $unlockuser = htmlspecialchars($_GET["user"]);

  $database_host = "localhost";
  $user_name = "lumbery";
  $password = "Builder99";
  $database_name = "lybuilds";

  mysql_connect($database_host, $user_name, $password) or die(mysql_error());
  mysql_select_db($database_name) or die(mysql_error());

  $pass_result = mysql_query("SELECT password FROM users WHERE username=\"" . $unlockuser . "\";");
  $server_pass = mysql_fetch_array($pass_result);

  if (md5($unlockpass) != $server_pass['password'])
  {
    exit("Invalid Password");
  }

  $command = escapeshellcmd("/scripts/get_signed.py --bucket=" . $bucket . " --expiry=" . $expiry);
  $output = shell_exec($command);
  $result = json_decode($output, true);
  
  echo "    <table style=\"width: 1280px; margin: 0px auto;\">\n";
  echo "      <tr>\n";
  echo "        <td>\n";
  echo "          <div><p class=\"title_font\">Lumberyard Secure File Distribution</p></div>\n";
  echo "        </td>\n";
  echo "      </tr>\n";
  echo "      <tr>\n";
  echo "        <td>\n";
  echo "          <div><p class=\"sidebar_text\">Bucket: " . $bucket . "</p></div>\n";
  echo "          <div><p class=\"sidebar_text\">Link will expire on: " . date('Y:m:d H:i:s', time() + $expiry) . "</p></div>\n";
  echo "        </td>\n";
  echo "      </tr>\n";
  echo "      <tr><td><div><p class=\"sidebar_text\"> </p></div></td></tr>\n";
  echo "      <tr><td><div><p class=\"sidebar_text\">Available Files:</p></div></td></tr>\n";
  echo "      <tr>\n";
  echo "        <td class=\"sidebar_column\">\n";
  foreach ($result as $key => $jsons) {
    echo "          <div class=\"sidebar_entry\"><a href=\"" . $result[$key] . "\"><p class=\"sidebar_text\">" . $key . "</p></a></div>\n";
  }
?>
        </td>
      </tr>
    </table>
  </body>
</html>
