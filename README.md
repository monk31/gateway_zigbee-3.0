# gateway_zigbee
gateway zigbee raspberry JN5169


If you want to control a smart plug , or light a lamp or monitoring temperature with your smartphone
this application can help you

I use a raspberry pi 3 to make a gateway zigbee but you can also use a beaglebone black with ethernet link.

To communicate with zigbee device, I choose a excellent application : "IOT on off", you can download with applestore
or playstore
see this link : https://www.iot-onoff.com/ for more details

this project has build with three binary and one script in python

see Wiki for installation with a raspberry pi3

Three kinds of device can be appairing in the zigbee network




you must configure broker adress to set your smartphone

you can download "Net Analyser" to scan your network at home


![GitHub Logo](/images/IMG_1367.jpg)





I do a summary of topic that you can configure with your smartphone

 topic state  -> "/sensor/state" + id device  : to know the feedback state of your smart plus or lamp

 topic tmemperature -> "sensor/tmp" + id device
 
 topic humidity -> "sensor/hum" + id device
 



