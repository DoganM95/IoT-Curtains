### Curtain controller pcb (wall module below)

- Pinout of screw spots:  

    Pin 5: Motor  
    Pin 4: Motor  
    Pin 3: close curtains  
    Pin 2: open curtains  
    Pin 1: GND  

- shorting pin 1 and pin 2 opens the curtains  
- shorting pin 1 and pin 3 closes the curtains  
- Pinout of female header pins (Wall side):

#### Power

The wall pcb provides power, rated at 3.3 Volts, but drops with increasing current draw.
The esp32 can be powered with a voltage between 2.2V and 3.6V and supposedly draws max 0.26A (Wifi Tx).
The following table contains measurements of voltage, respective to the current drawn at that moment.

- 0.0A -> 3.31V
- 0.1A -> 3.13V
- 0.2A -> 2.93V
- 0.3A -> 2.74V
- 0.4A -> 2.55V
- 0.5A -> 2.37V

### Hardware setup

- Shorting pins (1 and 2) && (1 and 3) by esp32 happens using AO3401A or AO3400A Mosfet, where esp controls the gate
- Mosfet Gate is connected to esp32 gpio (open/close)
- Mosfet gate is pulled down using a 10 kOhm resistor, connected from gate to ground
- pin 1 is connected to each Mosfet Source/Drain, depending on the diode direction of your mosfet (compare the 2 above in datasheets)
- pin 2/3 are each connected to a Source/Drain
- gpio will be high for a short time (e.g. 0.5s) to simulate a human button press

#### Existing wall module

![image](https://user-images.githubusercontent.com/38842553/168401683-ad2862df-2277-4cbb-bfc8-14a0a4172ebe.png)

#### Retrofitting pcb design

![routed_pcb.png](https://github.com/DoganM95/IoT-Curtains/blob/master/Server/ESP32/Hardware/preview/routed_pcb.png)
![schematic.png](https://github.com/DoganM95/IoT-Curtains/blob/master/Server/ESP32/Hardware/preview/schematic.png)
![unrouted_pcb.png](https://github.com/DoganM95/IoT-Curtains/blob/master/Server/ESP32/Hardware/preview/unrouted_pcb.png)
