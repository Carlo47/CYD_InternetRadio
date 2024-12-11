#include <Arduino.h>
#include <Preferences.h>

extern Preferences prefs;
extern int currentStation;
extern float currentVolume;


/**
 * Print alarm times retrieved from preferences namespace "alarms"
 * and also piepser state, clock variant and INIT_FLAG
 */
void printPrefs()
{
  prefs.begin("SETTINGS", true);
  Serial.printf("STATION    %d\n", prefs.getInt("STATION"));
  Serial.printf("VOLUME     %4.2f\n", prefs.getInt("VOLUME"));
  Serial.printf("INIT_FLAG  %d\n", prefs.getInt("INIT_FLAG"));
  prefs.end();
}

/**
 * Initialize preferences including INIT_FLAG
 */
void initPrefs()
{
  prefs.begin("SETTINGS");
  
  if (prefs.getInt("INIT_FLAG", 0) != 1947)
  { 
    prefs.clear();
    prefs.putInt("STATION", 0);
    prefs.putFloat("VOLUME", 0.33);
    prefs.putInt("INIT_FLAG", 1947);
  }
  else
  { 
    currentStation = prefs.getInt("STATION");
    currentVolume =  prefs.getFloat("VOLUME");
  }
  prefs.end();
  log_n("==> done");
}