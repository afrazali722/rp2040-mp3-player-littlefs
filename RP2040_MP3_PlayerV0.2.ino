#include <Arduino.h>
#include <LittleFS.h>
#include <I2S.h>
#include <BackgroundAudio.h>

// =====================================================
// MAX98357A I2S pins
// =====================================================
#define I2S_BCLK 20
#define I2S_LRC  21  // Automatically BCLK + 1
#define I2S_DOUT 22

// =====================================================
// Push-button GPIO pins
// Buttons connect between GPIO and GND
// =====================================================
#define BUTTON_1_PIN 10
#define BUTTON_2_PIN 11
#define BUTTON_3_PIN 12
#define BUTTON_4_PIN 13

// Onboard LED
#define LED_PIN 28

// MP3 read block
#define AUDIO_BUFFER_SIZE 512

// Button debounce without delay()
#define DEBOUNCE_TIME_MS 25

// =====================================================
// Button and song configuration
// =====================================================
struct ButtonSong {
  uint8_t pin;
  const char *fileName;

  bool rawState;
  bool stableState;

  unsigned long lastChangeTime;
};

ButtonSong buttons[] = {
  {BUTTON_1_PIN, "/song1.mp3", HIGH, HIGH, 0},
  {BUTTON_2_PIN, "/song2.mp3", HIGH, HIGH, 0},
  {BUTTON_3_PIN, "/song3.mp3", HIGH, HIGH, 0},
  {BUTTON_4_PIN, "/song4.mp3", HIGH, HIGH, 0}
};

const uint8_t BUTTON_COUNT =
  sizeof(buttons) / sizeof(buttons[0]);

// =====================================================
// Audio objects
// =====================================================
I2S i2s(OUTPUT);
BackgroundAudioMP3 mp3(i2s);

File songFile;

uint8_t audioBuffer[AUDIO_BUFFER_SIZE];

bool isPlaying = false;
const char *currentSong = nullptr;

// =====================================================
// Stop current playback immediately
// =====================================================
void stopSong() {
  if (songFile) {
    songFile.close();
  }

  // Remove already-buffered MP3 data and reset decoder
  mp3.flush();

  isPlaying = false;
  currentSong = nullptr;

  digitalWrite(LED_PIN, LOW);
}

// =====================================================
// Start selected MP3
// =====================================================
bool playSong(const char *fileName) {
  // Immediately stop the currently playing file
  stopSong();

  if (!LittleFS.exists(fileName)) {
    Serial.print("ERROR: File missing: ");
    Serial.println(fileName);
    return false;
  }

  songFile = LittleFS.open(fileName, "r");

  if (!songFile) {
    Serial.print("ERROR: Cannot open: ");
    Serial.println(fileName);
    return false;
  }

  currentSong = fileName;
  isPlaying = true;

  digitalWrite(LED_PIN, HIGH);

  Serial.println();
  Serial.print("Playing: ");
  Serial.println(fileName);

  Serial.print("File size: ");
  Serial.print(songFile.size());
  Serial.println(" bytes");

  return true;
}

// =====================================================
// Check four buttons without blocking
// Returns the index of a newly pressed button
// Returns -1 if no new press
// =====================================================
int checkButtons() {
  unsigned long now = millis();

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    bool reading = digitalRead(buttons[i].pin);

    // Raw input changed: restart debounce timer
    if (reading != buttons[i].rawState) {
      buttons[i].rawState = reading;
      buttons[i].lastChangeTime = now;
    }

    // Input remained stable long enough
    if (
      (now - buttons[i].lastChangeTime >= DEBOUNCE_TIME_MS) &&
      (buttons[i].stableState != buttons[i].rawState)
    ) {
      buttons[i].stableState = buttons[i].rawState;

      // INPUT_PULLUP means pressed = LOW
      if (buttons[i].stableState == LOW) {
        return i;
      }
    }
  }

  return -1;
}

// =====================================================
// Continuously feed MP3 data to decoder
// Non-blocking: only writes while buffer has space
// =====================================================
void serviceAudio() {
  if (!isPlaying || !songFile) {
    return;
  }

  while (
    songFile.available() &&
    mp3.availableForWrite() >= AUDIO_BUFFER_SIZE
  ) {
    int bytesRead = songFile.read(
      audioBuffer,
      AUDIO_BUFFER_SIZE
    );

    if (bytesRead > 0) {
      mp3.write(audioBuffer, bytesRead);
    }

    // Short read means end of file
    if (bytesRead < AUDIO_BUFFER_SIZE) {
      songFile.close();
      isPlaying = false;
      currentSong = nullptr;

      digitalWrite(LED_PIN, LOW);

      Serial.println("Song finished");
      break;
    }
  }

  // Extra end-of-file check
  if (songFile && !songFile.available()) {
    songFile.close();
    isPlaying = false;
    currentSong = nullptr;

    digitalWrite(LED_PIN, LOW);

    Serial.println("Song finished");
  }
}

