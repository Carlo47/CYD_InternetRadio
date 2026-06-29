/**
 * Program      CYD_InternetRadio.cpp
 *
 * Author       2024-12-08 Charles Geiser (https://www.dodeka.ch)
 *              2025-05-05 Updated to work again with the modified
 *                         AudioTools and ESP32AsyncWebserver libraries
 *              2026-06-29 platformio.ini and source code adapted to use
 *                - ESP-IDF 5.5.4
 *                - Arduino Framework 3.3.8
 *                - LovyanGFX @ 1.2.24
 *                - ESPAsyncWebServer @ 3.11.2
 *                - audio-tools @ 1.2.5+sha.fb0e745
 *                - libhelix @ 0.9.4+sha.7d0444c
 *                - Toolkit @ 1.0.1
 *                - UiComponents @ 1.0.0
 *
 * Purpose      The program shows how to use some functions of the versatile
 *              AudioTools library by Phil Schatzmann and how to realize an
 *              internet radio with touch screen operation.
 *
 *              The initial WiFi connection is established either using
 *              the pre-programmed SSID and password, or - if this fails -
 *              via a mobile phone using “AutoConnectAP” and by visiting
 *              the website 192.168.4.1.
 *
 * Board        ESP32-2432S028R aka CYD or Cheap Yellow Display
 *
 * Remarks      👉 No DAC such as the PCM5102, UDA1334A or VS1053B
 *              is required for output to the speaker connector, but then the
 *              touchpad is blocked because the DAC output pin GPIO_NUM_25 is
 *              used for the clock signal of the touchpad of the CYD.
 *
 *              It is better to use an external DAC, e.g. the UDA1334A for
 *              stereo headphones or 2 Max98357 for speakers.
 *
 *              To display metadata with correct german "Umlaute" we must
 *              supply a font with a coding range from 32 to 255. For this
 *              purpose, we can convert the ttf font Calibri or any other
 *              with the fontconvert tool and include the resulting header file.
 *
 *              fontconvert c:\windows\fonts\calibri.ttf 12 32 255 > Calibri12pt8b.h
 *
 * Caveats      ☢️ The touchpad and SD card are wired on the circuit board in such
 *              a way that they cannot be used simultaneously. Therefore, the
 *              touchpad function must be interrupted in order to activate the
 *              SD card and take a screenshot. Afterwards, the SD card is
 *              deactivated and the touchpad is put back into operation.
 *
 *              👉 The new AudioTools library needs arduino framework 3.x.y
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
#include <AudioTools.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include "AudioTools/Disk/AudioSourceURL.h"
#include "AudioTools/Communication/AudioHttp.h"
#include "Toolkit.h"
#include "UiComponents.h"
#include "Calibri8pt8b.h"
#include "Calibri12pt8b.h"

/** CYD rotation definitions. The origin is always upper left corner
o-------------.    o---|¨|--.    o-------------.    o--------.
|  LANDSCAPE  |    |        |   -|  LANDSCAPE  |    |        |
|  USB RIGHT  |-   |PORTRAIT|   -|  USB LEFT   |    |PORTRAIT|
|             |-   | USB UP |    |             |    |USB DOWN|
'-------------'    |        |    '-------------'    |        |
                   |        |                       |        |
                   '--------'                       '--|_|---'
*/
enum class ROTATION
{
  LANDSCAPE_USB_RIGHT,
  PORTRAIT_USB_UP,
  LANDSCAPE_USB_LEFT,
  PORTRAIT_USB_DOWN
};

const char  NTP_SERVER_POOL[] = "ch.pool.ntp.org";
const char* timeZone          = MEZ_MESZ;
const char  hostname[]        = "CYD-RADIO";
const char  ssid[]            = "your_ssid";     // Replace with your actual WiFi ssid
const char  password[]        = "your_password"; // Replace with your actual WiFi password

const int I2S_BCKL = GPIO_NUM_27; // --> BCKL |               // 26 |
const int I2S_WSEL = GPIO_NUM_21; // --> WSEL |UDA1334A       // 25 | when using spkr output
const int I2S_DOUT = GPIO_NUM_22; // --> DIN  |               // 27 |

const char autoConnectPrompt[] = R"(

 When not connecting...

 Connect your mobile to the
 access point 'AutoConnectAP'
 and enter your WLAN 
 credentials on the webpage 
 http://192.168.4.1)";

