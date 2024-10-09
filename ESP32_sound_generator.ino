//Rx servo reader library : https :  //github.com/rewegit/esp32-rmt-pwm-reader
#include <esp32-rmt-pwm-reader.h>
// #include "esp32-rmt-pwm-reader.h" // if the lib is located directly in the project directory

// init channels and pins
uint8_t pins[] = { 32, 33, 25 };  // desired input pins
int numberOfChannels = sizeof(pins) / sizeof(uint8_t);



//motor
enum { statusIdle,
       statusStarting,
       statusRunning,
       statusStopping };
int status = statusIdle;  //motor supposed stopped after booting the ESP32
int speedIndex = -1, currentSpeedIndex = 0;
int shoot, previousShoot;
int volIndex = 0, prevVolIndex = 0;
int startDuration = 20000;  //duration of start music in ms
long startTime, stopTime;
int firstTime = 0;
int hasSiren = 0; //by default no siren
                  // =1 siren enabled ("gun channel" will be 3 staes (motor only, motor + siren, motor + siren + gun))

#define LED_PIN 22
//DFPlayer
#define RXD2 23
#define TXD2 19
#define BUSY 26

#define HAS_SIREN


#define CMD_Play_Next 0x01
#define CMD_Play_Previous 0x02
#define CMD_Specify_playback_of_a_track 0x03
#define CMD_Increase_volume 0x04
#define CMD_Decrease_volume 0x05
#define CMD_Specify_volume 0x06
#define CMD_Specify_EQ 0x07
#define CMD_Specify_single_repeat_playback 0x08
#define CMD_Specify_playback_of_a_device 0x09
#define CMD_Set_Sleep 0x0A
#define CMD_Reset 0x0C
#define CMD_Play 0x0D
#define CMD_Pause 0x0E
#define CMD_Specify_playback_a_track_in_a_folder 0x0F
#define CMD_Audio_amplification_setting 0x10
#define CMD_Set_all_repeat_playback 0x11
#define CMD_Specify_playback_of_folder_named_MP3 0x12
#define CMD_Inter_cut_an_advertisement 0x13
#define CMD_Specify_playback_a_track_in_a_folder_3000 0x14
#define CMD_Stop_playing_intercut 0x15
#define CMD_Stop 0x16
#define CMD_Specify_repeat_playback_of_a_folder 0x17
#define CMD_Set_random_playback 0x18
#define CMD_Set_repeat_playback_of_current_track 0x19
#define CMD_Set_DAC 0x1A
#define CMD_Query_current_online_storage_device 0x3F
#define CMD_Module_returns_an_error_data_with_this_command 0x40
#define CMD_Module_reports_a_feedback_with_this_command 0x41
#define CMD_Query_current_status 0x42
#define CMD_Query_current_volume 0x43
#define CMD_Query_current_EQ 0x44
#define CMD_Query_total_file_numbers_of_USB_flash_disk 0x47
#define CMD_Query_total_file_numbers_of_micro_SD_Card 0x48
#define CMD_Query_current_track_of_USB_flash_disk 0x4B
#define CMD_Query_current_track_of_micro_SD_Card 0x4C
#define CMD_Query_total_file_numbers_of_a_folder 0x4E
#define CMD_Query_total_folder_numbers_of_the_storage_device 0x4F

#define TOTAL_FRAME_LEN 10

typedef union {
  uint8_t bytes[TOTAL_FRAME_LEN];
  struct {
    uint8_t _start_byte;
    uint8_t _version;
    uint8_t _length;
    uint8_t CMD;
    uint8_t feedback;
    uint8_t DATA_HI;
    uint8_t DATA_LO;
    uint8_t _chksumHI;
    uint8_t _chksumLO;
    uint8_t _end_byte;
  };
} T_Frame;

T_Frame send_frame, receive_frame;
bool startPlayed = false;

//Preferences
#include <Preferences.h>
Preferences preferences;


///////////////////////////////////////////////////////////////////
// Ajoute un octet à la trame en cours de réception
///////////////////////////////////////////////////////////////////
void AddByteToFrame(uint8_t addByte) {
  int i = 0;
  for (i = 0; i < TOTAL_FRAME_LEN - 1; i++) receive_frame.bytes[i] = receive_frame.bytes[i + 1];
  receive_frame.bytes[TOTAL_FRAME_LEN - 1] = addByte;
}

///////////////////////////////////////////////////////////////////
// Calcul le checksum de la trame
///////////////////////////////////////////////////////////////////

int ComputeChecksum(T_Frame* frame) {
  uint8_t i = 0;
  int checksum = 0;
  checksum = 0;
  for (i = 0; i < frame->_length; i++) {
    checksum += frame->bytes[i + 1];
  }

  checksum = 0 - checksum;
  /* Serial.print("\ncomputed checksum ");
  Serial.println(checksum, HEX);*/
  frame->_chksumHI = (checksum >> 8) & 0xFF;
  frame->_chksumLO = checksum & 0xFF;
  /* Serial.print("frame checksum "); 
  Serial.print(frame->_chksumHI, HEX);
  Serial.print(" ");
  Serial.println(frame->_chksumLO, HEX);*/
  return 0;
}


