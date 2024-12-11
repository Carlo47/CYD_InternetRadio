/**
 * Program      CYD_InternetRadio.cpp
 * Author       2024-12-08 Charles Geiser (https://www.dodeka.ch)  
 *  
 * Purpose      The program shows how to use some functions of the versatile 
 *              AudioTools library by Phil Schatzmann and how to realize an 
 *              internet radio with touch screen operation.
 * 
 *              The initial WiFi connection is established with the cell phone 
 *              using AutoConnect and calling up the website 192.168.4.1  
 * 
 * Board        ESP32-2432S028R aka CYD or Cheap Yellow Display
 * 
 * Remarks      👉 No DAC such as the PCM5102, UDA1334A or VS1053B 
 *              is required for output to the speaker connector, but then the 
 *              touchpad is blocked because the DAC output pin GPIO_NUM_25 is 
 *              used for the clock signal of the touchpad of the CYD.
 * 
 *              To display metadata with correct german "Umlaute" we must
 *              supply a font with a coding range from 32 to 255. For this 
 *              purpose, we can convert the ttf font Calibri or any other  
 *              with the fontconvert tool and include the resulting .h file.
 * 
 *              fontconvert c:\windows\fonts\calibri.ttf 12 32 255 > Calibri12pt8b.h  
 * 
 * Wiring
 * ------------ With external DAC UDA1334A -----------------------------
 * 
 *                             .-----------------. 
 *                  22  -->    o DIN             |  
 *                  21  -->    o WSEL       UDA  |    
 *                  27  -->    o BCLK      1334A |   
 *                  3.3V-->    o Vin (3.3-5V)  +---- 3.5mm Stereo Headphone jack 
 *                  GND -->    o GND           +----
 *                             `-----------------´  
 *   
 * ------------ With 2 external MAX98357A PCM Class D Amplifier --------
 *                             .-----------------. 
 *                  21 -->     o LRC             |  
 *                  27 -->     o BCLK       MAX  |
 *                  22 -->     o DIN       98357 |
 *                             o Gain            |   Spkr right
 *         5V Vin ---[560K]----o SD              |    _/|
 *                 GND -->     o GND             o---|  |
 *                 5V  -->     o Vin (5V)        o---|_ |
 *                             `-----------------´     \|   
 *                             .-----------------. 
 *                  21 -->     o LRC             |  
 *                  27 -->     o BCLK       MAX  |
 *                  22 -->     o DIN       98357 |
 *                             o Gain            |   Spkr left
 *         5V Vin ---[180K]----o SD              |    _/|
 *                 GND -->     o GND             o---|  |
 *                 5V  -->     o Vin (5V)        o---|_ |
 *                             `-----------------´     \|
 *
 * References   https://github.com/pschatzmann
 *              https://github.com/pschatzmann/arduino-audio-tools
 *              https://github.com/pschatzmann/arduino-audio-tools/wiki/Volume-Control
 *              https://www.analog.com/media/en/technical-documentation/data-sheets/MAX98357A-MAX98357B.pdf
 * 
 *              How to convert ttf fonts see:
 *              https://www.youtube.com/watch?v=L8MmTISmwZ8
 *              https://github.com/KrisKasprzak/FontConvert/blob/main/FontConvert.zip
 *              http://oleddisplay.squix.ch/
 */
#include <Arduino.h>
#include <WiFi.h>
#include <AudioTools.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "ESP32AutoConnect.h"
#include "lgfx_ESP32_2432S028.h"
#include "Calibri8pt8b.h"
#include "Calibri12pt8b.h"
#include "UiComponents.h"
#include "Wait.h"

const char NTP_SERVER_POOL[] = "ch.pool.ntp.org";
const char TIME_ZONE[]       = "MEZ-1MESZ-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char HOST_NAME[]       = "CYD-RADIO";

const int I2S_BCKL = GPIO_NUM_27;  // --> BCKL |               // 26 |
const int I2S_WSEL = GPIO_NUM_21;  // --> WSEL |UDA1334A       // 25 | when using spkr output
const int I2S_DOUT = GPIO_NUM_22;  // --> DIN  |               // 27 |

