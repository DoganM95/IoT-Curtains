# IoT-Curtains
Outer facade electric curtains controlled by an esp32/arduino using Blynk
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

<table>
  <tr>
    <td width="30%">
- 0.0A -> 3.31V<br/>  
- 0.1A -> 3.13V<br/>  
- 0.2A -> 2.93V<br/>  
- 0.3A -> 2.74V<br/>  
- 0.4A -> 2.55V<br/>  
- 0.5A -> 2.37V<br/>      
    </td>
    <td width="50%"><img src="https://github.com/DoganM95/IoT-Curtains/assets/38842553/9c8e161c-34f9-4ac7-bb64-0f778c8fefd5" alt="USB-C PCB Bottom View"/></td>
  </tr>
</table>

### Hardware setup

- Shorting pins (1 and 2) && (1 and 3) by esp32 happens using AO3401A or AO3400A Mosfet, where esp controls the gate
- Mosfet Gate is connected to esp32 gpio (open/close)
- Mosfet gate is pulled down using a 10 kOhm resistor, connected from gate to ground
- pin 1 is connected to each Mosfet Source/Drain, depending on the diode direction of your mosfet (compare the 2 above in datasheets)
- pin 2/3 are each connected to a Source/Drain
- gpio will be high for a short time (e.g. 0.5s) to simulate a human button press

#### Existing wall module
This is the controller board that is built in to the wall. The wires of the motors of the facade are routed trough the wall and connected to it using the screw terminals. This module stays as is.
<table>
  <tr>
<td width="50%"><img src="https://user-images.githubusercontent.com/38842553/168401683-ad2862df-2277-4cbb-bfc8-14a0a4172ebe.png" alt="Wall module"/></td>
    <td width="50%"><img src="https://github.com/DoganM95/IoT-Curtains/assets/38842553/e5726afd-bb9a-4ce0-ba86-bae11b25775e" alt="Wall module labeled"/></td>
  </tr>
</table>

#### Preview of retrofitting smart module
This is the smart-module built in this project, which attaches to the wall-module above. It replaces the simple stock pcb that was attached to the wall module by default and only provided two led's (green/red) and two buttons (open/close).
<table>
  <tr>
<td width="50%"><img src="https://github.com/DoganM95/IoT-Curtains/assets/38842553/53a401cd-9ca3-4d98-90eb-97e7cb21b43d" alt="USB-C PCB Bottom View"/></td>
    <td width="50%"><img src="https://github.com/DoganM95/IoT-Curtains/assets/38842553/27e2ff01-ed84-4876-893c-6a1bd83d1d68" alt="USB-C PCB Bottom View"/></td>
  </tr>
</table>