///////////////////////////////////////////////////////////////////
// Envoie la trame au module MP3
///////////////////////////////////////////////////////////////////

void SendMp3Command(uint8_t data_lo, uint8_t data_hi, uint8_t command) {
  send_frame.CMD = command;
  send_frame.DATA_HI = data_hi;
  send_frame.DATA_LO = data_lo;
  ComputeChecksum(&send_frame);
  Serial2.write(send_frame.bytes, TOTAL_FRAME_LEN);
}


///////////////////////////////////////////////////////////////////
// Reçoie les trames du module MP3
///////////////////////////////////////////////////////////////////

bool ReceiveMp3Command() {
  bool new_data = false;
  uint16_t rec_chksum;

  while (Serial2.available()) {  // there is data in the Serial buffer
    AddByteToFrame(Serial2.read());
    new_data = true;
  }
  if (new_data) {

    if (receive_frame._start_byte == 0x7E && receive_frame._end_byte == 0xEF) {
      //Serial.println("\nnew frame received");
      //for (int i = 0; i < TOTAL_FRAME_LEN; i++) {
      // Serial.print(receive_frame.bytes[i], HEX);
      //  Serial.print(" ");
      //}
      rec_chksum = receive_frame._chksumHI << 8 + receive_frame._chksumLO;
      ComputeChecksum(&receive_frame);
      // if ((receive_frame._chksumHI << 8 + receive_frame._chksumLO) == rec_chksum) {
      //Serial.println("\n checksum ok");
      // Trame valide reçue
      // fin de lecture: 7EFF 063D 0000 xxFE B6EF
      switch (receive_frame.CMD) {
        case 0x3A:
          Serial.println("card inserted");
          break;
        case 0x3B:
          Serial.println("card ejected");
          break;
        case 0x3D:
          {
            Serial.println("card playing finished");
            if (status == statusStarting) startPlayed = true;
            else startPlayed = false;
          }
          break;
        case 0x3F:
          Serial.println("SD card ok");
          volume(volIndex);
          break;
        case 0x40:
          {
            if (receive_frame.DATA_LO == 1) Serial.println("system busy");
            if (receive_frame.DATA_LO == 2) Serial.println("system sleeping");
            if (receive_frame.DATA_LO == 3) Serial.println("system Rx error");
            if (receive_frame.DATA_LO == 4) Serial.println("system checksum error");
            if (receive_frame.DATA_LO == 5) Serial.println("system file out of range");
            if (receive_frame.DATA_LO == 6) Serial.println("system file not found ");
            if (receive_frame.DATA_LO == 7) Serial.println("system insertion error");
          }
          break;
        default:
          Serial.println("\nnew frame received");
          for (int i = 0; i < TOTAL_FRAME_LEN; i++) {
            Serial.print(receive_frame.bytes[i], HEX);
            Serial.print(" ");
          }

          break;
      }
    }
  }
}

///////////////////////////////////////////////////////////////////
// Joue un fichier son
///////////////////////////////////////////////////////////////////
void PlaySound(uint8_t folderNum, uint8_t fileNum) {
  SendMp3Command(fileNum, folderNum, CMD_Specify_playback_a_track_in_a_folder);
  delay(100);
}
void PlaySoundLoop() {
  SendMp3Command(0, 0, CMD_Set_repeat_playback_of_current_track);
  delay(100);
}
void volume(uint8_t vol) {
  SendMp3Command(vol, 0, CMD_Specify_volume);
  Serial.print ("volume set to ");
  Serial.println (volIndex);
  delay(100);
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.begin(115200);

  // init Rx channels
  pwm_reader_init(pins, numberOfChannels);
  pwm_set_channel_pulse_neutral(0, 1000);  // throttle neutral is set to "zero"
  pwm_set_channel_pulse_neutral(1, 1000);  //gun neutral is set to "zero"
  pwm_set_channel_pulse_neutral(2, 1000);  //volume neutral is set to "zero"
  // here you can change channel defaults values before reading (if needed)
  // e.g. pwm_set_channel_pulse_min() /max/neutral
  // e.g. set auto_zero/auto_min_max for channel 0-2
  // for (int ch = 0; ch < 3; ch++) {
  //   pwm_set_auto_zero(ch, true);     // set channel to auto zero
  //   pwm_set_auto_min_max(ch, true);  // set channel to auto min/max calibration
  // }

  // begin reading
  esp_err_t err = pwm_reader_begin();
  if (err != ESP_OK) {
    Serial.printf("begin() err: %i", err);
  } else {
    Serial.println("decoding Rx channels started");
  }
  status = statusIdle;


  //DFPlayer
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  pinMode(BUSY, INPUT_PULLUP);
  send_frame._start_byte = 0x7E;
  send_frame._end_byte = 0xEF;
  send_frame._version = 0xFF;
  send_frame._length = 6;
  send_frame.feedback = 0;
  delay(3000);  //give some time to init

  //Preferences
  preferences.begin("soundMachine", false);

  //preferences.clear();              // Remove all preferences under the opened namespace
  //preferences.remove("counter");   // remove the counter key only
  volIndex = preferences.getInt("volIndex", 20);
  prevVolIndex = volIndex;
  hasSiren = preferences.getInt("hasSiren", 0);
  //preferences.end();  // Close the Preferences
  
  volume(volIndex);  // set volume to default value
  //delay(3000); 
}

