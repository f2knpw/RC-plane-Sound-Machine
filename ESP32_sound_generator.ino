#define RXD2 23
#define TXD2 19


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
bool sound_played = LOW;



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
      Serial.println("\nnew frame received");
      for (int i = 0; i < TOTAL_FRAME_LEN; i++) {
        Serial.print(receive_frame.bytes[i], HEX);
        Serial.print(" ");
      }
      rec_chksum = receive_frame._chksumHI << 8 + receive_frame._chksumLO;
      //ComputeChecksum(&receive_frame);
      // if ((receive_frame._chksumHI << 8 + receive_frame._chksumLO) == rec_chksum) {
      Serial.println("\n checksum ok");
      // Trame valide reçue
      // fin de lecture: 7EFF 063D 0000 xxFE B6EF
      if (receive_frame.CMD == 0x3A) Serial.println("card inserted");
      if (receive_frame.CMD == 0x3B) Serial.println("card ejected");
      if (receive_frame.CMD == 0x3D) Serial.println("card playing finished");
      if (receive_frame.CMD == 0x3F) Serial.println("SD card ok");
      if (receive_frame.CMD == 0x40) {
        if (receive_frame.DATA_LO == 1) Serial.println("system busy");
        if (receive_frame.DATA_LO == 2) Serial.println("system sleeping");
        if (receive_frame.DATA_LO == 3) Serial.println("system Rx error");
        if (receive_frame.DATA_LO == 4) Serial.println("system checksum error");
        if (receive_frame.DATA_LO == 5) Serial.println("system file out of range");
        if (receive_frame.DATA_LO == 6) Serial.println("system file not found ");
        if (receive_frame.DATA_LO == 7) Serial.println("system insertion error");
      }
      //}
    }
  }
}

///////////////////////////////////////////////////////////////////
// Joue un fichier son
///////////////////////////////////////////////////////////////////
void PlaySound(uint8_t folderNum, uint8_t fileNum) {
  SendMp3Command(fileNum, folderNum, CMD_Specify_playback_a_track_in_a_folder);
}
void PlaySoundLoop() {
  SendMp3Command(0, 0, CMD_Set_repeat_playback_of_current_track);
}
void volume(uint8_t vol) {
  SendMp3Command(vol, 0, CMD_Specify_volume);
}


void setup() {
  // Note the format for setting a serial port is as follows: Serial2.begin(baud-rate, protocol, RX pin, TX pin);
  Serial.begin(115200);
  //Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  send_frame._start_byte = 0x7E;
  send_frame._end_byte = 0xEF;
  send_frame._version = 0xFF;
  send_frame._length = 6;
  send_frame.feedback = 0;

  sound_played = LOW;
}

void loop() {  //Choose Serial1 or Serial2 as required
  ReceiveMp3Command();
  static unsigned long timer = millis();
  static int index = 1;
  if (millis() - timer > 4000) {
    timer = millis();

    Serial.print("next cmd ");
    Serial.println(index);
    //SendMp3Command(uint8_t data_hi, uint8_t data_lo, uint8_t command)
    //SendMp3Command(0x00, 0x01, CMD_Set_all_repeat_playback);  //loop all files,start
    PlaySound(0x00, index);  //PlaySound(uint8_t folderNum, uint8_t fileNum)
    delay(50);
    PlaySoundLoop();  //Play current track in loop
    delay(50);
    volume(20);   //set volume (0-30)
    index++;
    if (index > 12) index = 0;
  }
}