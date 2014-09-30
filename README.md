Connected weigh scale
=====================

Source code for a Spark Core to read from a weigh scale, display the result on an and OLED display and optionally upload the result to the cloud.

The setup utilized a high gain 24-bit ADC HX711 to digitize the amplified voltage difference across a Wheatstone bridge - the "load cell", that measures the weight.

The output of the ADC is read by the Spark Core and in turn displayed on a small, 128x64 OLED display.

Optionally, the measured data can be uploaded to the cloud for other purposes, such as Thingspeak.com for charting (included as sample in source), or pushingbox.com for alerts.

Details please see <a href="http://blog.l3l4.com/iot/kitchenscale01/">Conneting a Weigh Scale</a>
