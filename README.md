# ESP32 OBS Live LED & Web Interface (work in progress)
An Arduino script that uses a websocket to connect to OBS Studio and set the connected LEDs to a specific color.

### Implemented Features ✓
- Controll connected LEDs acording to the OBS status
- Web Interface 
  - Save and load settings from ESP32 storage
  - Regulation of streaming and recording color
  - Regulation of the brightness

### Coming Features
- Web Interface (Twitch specific & German)
  - Time schedule after which the LEDs do not turn on even if OBS is still live
  - Manual control of the LEDs
  - Manual color
  - Time schedule for manual control