void loop() {


  // Reading the actual pulse width of Throttle channel
  speedIndex = constrain((pwm_get_rawPwm(0) - 1000), 0, 1000);  //clip throttle value between 0 and 5
  speedIndex = map(speedIndex, 0, 1000, 0, 5);

  
  //Serial.print("speed\t");
  //Serial.print(speedIndex);
  shoot = constrain((pwm_get_rawPwm(1) - 1000), 0, 1000);  //get machine gun value between 0 and 1
  if (hasSiren == 0) shoot = map(shoot, 0, 1000, 0, 1);
  else               shoot = map(shoot, 0, 1000, 0, 2);
  // Serial.print("\tshoot\t");
  // Serial.println(shoot);


  if (pwm_get_state_name(2) == "STABLE")  // if we receive "volume" channel then compute volIndex and save it if it changes
  {
    volIndex = constrain((pwm_get_rawPwm(2) - 1000), 0, 1000);  //clip volume value between 0 and 30
    volIndex = map(volIndex, 0, 1000, 0, 30);

    if (volIndex != prevVolIndex) {  //change volume
      volume(volIndex);
      prevVolIndex = volIndex;
      preferences.putInt("volIndex", volIndex);
    }
  }


  // Do something with the pulse width... throttle and gun

  switch (status) {  //handle motor and gun
    case statusIdle:
      startTime = millis();
      if (speedIndex > 0) {
        status = statusStarting;
        volume(volIndex);
        currentSpeedIndex = -1;  //will force exit from startmotor playing
        PlaySound(0x00, 0);      //PlaySound(motor_start)/Play the 1.mp3 (motor start)
        Serial.println("starting");
      }
      break;
    case statusStarting:
      if (((millis() - startTime) > startDuration) || (startPlayed == true)) { // wait for the startPayed event of the startmotor track (or failsafe time out)
        status = statusRunning;
        PlaySound(0x00, 1);  //PlaySound(1_mot.wav) now go to the 001 rpm sound
        PlaySoundLoop();
        Serial.println("running");
      }
      break;
    case statusRunning:
      stopTime = millis();
      if ((speedIndex != currentSpeedIndex) || (shoot != previousShoot)) {
        currentSpeedIndex = speedIndex;
        previousShoot = shoot;
        if (speedIndex == 0) {  // change status to stop motor
          status = statusStopping;
        } else {
          PlaySound(shoot, speedIndex);  //Play the speedIndex.wav (motor run (+ gun if shoot)) and loop
          PlaySoundLoop();
        }
      }
      break;
    case statusStopping:
      if (speedIndex == 0) {
        if ((millis() - stopTime) > 100) {
          PlaySound(0x00, 6);  //Play the 06_motorStop.wav (motor stop) and do not loop
          Serial.println("stopping");
          status = statusIdle;
        }
      } else status = statusRunning;
      break;
    default:
      // statements
      break;
  }

  //DFPlayer
  ReceiveMp3Command();
  digitalWrite(LED_PIN, !digitalRead(BUSY));    //if you want to use BUSY signal from DFPlayer 

  if(Serial.available())                                   // if there is data comming from the ESP32 USB serial port 
  {
    String command = Serial.readStringUntil('\n');         // read string until meet newline character
    
    if((command.substring(0, 4) == "VOL=")||(command.substring(0, 4) == "vol=")||(command.substring(0, 4) == "Vol="))                  // VOL=xx to change the volume
    {
      volIndex = command.substring(4).toInt();
      volIndex = constrain(volIndex, 0, 30);
      volume(volIndex);
      prevVolIndex = volIndex;
      preferences.putInt("volIndex", volIndex);
    }
     if((command.substring(0, 4) == "SIR=")||(command.substring(0, 4) == "sir=")||(command.substring(0, 4) == "Sir="))                  // VOL=xx to change the volume
    {
      hasSiren = command.substring(4).toInt();
      hasSiren = constrain(hasSiren, 0, 1);
      preferences.putInt("hasSiren", hasSiren);
      Serial.print("has siren " );
      Serial.println(hasSiren);
    }
  }
}
