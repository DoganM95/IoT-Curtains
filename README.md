### Cutain controller pcb

- has 24V and GND to power the pcb (step-down converter will be added here to power the esp32)  
  
- Pinout:  

    Pin 5: Motor  
    Pin 4: Motor  
    Pin 3: close curtains  
    Pin 2: open curtains  
    Pin 1: GND  

- shorting pin 1 and pin 2 opens the curtains
- shorting pin 1 and pin 3 closes the curtains

![image](https://user-images.githubusercontent.com/38842553/168401683-ad2862df-2277-4cbb-bfc8-14a0a4172ebe.png)

### Hardware setup

- Shorting pins (1 and 2) && (1 and 3) by esp32 happens using mmbt2222a transistors, where esp controls the gate
- Transistor base is connected to esp32 gpio (open/close)
- Base is pulled down using a 10 kOhm resistor, connected from base to transistors emitter pin
- Voltage potential between pin 1 and pin 2/3 is `3.3v`, which the transistor should be able to handle
- pin 1 is connected to each transistors emitter
- pin 2/3 are each connected to a transistors collector pin
- gpio will be high for a short time (e.g. 0.5s) to simulate a human button press