AsyncWebServer server(80);

I2SConfig config;
ICYStream url(1024);
//ICYStream url(ssid, password); // Your WiFi SSID and password
I2SStream i2s;   // final output of decoded stream, fetched to the external DAC
VolumeStream volume(i2s);
EncodedAudioStream dec(&volume, new MP3DecoderHelix()); // decode stream and route it to the volume control
StreamCopy copier(dec, url);  // copies mp3-stream from url to the decoder

using Action = void(&)(LGFX &lcd);
using Radiostation = struct rs{ const char *name; const char *url; };

//                    Text       Background  Border      Shadow      Font
UiTheme dateTimeTheme(TFT_GREEN, DARKERGREY, DARKERGREY, DARKERGREY, &fonts::FreeSans12pt7b);

enum class ROTATION { LANDSCAPE_USB_RIGHT, PORTRAIT_USB_UP, 
                      LANDSCAPE_USB_LEFT,  PORTRAIT_USB_DOWN };

LGFX lcd;
GFXfont myFont = fonts::DejaVu18;
Wait waitDateTime(1000);  // diplay date and time every second
Preferences prefs;        // stores current station and volume

extern void nop(LGFX &lcd);
//extern void grid(LGFX &lcd);
extern void calibrateTouchPad(LGFX &lcd);
extern bool getMappedTouch(LGFX &lcd, int &x, int &y);
extern void initESP32AutoConnect(AsyncWebServer &webServer, Preferences &prefs, const char hostname[]);
extern void initDisplay(LGFX &lcd, uint8_t rot, GFXfont *theFont=&myFont, Action greet=nop);
extern bool initWiFi(const char ssid[], const char password[], const char hostname[]);
extern void initPrefs();
extern void printPrefs();
extern GFXfont defaultFont;


Radiostation radioStation[] =
{
  { "MDR-Klassik", "http://mdr-284350-0.cast.mdr.de/mdr/284350/0/mp3/high/stream.mp3" },
  { "SRF1 AG-SO",  "http://stream.srg-ssr.ch/m/regi_ag_so/mp3_128" },
  { "SRF2",        "http://stream.srg-ssr.ch/m/drs2/mp3_128" },
  { "SRF3",        "http://stream.srg-ssr.ch/m/drs3/mp3_128" },
  { "SRF4 NEWS",   "http://stream.srg-ssr.ch/m/drs4news/mp3_128" },
  { "SWISS CLASSIC",     "http://stream.srg-ssr.ch/m/rsc_de/mp3_128" },
  { "SWISS JAZZ",        "http://stream.srg-ssr.ch/m/rsj/mp3_128" },
  { "Svizra Rumantscha", "http://stream.srg-ssr.ch/m/rr/mp3_128" },
  { "SRF Virus",         "http://streaming.swisstxt.ch/m/drsvirus/mp3_128" },
  { "MUSIKWELLE",        "http://stream.srg-ssr.ch/m/drsmw/mp3_128" },
  { "BLASMUSIK",       "http://stream.bayerwaldradio.com/allesblasmusik" },
  { "Klassik Radio",   "http://live.streams.klassikradio.de/klassikradio-deutschland/stream/mp3" },
  { "Radio Classique", "http://radioclassique.ice.infomaniak.ch/radioclassique-high.mp3" },
  { "BR Klassik",      "https://dispatcher.rndfnk.com/br/brklassik/live/mp3/mid" },
  { "DLF",        "http://st01.dlf.de/dlf/01/128/mp3/stream.mp3" },
  { "WDR",        "https://wdr-wdr2-rheinland.icecastssl.wdr.de/wdr/wdr2/rheinland/mp3/128/stream.mp3" },
  { "WDR 1 Live", "http://www.wdr.de/wdrlive/media/einslive.m3u" },
  { "SWR1 BW"     "https://liveradio.swr.de/sw282p3/swr1bw/" },
  { "SWR2",       "https://liveradio.swr.de/sw282p3/swr2/" },
  { "SWR3",              "https://liveradio.swr.de/sw282p3/swr3/" },
  { "SWR4 BW",           "https://liveradio.swr.de/sw282p3/swr4bw/" },
  { "Blues Mobile",      "https://strm112.1.fm/blues_mobile_mp3" },
  { "Jazz MMX",          "http://jazz.streamr.ru/jazz-64.mp3" },
  { "HIT Radio FFH MP3", "http://mp3.ffh.de/radioffh/hqlivestream.mp3" },
  { "Capital London", "http://vis.media-ice.musicradio.com/CapitalMP3" },
  { "ORF",            "https://orf-live.ors-shoutcast.at/vbg-q1a" },
  { "Beatles Radio",  "http://www.beatlesradio.com:8000/stream/1/" },
};
constexpr int nbrRadiostations = sizeof(radioStation) / sizeof(radioStation[0]);
int   currentStation = 5;    // preselected station
float currentVolume  = 0.33; // initial loudness

