# motorized-IKEA-Skarsta


## The idea
The basic idea behind this was relatively simple: After the crank of my desk broke after a few months (the plastic part came lose) I wanted to motorize my Ikea Skarsta desk to automatically raise or lower by the press of a button. After a quick search I found two pages that inspired me and based on them I started putting the things together and modifying the code for the Arduino. As an added challenge: my desk isn't light. I got a water cooled desktop-PC sitting on it together with a Samsung 49" monitor. So I guess it's a total of 35-40 Kg that needs to be moved in total.

## Inspired by:
* http://cesarmoya.com/blog/motorizing-standup-desk/ (for most of the code and parts-list)
* https://github.com/aenniw/ARDUINO/tree/master/skarsta (for the 3d-printing layouts and the idea with sonar + 7-segment display hidden under a layer of 3D-print)

## The required parts (links to German shops):
* [Arduino Uno - 22€](https://www.exp-tech.de/plattformen/plattformen/arduino-plattform/mainboards/4380/arduino-uno-r3)
* [L298N Motor Driver - 4,50€](https://www.ebay.de/sch/i.html?_from=R40&_nkw=L298N&_sacat=0&rt=nc&LH_PrefLoc=1)
* [2 x 24v DC Motors (23kg.cm torque) - 41,65€/piece](https://www.exp-tech.de/motoren/dc-getriebemotoren/9878/50-1-metal-gearmotor-37dx70l-mm-24v-with-64-cpr-encoder-helical-pinion)
* [2 x L-Bracket for Motor - 6,50€](https://www.exp-tech.de/motoren/zubehoer-fuer-motoren/4857/pololu-stamped-aluminum-l-bracket-pair-for-37d-mm-metal-gearmotors)
* [24v 6A AC/DC Adapter - 17€](https://www.ebay.de/itm/Netzteil-Trafo-DC12-24V-2A-10A-Netzadapter-Driver-f-LED-Strip-Streifen-Notebook/224113909308?ssPageName=STRK%3AMEBIDX%3AIT&var=522903200218&_trksid=p2060353.m2749.l2649)
* [Sonar sensor - HC-SR04 - 3€](https://www.ebay.de/sch/i.html?_from=R40&_nkw=HC-SR04&_sacat=0&rt=nc&LH_PrefLoc=1)
* [TM1637 7-Segment-Display with 4 digits 0,56" - 3€](https://www.ebay.de/itm/TM1637-LED-4-Ziffern-7-Segment-Display-Uhr-Arduino-Raspberry/203030439344?hash=item2f458ea5b0:g:6NsAAOSwYDhdmjKQ)
* [2 x Motor Shaft Coupling (6mm to 7mm) - 3€/piece](https://www.ebay.de/sch/i.html?_from=R40&_nkw=Wellenkupplung+6mm&_sacat=0&rt=nc&LH_PrefLoc=1)
* 4x Push Buttons
* 8x 8mm washers
* 4x 10k OHM Resistors
* Barrel Jack to Cable adapter
* Junction Box
* Solderless Breadboard
* a hand full of jumper wires (Male-Female, as well as Male-Male)
* a friend with a 3D printer 😉
* some time to do metal working on the original drive shaft

Total cost ± 140€ depending on what small parts you have at home anyway.
Links to the shops are no ads, just to the parts I ordered and made some good experience with.

## The setup
Cut the crank apart if you want to, or use another 6mm allen wrench that you can get to fit. I recommend using a 7mm shaft coupling for easiest fit.

## The wiring
![Wiring for the Ikea Skarsta project](https://github.com/DerRheingold/motorized-IKEA-Skarsta/blob/main/wiring/MotorControlWithSonar.jpg)

## First run
![First setup of the system](https://github.com/DerRheingold/motorized-IKEA-Skarsta/blob/main/_pictures/first%20setup.jpg)
Install everything outside of the desk first. When you're sure that everything is complete and the motors turn correctly install them under your desk. Make sure to measure trice and screw in only once 😉 Between the table and the L-Bracket holding the motors I installed 3mm washers to get the motor into the correct height aligned to the drive shaft. 

Once the motors are installed under the table, keep the wiring and Arduino-parts on the table to make a dry run. This will make debugging the software easier. Also Have a look at the Serial Monitor output when it's running, that might help finding errors too.

Remember to unplug the 5V-Pin in the arduino if you're running the external power to the peripherals and have the arduino plugged in to your PC via USB.

## 3D print
A friend and colleague of mine was so kind to assist my project when it came to the part of 3D printing. Based on the files provided he shortened the panel to house the display and 4 buttons: up, down, 0 and 1.

Also the housing for the sonar sensor was printed. Yes we could have made some housing to fit sonar, display and buttons in one pice. That's maybe something for a future update 😉

## Problems encountered and solved
* Voltage: First I tried with a 29V 2A power supply. This didn't provide enough "ompf" to let the motors drive the desk up. It only wen down 🙄 After upgrading to a 24V 6A power supply it worked fine. Though the L298N only outputs 18-19V to the motors the current is strong enough to power the motors. After all they are rated with 3A per motor :)
* NewPing: Initially it worked fine and just as intended. However when I didn't provide USB-power anymore but used an external power supply I constantly got errors. Turns out it's somewhat of [a known issue with newPing](https://arduino.stackexchange.com/questions/17406/ultrasonic-sensor-returns-random-values-on-external-power-supply). So I migrated to "Ultrasonic" which works fine now 😊
