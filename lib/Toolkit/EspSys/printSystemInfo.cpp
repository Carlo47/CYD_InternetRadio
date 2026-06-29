#include "printSystemInfo.h"

namespace EspSys
{
  /**
   * Prints system info to the serial monitor as shown below:
   *
   *  --- System Info ---
   *  model          ESP32
   *  cores          2
   *  features       WIFI b/g/n   BLE   BT
   *  revision       3
   *
   *  Flash  size: 4194304
   *  Stack  size: 6560
   *  Total  heap: 324580
   *  Free   heap: 278028
   *  No PSRAM
   *  ESP-IDF Version: v5.5.1-710-g8410210c9a
   *  Arduino Core Version: 3.3.4
   */
  void printSystemInfo()
  {
    const char* chipModel[] = {
        "UNKOWN",    // 0
        "ESP32",     // 1
        "ESP32-S2",  // 2
        "",          // 3
        "",          // 4
        "ESP32-C3",  // 5
        "",          // 6
        "",          // 7
        "",          // 8
        "ESP32-S3",  // 9
        "",          // 10
        "",          // 11
        "",          // 12
        "",          // 13
        "",          // 14
        "",          // 15
        "ESP32-H2",  // 16
        "",          // 17
        "ESP32-P4",  // 18
        "",          // 19
        "ESP32-C61", // 20
        "",          // 21
        "",          // 22
        "ESP32-C5",  // 23
        "",          // 24
        "ESP32-H21", // 25
        "",          // 26
        "",          // 27
        "ESP32-H4",  // 28
    };
    const char*     chipFeatures[] = {"FLASH", "WIFI b/g/n", "", "", "BLE", "BT", "IEEE 802.15.4", "PSRAM"};
    esp_chip_info_t chipInfo;
    esp_chip_info(&chipInfo);
    uint32_t b = 1;

    Serial.println("\nSystem Info\n-----------");
    Serial.printf(
        "model          %d=%s\ncores          %d\n", chipInfo.model, chipModel[chipInfo.model], chipInfo.cores);
    Serial.print("features       ");
    for (int i = 0; i < 8; i++)
    { // output the bit-coded features
      if (chipInfo.features & b)
        Serial.printf("%s   ", chipFeatures[i]);
      b <<= 1; // mask the next bit
    }
    Serial.printf("\nrevision       %d\n\n", chipInfo.revision);

    Serial.printf("Flash  size: %d\n", ESP.getFlashChipSize());
    Serial.printf("Stack  size: %d\n", uxTaskGetStackHighWaterMark(NULL));
    Serial.printf("Total  heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free   heap: %d\n", ESP.getFreeHeap());
    if (psramFound())
    {
      Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
      Serial.printf("Free  PSRAM: %d\n", ESP.getFreePsram());
    }
    else
    {
      Serial.printf("No PSRAM\n");
    }

    Serial.printf("\nESP-IDF Version: %s\n", ESP.getSdkVersion());
    Serial.printf("Arduino Core Version: %d.%d.%d\n\n",
                  ESP_ARDUINO_VERSION_MAJOR,
                  ESP_ARDUINO_VERSION_MINOR,
                  ESP_ARDUINO_VERSION_PATCH);
  }
} // namespace EspSys