<!DOCTYPE html>
<html>
<head>
</head>
<body>




<?php
require("dbinfo.php");
/*
	// Example dbinfo.php
	$servername = "localhost";
	$username = "arduino_user";
	$password = "MyPassword";
	$dbname = "arduino_db";
*/

	
date_default_timezone_set('America/Los_Angeles');


function utc_to_local($utc_datetime, $time_zone) 
{ 
	$format_string = "Y-m-d H:i:s";
	$date = new DateTime($utc_datetime, new DateTimeZone('UTC')); 
	$date->setTimeZone(new DateTimeZone($time_zone)); 
	return $date->format($format_string); 
} 



$read = isset($_GET['read']) ? 1 : 0;




if($read) {
	// Create connection
	$conn = new mysqli($sql_servername, $sql_username, $sql_password, $sql_dbname);
	// Check connection
	if ($conn->connect_error) {
	    die("Connection failed: " . $conn->connect_error);
	} 
	$sql = "SELECT id, logtime_utc, temperature FROM  " . $sql_dbname . ".NeoPixelThermometer ORDER BY logtime_utc DESC LIMIT 0 , 50";
	$result = $conn->query($sql);

	if ($result->num_rows > 0) {
		echo "<table border=1>\n";
	    while($row = $result->fetch_assoc()) {
	        echo "<tr>";
	        echo "<td>" . $row["id"]. "</td>";
	        echo "<td>" . utc_to_local($row["logtime_utc"],'America/Los_Angeles') . "</td>";
	        echo "<td>" . $row["temperature"]. "</td>";
	        echo "</tr>\n";
	    }
		echo "</table>\n";	    
	} 
	else {
		echo "No Results";
	}

	$conn->close();

}
else {
	// Create connection
	$conn = new mysqli($sql_servername, $sql_username, $sql_password, $sql_dbname);
	// Check connection
	if ($conn->connect_error) {
	    die("Connection failed: " . $conn->connect_error);
	} 
	$sql = "SELECT logtime_utc, temperature FROM  " . $sql_dbname . ".NeoPixelThermometer ORDER BY logtime_utc DESC LIMIT 0 , 1";
	$result = $conn->query($sql);

	if ($result->num_rows > 0) {
	    while($row = $result->fetch_assoc()) {
	        
	        echo "<p><strong>" . $row["temperature"]. "</strong></p>";

	        echo "<p>" . utc_to_local($row["logtime_utc"],'America/Los_Angeles') . "</p>";


	    } 
	} 
	else {
		echo "No Results";
	}

	$conn->close();

}



?>

</body>
</html>