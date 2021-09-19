# Rocket Test Stand
Code for a rocket test stand to measure and record thrust

The code consists of two parts - main.cpp inside the src folder, which runs on an ESP8266 connected to a HX711 breakout with a load cell, and an Electron application that is to be connected to the ESP8266 inside the frontend folder.

The application uses mDNS to find the IP address of the ESP8266, and uses a fast, lightweight graphing library to plot data from the load cell at around **100 Hz**.

The connection drops are a little unbearable if the wireless connection is slow, and for some reason, does not work very well on mobile hotspots.

Here are some flaws with this cocde:
 - Uses TCP because I didn't want any data loss because the onboard data logging is kinda meh
 - Uses an Electron app to recieve the data instead of something like LabView, mostly because I have never used LabView.
 - Needs a really strong network connection(probably because of the TCP)
 - Writes data logs as a CSV string instead of binary, because I really didn't want to figure out how to serialize and deserialze binary accross platforms.
 - Hackathon-quality code
