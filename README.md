You can use this code to upload it to a LilyGo T-Block E-Paper like this one:
https://de.aliexpress.com/item/10000351304793.html

Usage:
1. Download the zipfile from https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library/releases/tag/V1.4.1 using the "Download ZIP" button and install it using the IDE ("Sketch" -> "Include Library" -> "Add .ZIP Library..."
2. Add ESP32 Boards by adding "File" -> "Settings" -> add Boardmanager-URL: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. "Tools" -> "Boards" -> "Boardmanager" -> Search for ESP32 (by Espressif Systems)
4. Change ssid, password, api_url and api_secret regarding to your values in the secrets.h
5. Compile Settings: 
  Board: "TTGO T-Watch"
  Partition Scheme: "Default (2* 6,5 MB app, 3,6 MB SPIFFS"
  PSRAM: "enabled"
  
You can change the Font size by creating a own size under https://rop.nl/truetype2gfx/

