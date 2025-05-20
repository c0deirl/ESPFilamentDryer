# ESPFilamentDryer

This is for controlling a 3D Printer filament dryer with an ESP32!

I modified a Creality Filament Dryer 2.0 after the rotary encoder went bad in it. Using the same heating element and fan, but removing the existing control board, and adding in my own!
Hosts a web interface, that is connected to your wifi (be sure to replace the wifi credentials in the code), and allows you to control the dryer remotely.  
&nbsp;

Materials:

- ESP32 Dev Board
- 5V Solid State Relay - ZeroCrossing - 250v 2A
- 5v Standard Relay - 10a 250v
- SSD1306 Display
- DHT22 Temperature and Humidity sensor

![FilamentDryer](https://github.com/user-attachments/assets/97a9a222-02ea-4390-a8a3-ec0388e8eb22)