AsyncWebServer server(80);

I2SConfig config;
ICYStream url(1024); // or use URLStream url(1024) but no meta data evailable

I2SStream          i2s; // final output of decoded stream, fetched to the external DAC
VolumeStream       volume(i2s);
EncodedAudioStream dec(&volume, new MP3DecoderHelix()); // decode stream and route it to the volume control
StreamCopy         copier(dec, url);                    // copies mp3-stream from url to the decoder

using Action       = void (&)(LGFX& lcd);
using Radiostation = struct rs
{
    const char* name;
    const char* url;
};

//                    Text       Background  Border      Shadow      Font
UiTheme dateTimeTheme(TFT_GREEN, DARKERGREY, DARKERGREY, DARKERGREY, &fonts::FreeSans12pt7b);

LGFX        lcd;
GFXfont     myFont = fonts::DejaVu18;
SPIClass    sdcardSPI(VSPI); // uncomment this line to take screenshots
Wait        waitDateTime;    // diplay date and time every second
Preferences prefs;           // stores current station and volume

extern GFXfont defaultFont;

Radiostation radioStation[] = {
    {"MDR-Klassik", "http://mdr-284350-0.cast.mdr.de/mdr/284350/0/mp3/high/stream.mp3"},
    {"SRF1 AG-SO", "http://stream.srg-ssr.ch/m/regi_ag_so/mp3_128"},
    {"SRF2", "http://stream.srg-ssr.ch/m/drs2/mp3_128"},
    {"SRF3", "http://stream.srg-ssr.ch/m/drs3/mp3_128"},
    {"SRF4 NEWS", "http://stream.srg-ssr.ch/m/drs4news/mp3_128"},
    {"SWISS CLASSIC", "http://stream.srg-ssr.ch/m/rsc_de/mp3_128"},
    {"SWISS JAZZ", "http://stream.srg-ssr.ch/m/rsj/mp3_128"},
    {"Svizra Rumantscha", "http://stream.srg-ssr.ch/m/rr/mp3_128"},
    {"SRF Virus", "http://streaming.swisstxt.ch/m/drsvirus/mp3_128"},
    {"MUSIKWELLE", "http://stream.srg-ssr.ch/m/drsmw/mp3_128"},
    {"BLASMUSIK", "http://stream.bayerwaldradio.com/allesblasmusik"},
    {"Klassik Radio", "http://live.streams.klassikradio.de/klassikradio-deutschland/stream/mp3"},
    {"Radio Classique", "http://radioclassique.ice.infomaniak.ch/radioclassique-high.mp3"},
    {"France Musique", "http://icecast.radiofrance.fr/francemusique-midfi.mp3"},
    {"France Musique Plus", "http://icecast.radiofrance.fr/francemusiqueclassiqueplus-midfi.mp3"},
    {"France Inter Paris (FIP)", "http://icecast.radiofrance.fr/fip-midfi.mp3"},
    {"BR Klassik", "https://dispatcher.rndfnk.com/br/brklassik/live/mp3/mid"},
    {"DLF", "http://st01.dlf.de/dlf/01/128/mp3/stream.mp3"},
    {"WDR", "https://wdr-wdr2-rheinland.icecastssl.wdr.de/wdr/wdr2/rheinland/mp3/128/stream.mp3"},
    {"WDR 1 Live", "http://www.wdr.de/wdrlive/media/einslive.m3u"},
    {"SWR1 BW"
     "https://liveradio.swr.de/sw282p3/swr1bw/"},
    {"SWR2", "https://liveradio.swr.de/sw282p3/swr2/"},
    {"SWR3", "https://liveradio.swr.de/sw282p3/swr3/"},
    {"SWR4 BW", "https://liveradio.swr.de/sw282p3/swr4bw/"},
    {"Blues Mobile", "https://strm112.1.fm/blues_mobile_mp3"},
    {"Jazz MMX", "http://jazz.streamr.ru/jazz-64.mp3"},
    {"HIT Radio FFH MP3", "http://mp3.ffh.de/radioffh/hqlivestream.mp3"},
    {"Capital London", "http://vis.media-ice.musicradio.com/CapitalMP3"},
    {"ORF", "https://orf-live.ors-shoutcast.at/vbg-q1a"},
    {"Beatles Radio", "http://www.beatlesradio.com:8000/stream/1/"},
};
constexpr int nbrRadiostations = sizeof(radioStation) / sizeof(radioStation[0]);
int           currentStation   = 5;    // preselected station
float         currentVolume    = 0.33; // initial loudness

