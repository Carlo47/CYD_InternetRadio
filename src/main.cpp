/**
 * Program      CYD_InternetRadio.cpp
 * Author       2024-04-23 Charles Geiser (https://www.dodeka.ch)  
 *  
 * Purpose      The program shows how to use the excellent AudioTools
 *              library from Phil Schatzmann.  
 * 
 * Board        ESP32-2432S028R aka CYD or Cheap Yellow Display
 * 
 * Remarks      👉 No DAC such as the Max98357, PCM5102, UDA1334A or VS1053B 
 *              is required for the output, but then the touchpad is blocked 
 *              because the DAC output pin GPIO_NUM_25 is used for the 
 *              clock signal of the touchpad of the CYD.
 * 
 * ------------ With external DAC UDA1334A -----------------------------
 * 
 *                              .-----------------. 
 *                  RX  -->     o DIN             |  
 *                  D4  -->     o LRC        UDA  |    
 *                  D8  -->     o BCLK      1334A |   
 *                  3.3V-->     o Vin (3.3-5V)  +---- 3.5mm Stereo Headphone jack 
 *                  GND -->     o GND           +----
 *                              `-----------------´    
 * 
 * ---------------------------------------------------------------------
 *   
 *
 * References   https://github.com/pschatzmann
 *              https://github.com/pschatzmann/arduino-audio-tools
 *              https://github.com/pschatzmann/arduino-audio-tools/wiki/Volume-Control
 */
#include <Arduino.h>
#include <AudioTools.h>
#include <AudioCodecs/CodecMP3Helix.h>
#include "lgfx_ESP32_2432S028.h"
#include "UiComponents.h"
#include "Wait.h"

const char ssid[]     = "Your SSID";
const char password[] = "Your Password";
const char NTP_SERVER_POOL[] = "ch.pool.ntp.org";
const char TIME_ZONE[]       = "MEZ-1MESZ-2,M3.5.0/02:00:00,M10.5.0/03:00:00";
const char HOST_NAME[]       = "CYD-RADIO";

I2SConfig config;
ICYStream url(ssid, password); // Your WiFi SSID and password
I2SStream i2s;   // final output of decoded stream, fetched to the external DAC
VolumeStream volume(i2s);
EncodedAudioStream dec(&volume, new MP3DecoderHelix()); // decode stream and route it to the volume control
StreamCopy copier(dec, url);  // copies mp3-stream from url to the decoder

using Action = void(&)(LGFX &lcd);
using Radiostation = struct rs{ const char *name; const char *url; };

//                    Text       Background  Border      Shadow      Font
UiTheme dateTimeTheme(TFT_GREEN, DARKERGREY, DARKERGREY, DARKERGREY, &fonts::FreeSans12pt7b);

enum Rotation {PORTRAIT, LANDSCAPE};
LGFX lcd;
GFXfont myFont = fonts::DejaVu18;
Wait waitUserInput(100);
Wait waitDateTime(1000);

extern void nop(LGFX &lcd);
extern void grid(LGFX &lcd);
extern void initDisplay(LGFX &lcd, uint8_t rotation=0, GFXfont *theFont=&myFont, Action greet=nop);
extern GFXfont defaultFont;


Radiostation radioStation[] =
{
  { "SRF1 AG-SO",    "http://stream.srg-ssr.ch/m/regi_ag_so/mp3_128" },
  { "SRF2",          "http://stream.srg-ssr.ch/m/drs2/mp3_128" },
  { "SRF3",          "http://stream.srg-ssr.ch/m/drs3/mp3_128" },
  { "SRF4 NEWS",     "http://stream.srg-ssr.ch/m/drs4news/mp3_128" },
  { "SWISS CLASSIC", "http://stream.srg-ssr.ch/m/rsc_de/mp3_128" },
  { "SWISS JAZZ",    "http://stream.srg-ssr.ch/m/rsj/mp3_128" },
  { "MUSIKWELLE",    "http://stream.srg-ssr.ch/m/drsmw/mp3_128" },
  { "BLASMUSIK",     "http://stream.bayerwaldradio.com/allesblasmusik" },
  { "Klassik Radio", "http://live.streams.klassikradio.de/klassikradio-deutschland/stream/mp3" },
  { "DLF",           "http://st01.dlf.de/dlf/01/128/mp3/stream.mp3" },
  { "WDR",           "https://wdr-wdr2-rheinland.icecastssl.wdr.de/wdr/wdr2/rheinland/mp3/128/stream.mp3" },
  { "SWR4",          "https://liveradio.swr.de/sw282p3/swr4bw/" },
  { "KVB",           "http://kvbstreams.dyndns.org:8000/wkvi-am" },
};
constexpr int nbrRadiostations = sizeof(radioStation) / sizeof(radioStation[0]);
int   currentStation = 4;    // preselected station
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
            panelText(60, 20, "CYD Internet Radio", TFT_MAROON, fonts::DejaVu18);
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
            panelText(5, 18, "Composer", TFT_MAROON, fonts::DejaVu18);
            panelText(5, 38, "Opus", TFT_MAROON, fonts::DejaVu12);
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
      int d = 8;  // distance between buttons
      UiButton *_station   = new UiButton(this, _x+D,         _y+10,  40, 26, defaultTheme, "", ""); 
      UiHslider *_volume     = new UiHslider(this, _x+D     , _y+60, 162,  8, TFT_GOLD, "Volume"); 
      UiButton *_first     = new UiButton(this, _x+D,         _y+90,  40, 26, "<<", "");
      UiButton *_next      = new UiButton(this, _x+D+d+40,    _y+90,  32, 26, "<", "");
      UiButton *_previous  = new UiButton(this, _x+D+d+76,    _y+90,  32, 26, ">", "");
      UiButton *_last      = new UiButton(this, _x+D+2*d+108, _y+90,  40, 26, ">>", "Stations");

      std::vector<UiButton *> _btns = { _station, _volume, _first, _next, _previous, _last};
};


// Declare pointers to the panels and initialize them with nullptr
UiPanelTitle    *panelTitle = nullptr;
UiPanelDateTime *panelDateTime = nullptr;
UiPanelMetaData *panelMetaData = nullptr;
UiPanelRadio    *panelRadio = nullptr; 

// Declare the static class variable again in main
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
      indexDash = String(str).indexOf(" -");
      panelMetaData->show();
      txt = String(str).substring(0, indexDash);
      panelMetaData->panelText(5, 18, txt, TFT_MAROON, fonts::DejaVu18);
      txt = String(str).substring(indexDash+3);
      panelMetaData->panelText(5, 38, txt, TFT_MAROON, fonts::DejaVu12);
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


void initNTP()
{
  configTzTime(TIME_ZONE, NTP_SERVER_POOL);
  log_i("===> done");
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
  config.pin_bck  = 27;
  config.pin_ws   = 21;
  config.pin_data = 22; 

  startPlaying(currentStation, currentVolume);  
}


void setup()
{
  Serial.begin(115200);

  initDisplay(lcd, LANDSCAPE);
  WiFi.setHostname(HOST_NAME);
  initAudio();
  initNTP();

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

  waitUserInput.begin();
  waitDateTime.begin();
  log_i("==> done");
}


void loop()
{
    int x, y;
  
    if (!panelDateTime->isHidden() && waitDateTime.isOver()) panelDateTime->updateDateTime();

    if (lcd.getTouch(&x, &y))
    {
        log_i("Key pressed at %3d, %3d\n", x, y);
        if (!panelRadio->isHidden()) panelRadio->handleKeys(x, y);
    }

    copier.copy();
}