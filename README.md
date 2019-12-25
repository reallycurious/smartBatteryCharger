# smartBatteryCharger (incomplete)
PIC12F1572 based Charger for 14.4V cordless drill

<b>Warning:</b> This charger design and accompanying code is based on a lot of assumptions. Some of which may or might not hold true. So if you want to adopt this design and/or code do so at your own risk.

After building a 2 meter by 2 meter cupboard and fastening all screws by hand I decided that a cordless drill is needed. So I bought the most inexpensive one I found. This cordless drill came with a 18v 400ma rated power adapter and charger stand that consists of few simple passive components: A 2W 10 Ohm resistor, an 4007 diode, two 3mm leds and two smd 0805 resistors one 2 KOhm the other 100 Ohm. Oh and instructions to charge 3-5 hours: which resulted in a very hot battery. I feared that it might vent electrolyte so I decided to replace electronics in charger stand, making it smarter.
  
So where to start designing a smart charger circuit that fits in housing of the old dumb one: a net search gave some ideas. Article <a href="https://batteryuniversity.com/learn/article/charging_nickel_based_batteries">Charging Nickel-cadmium @ batteryuniversity.com</a> is helpful but insufficient for designing a charger. <a href="http://electronics-diy.com/electronic_schematic.php?id=813">Intelligent NiCd/NiMH Battery Charger @ electronics-diy.com</a> was better: Although there is no source code as the author states "The source code is not freeware or shareware so please don't ask me to mail you the source code as it has commercial applications" reading the article gives a clear idea on how a smart chager should work and helped me designing the algorithm. <a href="https://electrosome.com/uart-pic-microcontroller-mplab-xc8/">Using UART of PIC Microcontroller – MPLAB XC8 @ electrosome.com</a> was helpful designing uart code. I had no such luck in pwm code, no practical examples for pic12f1572, so I cooked something from datasheet. Thankfully datasheet had sample assembly code for ADC.
  
You might ask, why pic12f1572 ? because with a 3,5 TL price tag for <a href="https://www.direnc.net/pic12f1572t-i-mf-dfn8-8bit-32mhz-mikrodenetleyici">dfn footprint version</a>, it was the cheapest but capable microcontroller that I could find at the time. And except its 10 bit ADC (not 12 bit) I think it is very suitable for the task. I should add that I had not programmed a PIC micro before. I had to learn pic programming (mostly reading the datasheet) for programming the pic12f1572.