// =====================================================
// Print files stored in LittleFS
// =====================================================
void listFiles() {
  Serial.println();
  Serial.println("Files inside LittleFS:");

  Dir dir = LittleFS.openDir("/");
  bool found = false;

  while (dir.next()) {
    found = true;

    Serial.print("  /");
    Serial.print(dir.fileName());
    Serial.print("  ");
    Serial.print(dir.fileSize());
    Serial.println(" bytes");
  }

  if (!found) {
    Serial.println("  No files found");
  }
}

// =====================================================
// Check required MP3 files
// =====================================================
void checkSongFiles() {
  Serial.println();
  Serial.println("Checking MP3 files:");

  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    Serial.print(buttons[i].fileName);
    Serial.print(": ");

    if (LittleFS.exists(buttons[i].fileName)) {
      File file = LittleFS.open(buttons[i].fileName, "r");

      if (file) {
        Serial.print("OK, ");
        Serial.print(file.size());
        Serial.println(" bytes");
        file.close();
      } else {
        Serial.println("cannot open");
      }
    } else {
      Serial.println("MISSING");
    }
  }
}

// =====================================================
// Setup
// =====================================================
void setup() {
  Serial.begin(115200);

  unsigned long serialStart = millis();

  while (!Serial && millis() - serialStart < 5000) {
    // Only waiting during startup
  }

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Internal pull-up: pressed button connects GPIO to GND
  for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
    pinMode(buttons[i].pin, INPUT_PULLUP);

    bool initialState = digitalRead(buttons[i].pin);

    buttons[i].rawState = initialState;
    buttons[i].stableState = initialState;
    buttons[i].lastChangeTime = millis();
  }

  Serial.println();
  Serial.println("================================");
  Serial.println("RP2040 FOUR-BUTTON MP3 PLAYER");
  Serial.println("================================");

  // Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("ERROR: LittleFS mount failed");

    while (true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(200);
    }
  }

  Serial.println("LittleFS mounted successfully");

  FSInfo fsInfo;

  if (LittleFS.info(fsInfo)) {
    Serial.print("LittleFS total: ");
    Serial.print(
      (float)fsInfo.totalBytes / 1024.0 / 1024.0,
      2
    );
    Serial.println(" MB");

    Serial.print("LittleFS used: ");
    Serial.print(
      (float)fsInfo.usedBytes / 1024.0 / 1024.0,
      2
    );
    Serial.println(" MB");

    Serial.print("LittleFS free: ");
    Serial.print(
      (float)(fsInfo.totalBytes - fsInfo.usedBytes) /
      1024.0 / 1024.0,
      2
    );
    Serial.println(" MB");
  }

  listFiles();
  checkSongFiles();

  // Configure I2S output
  i2s.setBCLK(I2S_BCLK);

  // GP21 is selected automatically because it is BCLK + 1
  i2s.setDATA(I2S_DOUT);

  i2s.setBuffers(8, 256);

  Serial.println();
  Serial.println("Starting MP3 decoder...");

  if (!mp3.begin()) {
    Serial.println("ERROR: MP3 decoder failed to start");

    while (true) {
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      delay(100);
    }
  }

  // 1.0 = normal volume
  // Reduce this if audio distorts.
  mp3.setGain(1.0f);

  Serial.println("MP3 decoder started");
  Serial.println();
  Serial.println("GP10 -> song1.mp3");
  Serial.println("GP11 -> song2.mp3");
  Serial.println("GP12 -> song3.mp3");
  Serial.println("GP13 -> song4.mp3");
  Serial.println();
  Serial.println("Ready. Press a button.");
}

// =====================================================
// Main loop — no delay()
// =====================================================
void loop() {
  // Check buttons first for fast response
  int pressedButton = checkButtons();

  if (pressedButton >= 0) {
    Serial.print("Button ");
    Serial.print(pressedButton + 1);
    Serial.println(" pressed");

    playSong(buttons[pressedButton].fileName);
  }

  // Keep filling the background MP3 decoder
  serviceAudio();
}