// Forward declaration of functions
void firstStation();
void lastStation();
void nextStation();
void prevStation();
void showCurrent();
void cbShowMetaData(MetaDataType info, const char* str, int len);

class UiPanelTitle : public UiPanel
{
  public:
    UiPanelTitle(LGFX& lcd, int x, int y, int w, int h, int bgColor, bool hidden = true)
        : UiPanel(lcd, x, y, w, h, bgColor, hidden)
    {
      if (!_hidden)
      {
        show();
      }
    }

    void show()
    {
      UiPanel::show();
      _lcd.setTextDatum(textdatum_t::middle_left);
      panelText(60, 20, "CYD Web Radio", TFT_MAROON, fonts::DejaVu24);
    }

  private:
};

class UiPanelDateTime : public UiPanel
{
  public:
    UiPanelDateTime(LGFX& lcd, int x, int y, int w, int h, int bgColor, bool hidden = true)
        : UiPanel(lcd, x, y, w, h, bgColor, hidden)
    {
      if (!_hidden)
      {
        show();
      }
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
    UiButton* _theTime = new UiButton(this, _x + 8, _y + 2, 94, 24, dateTimeTheme, "");
    UiButton* _theDate = new UiButton(this, _x + 190, _y + 2, 122, 24, dateTimeTheme, "");

    std::vector<UiButton*> _btns = {_theTime, _theDate};
};

class UiPanelMetaData : public UiPanel
{
  public:
    UiPanelMetaData(LGFX& lcd, int x, int y, int w, int h, int bgColor, bool hidden = true)
        : UiPanel(lcd, x, y, w, h, bgColor, hidden)
    {
      if (!_hidden)
      {
        show();
        panelText(5, 18, "Composer", TFT_MAROON, Calibri12pt8b);
        panelText(5, 38, "Opus", TFT_MAROON, Calibri8pt8b);
      }
    }
};

class UiPanelRadio : public UiPanel
{
  public:
    UiPanelRadio(LGFX& lcd, int x, int y, int w, int h, int bgColor, bool hidden = true)
        : UiPanel(lcd, x, y, w, h, bgColor, hidden)
    {
      if (!_hidden)
      {
        show();
      }
    }

    void show()
    {
      UiPanel::show();
      for (int i = 0; i < _btns.size(); i++)
      {
        _btns.at(i)->draw();
      }
    }

    void                   handleKeys(int x, int y);
    std::vector<UiButton*> getButtons()
    {
      return _btns;
    }

  private:
    int        D         = 5; // distance from the left panel side
    int        d         = 4; // distance between buttons
    UiButton*  _station  = new UiButton(this, _x + D, _y + 10, 40, 26, defaultTheme, "", "");
    UiHslider* _volume   = new UiHslider(this, _x + D + 1 * d + 65, _y + 60, 155, 8, TFT_GOLD, "Volume");
    UiButton*  _recall   = new UiButton(this, _x + D, _y + 50, 60, 26, "recall");
    UiButton*  _store    = new UiButton(this, _x + D, _y + 90, 60, 26, "store");
    UiButton*  _first    = new UiButton(this, _x + 2 * D + 1 * d + 60, _y + 90, 40, 26, "<<", "");
    UiButton*  _previous = new UiButton(this, _x + 2 * D + 2 * d + 100, _y + 90, 32, 26, "<", "");
    UiButton*  _next     = new UiButton(this, _x + 2 * D + 3 * d + 133, _y + 90, 32, 26, ">", "");
    UiButton*  _last     = new UiButton(this, _x + 2 * D + 4 * d + 165, _y + 90, 40, 26, ">>", "Stations");

