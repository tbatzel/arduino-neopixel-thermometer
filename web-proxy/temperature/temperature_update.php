<?php

require("../dbinfo.php");
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




$temperature = isset($_GET['temperature']) ? $_GET['temperature'] : '';
$temperature = filter_var($temperature, FILTER_SANITIZE_NUMBER_FLOAT, FILTER_FLAG_ALLOW_FRACTION);

$read = isset($_GET['read']) ? 1 : 0;




if($temperature) {
	// Create connection
	$conn = new mysqli($sql_servername, $sql_username, $sql_password, $sql_dbname);
	// Check connection
	if ($conn->connect_error) {
	    die("<error>Connection failed: " . $conn->connect_error);
	} 
	$sql = "INSERT INTO " . $sql_dbname . ".NeoPixelThermometer (temperature, logtime_utc)
			VALUES ('" . $temperature . "', UTC_TIMESTAMP())";
	if ($conn->query($sql) === TRUE) {
	    echo "<rows>" . $conn->affected_rows . "</rows>\n";
	} else {
	    echo "<error>" . $conn->error . "</error>\n";
	}
	$conn->close();

}







?>