// Forward declaration of functions
void firstStation();
void lastStation();
void nextStation();
void prevStation();
void showCurrent();
void cbShowMetaData(MetaDataType info, const char *str, int len);


class UiPanelTitle : public UiPanel
{
    public:
        UiPanelTitle(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            if (! _hidden) { show(); }
        }

        void show()
        {
            UiPanel::show();
            _lcd.setTextDatum(textdatum_t::middle_left);
            panelText(60, 20, "CYD Web Radio", TFT_MAROON, fonts::DejaVu18);
        }
    private:
};


class UiPanelDateTime : public UiPanel
{
    public:
      UiPanelDateTime(LGFX & lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) :
        UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
          if (! _hidden) { show(); }
        }

        void updateDateTime();

        void show()
        {
            UiPanel::show();
            for (int i = 0; i < _btns.size(); i++)
            {
                _btns.at(i)->draw();
            }
        }

    private:
        UiButton *_theTime = new UiButton(this, _x+8,  _y+2,  94, 24, dateTimeTheme, "");
        UiButton *_theDate = new UiButton(this, _x+190, _y+2, 122, 24, dateTimeTheme, "");
        
        std::vector<UiButton *> _btns = { _theTime, _theDate }; 
};


class UiPanelMetaData : public UiPanel
{
    public:
        UiPanelMetaData(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden) 
        {
            if (! _hidden) { show(); } 
            panelText(5, 18, "Composer", TFT_MAROON, Calibri12pt8b);
            panelText(5, 38, "Opus", TFT_MAROON, Calibri8pt8b);
        }   
};


class UiPanelRadio : public UiPanel
{
    public:
        UiPanelRadio(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden=true) : 
            UiPanel(lcd, x, y, w, h, bgColor, hidden)
        {
            if (! _hidden) { show(); }
        }

        void show()
        {
            UiPanel::show();
            for (int i = 0; i < _btns.size(); i++)
            {
                _btns.at(i)->draw();
            }
        }

        void handleKeys(int x, int y);
        std::vector<UiButton *> getButtons() { return _btns; }

    private:
      int D = 5; // distance from the left panel side
      int d = 4;  // distance between buttons
      UiButton  *_station  = new UiButton(this,  _x+D,           _y+10,  40, 26, defaultTheme, "", ""); 
      UiHslider *_volume   = new UiHslider(this, _x+D,           _y+60, 220,  8, TFT_GOLD, "Volume");
      UiButton  *_save     = new UiButton(this,  _x+D,           _y+90,  54, 26, "save"); 
      UiButton  *_first    = new UiButton(this,  _x+2*D+1*d+54,  _y+90,  40, 26, "<<", "");
      UiButton  *_next     = new UiButton(this,  _x+2*D+2*d+94,  _y+90,  32, 26, "<", "");
      UiButton  *_previous = new UiButton(this,  _x+2*D+3*d+126, _y+90,  32, 26, ">", "");
      UiButton  *_last     = new UiButton(this,  _x+2*D+4*d+158, _y+90,  40, 26, ">>", "Stations");
      