    std::vector<UiButton*> _btns = {_station, _volume, _first, _previous, _next, _last, _store, _recall};
};

// Declare pointers to the panels and initialize them with nullptr
UiPanelTitle*    panelTitle    = nullptr;
UiPanelDateTime* panelDateTime = nullptr;
UiPanelMetaData* panelMetaData = nullptr;
UiPanelRadio*    panelRadio    = nullptr;

// Declare the static class variable again here in main
std::vector<UiPanel*> UiPanel::panels;

void startPlaying(int station, float loudness)
{
  i2s.begin(config);
  dec.begin();
  volume.begin(config);
  volume.setVolume(loudness);
  UiHslider* s = static_cast<UiHslider*>(panelRadio->getButtons().at(1));
  s->slideToValue(loudness);
  url.begin(radioStation[station].url, "audio/mp3");
}

void stopPlaying()
{
  // panelMetaData->show();
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
  if (currentStation == nbrRadiostations)
    currentStation = 0;
  stopPlaying();
  startPlaying(currentStation, currentVolume);
  showCurrent();
}

void prevStation()
{
  if (currentStation == 0)
    currentStation = nbrRadiostations;
  currentStation--;
  stopPlaying();
  startPlaying(currentStation, currentVolume);
  showCurrent();
}

void storePreference()
{
  prefs.begin("SETTINGS");
  prefs.putInt("STATION", currentStation);
  prefs.putFloat("VOLUME", currentVolume);
  prefs.end();
  log_i("Settings saved to Preferences");
}

void recallPreferences()
{
  prefs.begin("SETTINGS");
  currentStation = prefs.getInt("STATION");
  currentVolume  = prefs.getFloat("VOLUME");
  prefs.end();
  log_i("station=%d, volume=%f", currentStation, currentVolume);
  stopPlaying();
  startPlaying(currentStation, currentVolume);
  showCurrent();
  log_i("Settings recalled from Preferences");
}

void showCurrent()
{
  panelRadio->getButtons().at(0)->updateValue(currentStation);
  panelRadio->getButtons().at(0)->clearLabel();
  panelRadio->getButtons().at(0)->setLabel(radioStation[currentStation].name);
  Serial.printf_P(
      PSTR("Current Station: %s --> %s\n"), radioStation[currentStation].name, radioStation[currentStation].url);
};

/**
 * Save a screenshot to SD card
 */
void takeScreenShot()
{
  tm   rtcTime;
  char buf[48];
  int  bufSize = sizeof(buf);
  getLocalTime(&rtcTime);
  strftime(buf, bufSize, "/Screenshots/Scr%Y%m%d_%H%M%S.bmp", &rtcTime);
  log_i("filename = %s", buf);
  stopPlaying();
  SDCard::initSDCard(sdcardSPI);
  SDCard::saveBmpToSD_24bit(lcd, buf) ? log_i("screenshot %s saved to SD card", buf) : log_i("saving screensho failed");
  SD.end();
  sdcardSPI.end();
  lcd.touch()->init();
  startPlaying(currentStation, currentVolume);
}

void cbShowMetaData(MetaDataType info, const char* str, int len)
{
  String txt;
  int    indexDash;
  switch (info)
  {
    case MetaDataType::Title:
      log_i("%s", MetaDataTypeStr[MetaDataType::Title]);
      log_i("%s", str);
      indexDash = String(str).indexOf(" -"); // log_i("index dash = %d", indexDash);
      panelMetaData->show();
      txt = String(str).substring(0, indexDash);
      panelMetaData->panelText(5, 18, txt, TFT_MAROON, Calibri12pt8b);
      txt = String(str).substring(indexDash > 0 ? indexDash + 3 : 0);
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
  UiHslider* slider;
  double     loudness;

  for (int i = 0; i < _btns.size(); i++)
  {
    if (_btns.at(i)->touched(x, y))
    {
      log_i("Key pressed: %d", i);
      switch (i)
      {
        case 0: // station
          takeScreenShot();
          break;

        case 1: // slider
          slider = (UiHslider*) _btns.at(i);
          slider->slideToPosition(x);
          slider->getValue(loudness);
          currentVolume = (float) loudness;
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
          storePreference();
          break;

        case 7: // recall
          recallPreferences();
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

void printPrefs()
{
  prefs.begin("SETTINGS", true);
  Serial.printf("STATION    %d\n", prefs.getInt("STATION"));
  Serial.printf("VOLUME     %4.2f\n", prefs.getInt("VOLUME"));
  prefs.end();
}

/**
 * Initialize preferences
 */
void initPrefs()
{
  prefs.begin("SETTINGS");

  if (!prefs.isKey("STATION"))
  {
    prefs.putInt("STATION", 0);
    prefs.putFloat("VOLUME", 0.33);
  }
  else
  {
    currentStation = prefs.getInt("STATION");
    currentVolume  = prefs.getFloat("VOLUME");
  }
  prefs.end();
  log_n("==> done");
}

/**
 * Initialize audio output and start playing
 */
void initAudio()
{
  AudioLogger::instance().begin(Serial, AudioLogger::Error);
  url.setMetadataCallback(cbShowMetaData);

  // configure i2s stream
  config          = i2s.defaultConfig(TX_MODE);
  config.pin_bck  = I2S_BCKL;
  config.pin_ws   = I2S_WSEL;
  config.pin_data = I2S_DOUT;
  startPlaying(currentStation, currentVolume);
  log_i("==> done");
}

void initPanels()
{
  // Create the panels and showm them ( argument hidden is set to false)
  panelTitle    = new UiPanelTitle(lcd, 0, 0, lcd.width(), 35, TFT_GOLD, false);
  panelDateTime = new UiPanelDateTime(lcd, 0, 35, lcd.width(), 30, DARKERGREY, false);
  panelMetaData = new UiPanelMetaData(lcd, 0, 65, lcd.width(), 50, TFT_SILVER, false);
  panelRadio    = new UiPanelRadio(lcd, 0, 115, lcd.width(), lcd.height() - 115, TFT_MAROON, false);

  // Initialize the static class variable with all panels
  UiPanel::panels = {panelTitle, panelDateTime, panelMetaData, panelRadio};

  panelRadio->getButtons().at(0)->updateValue(currentStation);
  panelRadio->getButtons().at(0)->setLabel(radioStation[currentStation].name);
  UiHslider* s = reinterpret_cast<UiHslider*>(panelRadio->getButtons().at(1));
  s->setRange(0.0, 1.0);
  s->slideToValue(currentVolume);
}

/**
 * Corrects the rotation of the touchpad when
 * the display is rotated with setRotation().
 */
bool getMappedTouch(LGFX& lcd, int& x, int& y)
{
  int     x0, y0;
  uint8_t rot = lcd.getRotation();
  if (lcd.getTouch(&x0, &y0))
  {
    if (rot == 1 || rot == 3)
    {
      x = map(x0, lcd.width(), 0, 0, lcd.width());
      y = map(y0, lcd.height(), 0, 0, lcd.height());
    }
    else
    {
      x = x0;
      y = y0;
    }
    return true;
  }
  return false;
}

void setup()
{
  Serial.begin(115200);

  lcd.init();
  lcd.setRotation((uint8_t) ROTATION::LANDSCAPE_USB_RIGHT);

  bool connected = InitWiFi::initWiFi(ssid, password, hostname); // Try connecting with given WiFi credentials
  if (connected)
  {
    log_i("Connected with predefined ssid/password: %s/%s\n", ssid, password);
  }
  else
  {
    log_i("Connecting to WiFi with predefined ssid/password failed: %s/%s", ssid, password);
    log_i("%s", autoConnectPrompt);
    lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setFont(&fonts::DejaVu18);
    lcd.printf(autoConnectPrompt, lcd.width() / 2, 20);
    SDCard::saveBmpToSD_16bit(lcd, "/prompt_autoconnect.bmp");
    InitWiFi::initESP32AutoConnect(hostname); // Autoconnect or ask for credentials on mobile phone
    InitWiFi::printNearbyNetworks();
    InitWiFi::printConnectionDetails();
    lcd.clear();
  }

  initPrefs();  // initialize volume and station from Preferences
  printPrefs(); // print current station and volume to serial monitor
  initPanels();
  SDCard::initSDCard(sdcardSPI);
  SDCard::printSDCardInfo();       // print information about the SD card
  SDCard::listFiles(SD.open("/")); // list all files in the root directory of the SD card
  SD.end();                        // stop SD card to free the SPI bus for the touchpad
  sdcardSPI.end();                 // stop SPI bus for SD card to free the SPI bus for the touchpad
  lcd.touch()->init();
  initAudio();
  InitRTC::initRTC(timeZone, NTP_SERVER_POOL);
  EspSys::printSystemInfo();

  waitDateTime.begin(1000);
  log_i("==> done");
}

void loop()
{
  int x, y;

  if (!panelDateTime->isHidden() && waitDateTime.isOver())
  {
    panelDateTime->updateDateTime();
  }

  if (getMappedTouch(lcd, x, y))
  {
    // Serial.printf("Key pressed at %3d, %3d\n", x, y);
    if (!panelRadio->isHidden())
      panelRadio->handleKeys(x, y);
    vTaskDelay(pdMS_TO_TICKS(200));
  }

  copier.copy();
}