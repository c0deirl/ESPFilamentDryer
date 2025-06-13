# ESPFilamentDryer

This is for controlling a 3D Printer filament dryer with an ESP32!

I modified a Creality Filament Dryer 2.0 after the rotary encoder went bad in it. Using the same heating element, but removing the existing control board, and adding in my own!
Hosts a web interface, that is connected to your wifi (be sure to replace the wifi credentials in the code), and allows you to control the dryer remotely.  

This is of course for modifying an already existing Creality Dryer. If you would want to build one yourself from scratch, you will need to source a 120v heating element. 

## Materials:

- ESP32 Dev Board
- 5V Solid State Relay - ZeroCrossing - 250v 2A  
- 5v 5015 Blower
- 5v Standard Relay - 10a 250v
- SH1107 Display
- DHT22 Temperature and Humidity sensor
- 5v Power Supply

## Pictures and Wiring Diagram
![FilamentDryer](https://github.com/user-attachments/assets/97a9a222-02ea-4390-a8a3-ec0388e8eb22)  
![filamentdryer_wiring](https://github.com/user-attachments/assets/dd70efe3-6284-42fd-ba7d-9f101eceab8c)
![image](https://github.com/user-attachments/assets/74e0142c-d565-47c6-bbcb-b6cd182dd120)
![image](https://github.com/user-attachments/assets/0f61dce7-34c8-4902-b9fe-3298bc33c7d3)

