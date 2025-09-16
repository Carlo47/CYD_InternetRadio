#include <Arduino.h>
#include <Preferences.h>

extern Preferences prefs;
extern int currentStation;
extern float currentVolume;


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
  
  if (! prefs.isKey("STATION"))
  { 
    prefs.putInt("STATION", 0);
    prefs.putFloat("VOLUME", 0.33);
  }
  else
  { 
    currentStation = prefs.getInt("STATION");
    currentVolume =  prefs.getFloat("VOLUME");
  }
  prefs.end();
  log_n("==> done");
}