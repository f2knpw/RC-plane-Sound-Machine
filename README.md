# RC-plane-Sound-Machine
A friend of mine wanted a realistic RC plane warbird with "engine" and "machine gun" sounds.  Throttle stick will startup the engine, then sound will realistically change with the throttle until you fully stop the motor. Additionnally a button can trigger machine gun shots !

This project is also fully described on my hackaday's pages : https://hackaday.io/project/198182-rc-plane-sound-machine


I decided to build this system with the following features :

- play start engine sound when throttle stick leaves "zero" position
- seamlessly increase/decrease motor RPM sound with the throttle stick
- while motor running, play machine gun sound when pressing the gun button on the radio
- play stop engine sound when turning motor off


And to get a good sound level and quality :

- store the sound files into a SD card (you can change these files easily)
- use a DFPlayer mp3/wav module to decode the files
- amplify the sound with a 60W amplifier
- hange the sound volume with a radio channel (potentiometer) or select a fixed volume value 
- use an audio exciter instead of a regular loudspeaker

  code runs on an ESP32 MCU and is available on this repo : 
