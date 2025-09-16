## CYD_Internetradio with the ESP32-2432S028R (Cheap Yellow Display)
Because the CYD (Cheap Yellow Display) already has a display, it makes 
sense to realize a graphical user interface with touch input for the 
[Simplest Internet Radio](https://github.com/Carlo47/CYD_Simplest_InternetRadio) 
presented a earlier.

### Graphical User Interface
The user interface should offer the following options:
- Display time and date
- Display metadata such as composer and music title
- Display station number and name
- Volume adjustment via slider
- Navigation through the predefined station list
- Saving and recalling the preferred station and volume
- Saving a screenshot to SD card by touching the station number

![img1](images/CYD_RadioGui.png)<br />Graphical User Interface


To be able to display umlauts in German metadata, a character set is 
required that also contains the characters from 128 to 255. Such a 
font can easily be created from any ttf font with the program 
[fontconvert](https://github.com/KrisKasprzak/FontConvert/blob/main/FontConvert.zip) 
and ttf fonts are found under Windows in the font directory. 
But any existing copyright must be respected!

```                                        
                                        size first last  destination
fontconvert c:\windows\fonts\calibri.ttf 8    32    255 > calibri.h
```

After a short time, the designed user interface worked as desired. 
But as soon as I activated the code for the radio, the touch input 
was blocked. The reason was quickly found: The AnalogAudioStream 
outputs the analog signals of the built-in DAC to the GPIO_NUM 25 
and GPIO_NUM_26 pins. Unfortunately the CYD uses GPIO_NUM_25 for 
the clock signal of the touch pad.

### External DAC
Therefore an external DAC must be used, e.g. PCM5102, VS1053B or the 
UDA1334A which I used for my stereo headphones. 

![img2](images/UA1334A.png) UDA1334A

I wired the CYD and the UA1334 as shown in the table:

| Con P3 | UA1334A |    ¦    | Con CN1   | UA1334A |
|:------:|:-------:|:-------:|:---------:|:-------:|
| IO 22  | DIN     |    ¦    | GND       | GND     |
| IO 21  | WSEL    |    ¦    | IO 27     | BCLK    |
|        |         |    ¦    | 3.3V      | VIN     | 

### External Amplifier
If you want to connect 2 stereo speakers instead, you can use 
2 MAX98357 PCM Class D amplifiers. The stereo amplifiers must 
be wired with different resistor values to get the left and right 
stereo channel, see the description in the repository 
[ESP32Internetradio](https://github.com/Carlo47/ESP32InternetRadio).

![img3](images/MAX98357A.png) MAX98357A

I wired the CYD and the MAX98357 as shown in the table:

| MAX98357 |  ¦  | Con CN1 |  ¦  | Con P3 |  ¦  | Con P5 |
|:--------:|:---:|:-------:|:---:|:------:|:---:|:------:|
|   LRC    |  ¦  |         |  ¦  | IO 21  |  ¦  |        |
|   BCLK   |  ¦  |  IO 27  |  ¦  |        |  ¦  |        |
|   DIN    |  ¦  |         |  ¦  | IO 22  |  ¦  |        |
|   GND    |  ¦  |         |  ¦  |        |  ¦  | GND    |
|   Vin    |  ¦  |         |  ¦  |        |  ¦  | VIN    |

```
                      .-----------------. 
           21 -->     o LRC             |  
           27 -->     o BCLK       MAX  |
           22 -->     o DIN       98357 |
                      o Gain            |   Spkr right
  5V Vin ---[560K]----o SD              |    _/|
          GND -->     o GND             o---|  |
          5V  -->     o Vin (5V)        o---|_ |
                      `-----------------´     \|   
                      .-----------------. 
           21 -->     o LRC             |  
           27 -->     o BCLK       MAX  |
           22 -->     o DIN       98357 |
                      o Gain            |   Spkr left
  5V Vin ---[180K]----o SD              |    _/|
          GND -->     o GND             o---|  |
          5V  -->     o Vin (5V)        o---|_ |
                      `-----------------´     \|
 ```

### Connecting to the WLAN
When the web radio is put into operation for the first time, the AutoConnectAP access point is started. Log in here with your cell phone and then enter the SSID and password of your router via the web browser at the URL 192.168.4.1. The login data is then saved permanently so that the web radio connects automatically in future.

### Software 
I use the **LovyanGFX** library for programming graphical elements. I 
programmed some components for the user interface myself and compiled 
them in the UiComponents library. For the audio functions, I planned 
to use Phil Schatzmann's **AudioTools**. However, I encountered problems, 
namely the following:

- The AudioTools require Arduino-ESP32 Core version 3.x, so I had to update 
my Platformio programming environment. To do this, I downloaded the 
Platformio extension **pioarduino** and added this line to platformio.ini 
**platform** = https://github.com/pioarduino/platform-espressif32/releases/download/stable/platform-espressif32.zip. 