      std::vector<UiButton *> _btns = { _station, _volume, _first, _next, _previous, _last, _save};
};

// Declare pointers to the panels and initialize them with nullptr
UiPanelTitle    *panelTitle = nullptr;
UiPanelDateTime *panelDateTime = nullptr;
UiPanelMetaData *panelMetaData = nullptr;
UiPanelRadio    *panelRadio = nullptr; 

// Declare the static class variable again here in main
std::vector<UiPanel *> UiPanel::panels;


void startPlaying(int station, float loudness)
{
  i2s.begin(config);
  dec.begin();
  volume.begin(config);
  volume.setVolume(loudness);
  url.begin(radioStation[station]. url, "audio/mp3");
}


void stopPlaying()
{
  panelMetaData->show();
  url.end();
  volume.end();
  dec.end();
  i2s.end();
}


void firstStation()
{
  currentStation = 0;
  stopPlaying();
  startPlaying(currentStation, currentVolume); 
  showCurrent();
}


void lastStation()
{
  currentStation = nbrRadiostations - 1;
  stopPlaying();
  startPlaying(currentStation, currentVolume);
  showCurrent();
}


void nextStation()
{
  currentStation++;
  if (currentStation == nbrRadiostations) currentStation = 0;
  stopPlaying();
  startPlaying(currentStation, currentVolume);
  showCurrent();
}


void prevStation()
{
  if (currentStation == 0) currentStation = nbrRadiostations;
  currentStation--;
  stopPlaying();
  startPlaying(currentStation, currentVolume);
  showCurrent();
}

void savePreference()
{
  prefs.begin("SETTINGS");
  prefs.putInt("STATION", currentStation);
  prefs.putFloat("VOLUME", currentVolume);
  prefs.end();
  log_i("Settings saved to Preferences");
}


void showCurrent() 
{
  panelRadio->getButtons().at(0)->updateValue(currentStation);
  panelRadio->getButtons().at(0)->clearLabel();
  panelRadio->getButtons().at(0)->setLabel(radioStation[currentStation].name);
  Serial.printf_P(PSTR("Current Station: %s --> %s\n"), radioStation[currentStation].name, radioStation[currentStation].url);
};


void cbShowMetaData(MetaDataType info, const char *str, int len)
{
  String txt;
  int indexDash;
  switch (info)
  {
    case MetaDataType::Title:
      log_i("%s", MetaDataTypeStr[MetaDataType::Title]);
      log_i("%s", str);
      indexDash = String(str).indexOf(" -"); //log_i("index dash = %d", indexDash);
      panelMetaData->show();
      txt = String(str).substring(0, indexDash);
      panelMetaData->panelText(5, 18, txt, TFT_MAROON, Calibri12pt8b);
      txt = String(str).substring(indexDash > 0 ? indexDash+3 : 0);
      panelMetaData->panelText(5, 38, txt, TFT_MAROON, Calibri8pt8b);
    break;
    case MetaDataType::Artist:
      log_i("%s", MetaDataTypeStr[MetaDataType::Artist]);
    break;
    case MetaDataType::Album:
      log_i("%s", MetaDataTypeStr[MetaDataType::Album]);
      log_i("%s", str);
    break;
    case MetaDataType::Genre:
      log_i("%s", MetaDataTypeStr[MetaDataType::Genre]);
      log_i("%s", str);
    break;
    case MetaDataType::Name:
      log_i("%s", MetaDataTypeStr[MetaDataType::Name]);
      log_i("%s", str);
    break;
    case MetaDataType::Description:
      log_i("%s", MetaDataTypeStr[MetaDataType::Description]);
      log_i("%s", str);
    break;
  }
}


