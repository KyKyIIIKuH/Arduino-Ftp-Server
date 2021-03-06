/*
 * **********************  FTP server library for Arduino **********************
 *                  Copyright (c) 2014-2020 by Jean-Michel Gallego
 *                  
 * This sketch demonstrate how to retieve FTP server state
 *
 * SdFat library version 2.0.2 from William Greiman is used to access the file
 *   system.
 *   
 * Server state is returned by ftpSrv.service() and his value is
 *    cmdStage | ( transferStage << 3 ) | ( dataConn << 6 )
 *  where cmdStage, transferStage and dataConn are private variables of class
 *  FtpServer. The values that they can take are defined respectively by
 *  ftpCmd, ftpTransfer and ftpDataConn in FtpServer.h
 *  
 * For example:
 *   if(( ftpSrv.service() & 0x07 ) > 0 ) ...
 *  to determine that a client is connected
 *  
 *   if(( ftpSrv.service() & 0x38 ) == 0x10 ) ...
 *  to determine that a STORE command is being performed (uploading a file)
 *  
 *   if(( ftpSrv.service() & 0xC0 ) == 0x40 ) ...
 *  to determine that the server is in pasive mode
 *  
 * Please see example FtpServerSdFat2 for more explanations.
 */

#include <SdFat.h>
#include <sdios.h>
#include <FtpServer.h>

// Define Chip Select for your SD card according to hardware 
// #define CS_SDCARD 4  // SD card reader of Ehernet shield
#define CS_SDCARD 53 // Chip Select for SD card reader on Due

// Define Reset pin for W5200 or W5500
// set to -1 for other ethernet chip or if Arduino reset board is used
// #define W5x00_RESET -1
#define W5x00_RESET 8  // on Due

// Define pin for led
//#define LED_PIN LED_BUILTIN
#define LED_PIN 5

SdFat sd;

FtpServer ftpSrv;

// Mac address of ethernet adapter
// byte mac[] = { 0x90, 0xa2, 0xda, 0x00, 0x00, 0x00 };
byte mac[] = { 0x00, 0xaa, 0xbb, 0xcc, 0xde, 0xef };

// IP address of ethernet adapter
// if set to 0, use DHCP for the routeur to assign IP
// IPAddress serverIp( 192, 168, 1, 40 );
IPAddress serverIp( 0, 0, 0, 0 );

ArduinoOutStream cout( Serial );

/*******************************************************************************
**                                                                            **
**                               INITIALISATION                               **
**                                                                            **
*******************************************************************************/

void setup()
{
  Serial.begin( 115200 );
  cout << F( "=== FTP Server state Led ===" ) << endl;

  // initialize digital pin LED_PIN as an output.
  pinMode( LED_PIN, OUTPUT );
  // turn the LED off
  digitalWrite( LED_PIN, LOW );

  // If other chips are connected to SPI bus, set to high the pin connected to their CS
  pinMode( 4, OUTPUT ); 
  digitalWrite( 4, HIGH );
  pinMode( 10, OUTPUT ); 
  digitalWrite( 10, HIGH );

  // Initialize the SD card.
  cout << F("Mount the SD card ... ");
  if( ! sd.begin( CS_SDCARD, SD_SCK_MHZ( 50 )))
  {
    cout << F("Unable to mount SD card") << endl;
    while( true ) ;
  }
  pinMode( CS_SDCARD, OUTPUT ); 
  digitalWrite( CS_SDCARD, HIGH );
  cout << F("ok") << endl;

  // Send reset to Ethernet module
  if( W5x00_RESET > -1 )
  {
    pinMode( W5x00_RESET, OUTPUT );
    digitalWrite( W5x00_RESET, LOW );
    delay( 200 );
    digitalWrite( W5x00_RESET, HIGH );
    delay( 200 );
  }

  // Initialize the network
  cout << F("Initialize ethernet module ... ");
  if((uint32_t) serverIp != 0 )
    Ethernet.begin( mac, serverIp );
  else if( Ethernet.begin( mac ) == 0 )
  {
    cout << F("failed!") << endl;
    while( true ) ;
  }
  cout << F("ok") << endl;

  // Initialize the FTP server
  ftpSrv.init();
  ftpSrv.credentials( "arduino", "test" );
  serverIp = Ethernet.localIP();
  cout << F("IP address of server: ")
       << int( serverIp[0]) << "." << int( serverIp[1]) << "."
       << int( serverIp[2]) << "." << int( serverIp[3]) << endl;

  SdFile::dateTimeCallback( dateTime );
}

// Call back for file timestamps.  Only called for file create and sync().
void dateTime(uint16_t * date, uint16_t * time)
{
  *date = FAT_DATE( 2000, 1, 1 );
  *time = FAT_TIME( 0, 0, 0 );
/*
  DateTime now = rtc.now();
  *date = FS_DATE(now.year(), now.month(), now.day());
  *time = FS_TIME(now.hour(), now.minute(), now.second());
*/
}

/*******************************************************************************
**                                                                            **
**                                 MAIN LOOP                                  **
**                                                                            **
*******************************************************************************/

void loop()
{
  static uint8_t state0;
  static uint32_t tick = 0;
  
  uint8_t state = ftpSrv.service();
  if( state0 != state )
  {
    tick = 0;
    if(( state & 0x07 ) <= 2 )       // no client connected. Led off
      digitalWrite( LED_PIN, LOW );
    else if(( state & 0x38 ) == 0 )  // client connected but no data transfer. Led on
      digitalWrite( LED_PIN, HIGH );
    else                             // data transfer. Led blink
    {
      digitalWrite( LED_PIN, LOW );
      tick = millis() + 100;
    }
    state0 = state;
  }

  if( tick > 0 && (int32_t) ( millis() - tick ) > 0 )
  {
    digitalWrite( LED_PIN, ! digitalRead( LED_PIN ));
    tick = millis() + 100;
  }
 
  // more processes... 
}
