# esp-idf-a2dp-source
ESP32 Bluetooth A2DP-SOURCE for esp-idf.   
Play wav to speaker via bluetooth.   

# Background   
ESP-IDF contains A2DP-SOURCE demo code.   
https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/a2dp_source

However, this example sends random noise data, not music.   
This project sends WAV(RIFF waveform Audio Format) data using A2DP.   
You can listen WAV data using your bluetooth speaker.   
I used [this](https://github.com/admiralmaggie/esp32_bt_source) as a reference.

# Changes from the referenced code   
- You can specify your speaker name using menuconfig.   
- WAV data is defined as hexadecimal data.   
 This reduces the size of the header file.   
- A header file generator is attached.   

# Hardware requirement    
- Bluetooth speaker

# Installation
```
git clone https://github.com/nopnop2002/esp-idf-a2dp-source
cd esp-idf-a2dp-source
idf.py set-target esp32
idf.py menuconfig
idf.py flash monitor
```

# Configure
You have to set this config value with menuconfig.   
- CONFIG_SPEAKER_NAME   
Your bluetooth speaker name.   
![config-main](https://user-images.githubusercontent.com/6020549/107940288-5c267300-6fcb-11eb-9323-dd8a6cf77c9a.jpg)
![config-app](https://user-images.githubusercontent.com/6020549/107940298-5e88cd00-6fcb-11eb-8c4a-28639db1df96.jpg)   

You can use [this](https://github.com/espressif/esp-idf/tree/master/examples/bluetooth/bluedroid/classic_bt/bt_discovery) to find the name of your Bluetooth speaker.   
```
I (0) cpu_start: Starting scheduler on APP CPU.
I (547) BTDM_INIT: BT controller compile version [7972edf]
I (547) system_api: Base MAC address is not set
I (547) system_api: read default base MAC address from EFUSE
I (557) phy_init: phy_version 4670,719f9f6,Feb 18 2021,17:07:07
I (1247) GAP: Discovery started.
I (3847) GAP: Device found: 54:14:8e:38:a8:28
I (3847) GAP: --Class of Device: 0x340404
I (3847) GAP: --RSSI: -57
I (3847) GAP: Found a target device, address 54:14:8e:38:a8:28, name TG-117 --> This is your speaker
I (3857) GAP: Cancel device discovery ...
I (3867) GAP: Device found: 54:14:8e:38:a8:28
I (3867) GAP: --Class of Device: 0x340404
I (3877) GAP: --RSSI: -56
I (3877) GAP: Device discovery stopped.
I (3877) GAP: Discover services ...
I (5337) GAP: Services for device 54:14:8e:38:a8:28 found
I (5337) GAP: --1101
I (5337) GAP: --111e
I (5337) GAP: --110b
I (5337) GAP: --110e
```

# Convert WAV file to C header format   
It is necessary to read the WAV file at high speed.   
If you put the WAV file in SPIFFS and read it, it will not be in time.   
It can be read at high speed by converting it to the C language header format.   
I made this program with reference to [this](https://blog.goo.ne.jp/lm324/e/ca93257fc9861a07bb6b8f27caa7d382) site.   

```
cd wav2code
make
./wav2code futta-prayer3t.wav music.h
cp music.h ../main/
cd ..
idf.py flash monitor
```

# Free WAV file   
I downloaded the WAV file from [here](https://music.futta.net/mp3.html).   
There is many WAV format data in the Internet.   

# Limitations   
The WAV file header has an average number of bytes per second.   
This determines the speed at which it plays.   
This project ignores this.   
