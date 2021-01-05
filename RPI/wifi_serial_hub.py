import os
import time
import board
import busio
import digitalio
from digitalio import DigitalInOut, Direction, Pull
import serial

import requests
import json
import datetime


from mySecrets import config_q, config_appid



def getHttpData():
    url = "https://api.openweathermap.org/data/2.5/weather?q=" + config_q + "&appid=" + config_appid
    req = requests.get(url)
    try:
        jsonData = req.json()
        dataStore["K"] = jsonData["main"]["temp"]
        dataStore["C"] = round(dataStore["K"]-273.15,2)
        dataStore["F"] = round(dataStore["C"]*(9/5) + 32,2)

    except Exception as e:
        print("Exception loading response data to JSON: ", e)
        return
    print(dataStore)


#Setup - Digital Pin for Flag Notify
flagNotify = digitalio.DigitalInOut(board.D18)
flagNotify.direction = digitalio.Direction.OUTPUT
flagNotify.value = False

# Setup - Data Dictionary
dataStore = dict()
dataStore["K"] = 0;

# Setup - Serial Port
ser = serial.Serial('/dev/ttyS0')
ser.baudrate = 115200
ser.timeout = 0.5

# Setup - Web Get Interval
updateInterval = 90
updateLast = 0



while True:

    #Get new data from server based on timer
    if time.monotonic() - updateLast > updateInterval:
        getHttpData()
        updateLast = time.monotonic()


    #Update the digital pin output
    #updateFlag()


    # Check for new serial data
    try:
        line = ser.readline().decode("utf-8")
    except Exception as e:
        print("Exception: ", e)

    # Do something if new serial data
    if len(line) >= 1:
        if line[-1] == "\n":
            line = line[:-1]
            if(line == "GET_TEMP"):
                try:
                    #ser.write(bytes(json.dumps(dataStore["F"]), "utf-8"))
                    ser.write(bytes("206480:" + json.dumps(dataStore["F"]), "utf-8")) # Hex RGB


                except Exception as e:
                    print("Exception: ", e)
            else:
                ser.write(bytes("?", "utf-8"))
            ser.write(bytes("\n", "utf-8"))






