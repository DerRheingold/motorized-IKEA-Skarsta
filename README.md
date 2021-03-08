# motorized-IKEA-Skarsta


## The idea
The basic idea behind this was relatively simple: After the crank of my desk broke after a few months (the plastic part came lose) I wanted to motorize my Ikea Skarsta desk to automatically raise or lower by the press of a button. After a quick search I found two pages that inspired me and based on them I started putting the things together and modifying the code for the Arduino. As an added challenge: my desk isn't light. I got a water cooled desktop-PC sitting on it together with a Samsung 49" monitor. So I guess it's a total of 35-40 Kg that needs to be moved in total.

## Inspired by:
* http://cesarmoya.com/blog/motorizing-standup-desk/ (for most of the code and parts-list)
* https://github.com/aenniw/ARDUINO/tree/master/skarsta (for the 3d-printing layouts and the idea with sonar + 7-segment display hidden under a layer of 3D-print)

## The required parts:
* Arduino Uno
* L298N Motor Driver
* 2 x 24v DC Motors (23kg.cm torque)
* 2 x L-Bracket for Motor
* 24v 6A AC/DC Adapter
* 4x Push Buttons
* 4x 10k OHM Resistors
* 2 x Motor Shaft Coupling (6mm to 6 or 7mm)
* Barrel Jack to Cable adapter
* Junction Box (replaceable)
* Solderless Breadboard
* a hand full of jumper wires (Male-Female, as well as Male-Male)
* a friend with a 3D printer 😉
* some time to do metal working on the original drive shaft
* 8x 8mm washers

## The setup
Cut the crank apart if you want to, or use another 6mm allen wrench that you can get to fit. Note that you'll have to to some metal-work on the 6mm allen wrench to make it fit into the 6mm Motor shaft coupling

## The wiring:
![Wiring for the Ikea Skarsta project](https://github.com/DerRheingold/motorized-IKEA-Skarsta/blob/main/wiring/MotorControlWithSonar.jpg)

## First run:
Install everything outside of the desk first. When you're sure that everything is complete and the motors turn correctly install them under your desk. Make sure to measure trice and screw in only once 😉 Between the table and the L-Bracket holding the motors I installed 3mm washers to get the motor into the correct height compared to the drive shaft. 
Once the motors are installed under the table, keep the wiring and Arduino-parts on the table to make a dry run. This will make debugging the software easier. Also Have a look at the Serial Monitor output when it's running, that might help finding errors too.

## 3D print
A friend and colleague of mine was so kind to assist my project when it came to the part of 3D printing. Based on the files provided he shortened the panel to house the display and 4 buttons: up, down, 0 and 1.
Also the housing for the sonar sensor was printed. Yes we could have made some housing to fisonar, display and buttons in one pice. but you need to leave some room for improvement 😉 It's on our backlog for the future 😊

## Problems encountered and solved.
* Voltage: First I tried with a 29V 2A power supply. This didn't provide enough "ompf" to let the motors drive the desk up. It only wen down 🙄 After upgrading to a 24V 6A power supply it worked fine. Though the L298N only outputs 18-19V to the motors the current is strong enough to power the motors. After all they are rated with 3A per motor :)
* NewPing: Initially it worked fine and just as intended. However when I didn't provide USB-power anymore but used an external power supply I constantly got errors. Turns out it's somewhat of a known issue with newPing. So I migrated to "Ultrasonic" which works fine now 😊
