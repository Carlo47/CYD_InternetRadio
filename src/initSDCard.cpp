#include <Arduino.h>
#include <SD.h>


/**
 * Initialize the SD card (Trans Flash) using VSPI (SPI3_HOST) or HSPI_HOST (SPI2_HOST)
 * The pins SCK = 18, MISO = 19, MOSI = 23 and CS = 5 are wired onboard of the CYD
 * Define 
 * SPIClass sdcardSPI(HSPI);
 * or
 * SPIClass sdcardSPI(VSPI);
 * in main.cpp 
 * and pass sdcardSPI as argument to initSDcard()
 * In order to be able to use the touchpad in addition to the display and the SD card, 
 * a software SPI must be implemented for this.
*/
void initSDCard(SPIClass &spi)
{
  // Use custom SPI class
  spi.begin(TF_SCLK, TF_MISO, TF_MOSI, TF_CS);
  if (!SD.begin(TF_CS, spi)) // 👉 Use default frequency of 4MHz
      log_e("==> SD.begin failed!");
  else
      log_e("==> done");

/*     // Use default VSPI with pins 5, 18, 19, 23 (CS, SCLK, MISO, MOSI)
    if (!SD.begin()) // 👉 Use default frequency of 4MHz
        log_e("==> SD.begin failed!");
    else
        log_e("==> done"); */
}


/**
 * Use a raw string literal to print a formatted string of SD card details
*/
void printSDCardInfo()
{
  const char *knownCardTypes[] = {"NONE", "MMC", "SDSC", "SDHC", "UNKNOWN"};
  sdcard_type_t cardType = SD.cardType();
  //uint64_t numSectors= SD.numSectors();
  //uint64_t sectorSize= SD.sectorSize(); 
  uint64_t cardSize  = SD.cardSize() >> 20; // divide by 2^20 = 1'048'576 to get size in MB
  uint64_t cardTotal = SD.totalBytes() >> 20;
  uint64_t cardUsed  = SD.usedBytes() >>  20;
  uint64_t cardFree  = cardTotal - cardUsed; 
  Serial.printf(R"(
SDCard Info
-----------
type     %s
size   %6llu MB
total  %6llu MB
used   %6llu MB
free   %6llu MB
)", knownCardTypes[cardType], cardSize, cardTotal, cardUsed, cardFree);
  Serial.printf("\n");  
}


/**
 * Recursively lists all directories/files of 
 * the file system starting at direcory dir
 * 
 * Usage    listFiles(SD.open("/"));            // show content starting at root
 *          listFiles(SD.open("/SCREENSHOTS"))  // show content of directory SCREENSHOTS
 * 
 * Code borrowed from:
 * https://wiki-content.arduino.cc/en/Tutorial/LibraryExamples/Listfiles * 
*/
void listFiles(File dir, int indent=0) 
{
  while (true) 
  {
    File entry =  dir.openNextFile();

    if (! entry) break; // no more files
    
    Serial.printf("%*c", indent*4, ' ');

    if (entry.isDirectory()) 
    {
      Serial.printf("%s/\n", entry.name());
      listFiles(entry, indent + 1);
    } 
    else 
    {
      // files have sizes, directories do not
      Serial.printf("%s, %d\n", entry.name(), entry.size());
    }
    entry.close();
  }
}