void UiPanelRadio::handleKeys(int x, int y)
{
  UiHslider *slider;
  double loudness;

    for (int i = 0; i < _btns.size(); i++)
    {
        if (_btns.at(i)->touched(x, y)) 
        {
            log_i("Key pressed: %d", i);
            switch(i)
            {
                case 0: // station
                break;

                case 1: // slider
                  slider = (UiHslider *)_btns.at(i);
                  slider->slideToPosition(x);
                  slider->getValue(loudness);
                  currentVolume = (float)loudness;
                  volume.setVolume(currentVolume);                      
                break;

                case 2:  
                  firstStation();   
                break;

                case 3:
                  prevStation();                   
                break;

                case 4: // first
                  nextStation();                   
                break;

                case 5: // previous
                  lastStation();
                break;

                case 6: // save
                  savePreference();
                break;
            }
            delay(100);
        }
    }
}


void UiPanelDateTime::updateDateTime()
{
    tm   rtcTime;
    char buf[12];
    getLocalTime(&rtcTime);
    strftime(buf, sizeof(buf), "%T", &rtcTime); // hh:mm:ss
    _theTime->updateValue(buf);
    strftime(buf, sizeof(buf), "%F", &rtcTime); // YYYY-MM-DD
    _theDate->updateValue(buf);
}


/**
 * Initialize audio output and start playing
 */
void initAudio()
{
  AudioLogger::instance().begin(Serial, AudioLogger::Error);
  url.setMetadataCallback(cbShowMetaData);

  // configure i2s stream
  config = i2s.defaultConfig(TX_MODE);
  config.pin_bck  = I2S_BCKL;
  config.pin_ws   = I2S_WSEL;
  config.pin_data = I2S_DOUT; 
  startPlaying(currentStation, currentVolume);
  log_i("==> done");  
}


/**
 * Initialize the RTC of the ESP32 with the 
 * time zone and a suitable ntp server pool
 */
void initRTC()
{
  configTzTime(TIME_ZONE, NTP_SERVER_POOL);
  log_i("===> done");
}


void initPanels()
{
  // Create the panels and showm them ( argument hidden is set to false)
  panelTitle    = new UiPanelTitle(lcd, 0, 0,  lcd.width(), 35, TFT_GOLD,  false);
  panelDateTime = new UiPanelDateTime(lcd, 0, 35, lcd.width(), 30, DARKERGREY, false);
  panelMetaData = new UiPanelMetaData(lcd, 0, 65, lcd.width(), 50, TFT_SILVER, false);
  panelRadio    = new UiPanelRadio(lcd, 0, 115, lcd.width(), lcd.height()-115, TFT_MAROON,  false);

  // Initialize the static class variable with all panels
  UiPanel::panels = { panelTitle, panelDateTime, panelMetaData, panelRadio };

  panelRadio->getButtons().at(0)->updateValue(currentStation);
  panelRadio->getButtons().at(0)->setLabel(radioStation[currentStation].name);
  UiHslider *s = reinterpret_cast<UiHslider *>(panelRadio->getButtons().at(1));
  s->setRange(0.0, 1.0);
  s->slideToValue(currentVolume);
}


void setup()
{
  Serial.begin(115200);

  // Initialize and calibrate the display
  //initDisplay(lcd, static_cast<uint8_t>(ROTATION::LANDSCAPE_USB_RIGHT), &defaultFont, calibrateTouchPad);
  
  // Initialize the display without prior calibration
  initDisplay(lcd, static_cast<uint8_t>(ROTATION::LANDSCAPE_USB_RIGHT));
  lcd.print(R"(
  Connetc your cell phone to
  the access point 
  AutoConnectAP and enter
  your WiFi credentials in a
  web browser on the page
  192.168.4.1)");
  initPrefs();
  printPrefs();
  initESP32AutoConnect(server, prefs, HOST_NAME);
  initAudio();
  initRTC();
  initPanels();

  waitDateTime.begin();
  log_i("==> done");
}


void loop()
{
    int x, y;
  
    if (!panelDateTime->isHidden() && waitDateTime.isOver()) { panelDateTime->updateDateTime(); }
 
    if (getMappedTouch(lcd, x, y))
    {
        //Serial.printf("Key pressed at %3d, %3d\n", x, y);
        if (!panelRadio->isHidden()) panelRadio->handleKeys(x, y);
    }

    copier.copy();
}