# Guestbook
esp01 guestbook server
Story

Hello,

Here I want to present a simple guest book access point server with a web interface.

How to make it:

Install needed software (Arduino IDE, ESP8266 core for Arduino and Spiffs.

Download and uncompress the zip file. Plug the ESP01 in the programmer and set the programmer switch in the program mode. Then stick it in the usb port. Open Guestbook.ino, compile and upload the program to the ESP01. After that, unplug and plug the programmer again in the usb port. Now upload the ESP8266 data from the Arduino IDE. That's all. On your computer select the access point Guestbook. Open your browser and input the url Guestbook.com. You see the the first site from your ESP.

The admin mode is only fully working in Chrome.

User: admin

Password: esp8266
