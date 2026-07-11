# RP2040 MP3 Player with LittleFS & MAX98357A

A compact 4-button MP3 player built with the Raspberry Pi Pico (RP2040). Audio files are stored in the Pico's internal flash using LittleFS and played through a MAX98357A I2S amplifier—no SD card required.

## Features

- 🎵 Play 4 different MP3 files using push buttons
- 💾 Stores audio in LittleFS (internal flash)
- 🔊 High-quality I2S audio output
- 🚫 No SD card required
- ⚡ Simple Arduino IDE project
- 🎛 Easy to customize with your own MP3 files

## Hardware

- Raspberry Pi Pico (RP2040)
- MAX98357A I2S Amplifier
- 4Ω/8Ω Speaker
- 4 Push Buttons
- Breadboard
- Jumper Wires

## Wiring

### MAX98357A

| MAX98357A | RP2040 |
|-----------|--------|
| VIN | 5V |
| GND | GND |
| DIN | GP4 |
| BCLK | GP2 |
| LRC | GP3 |

### Push Buttons

| Button | GPIO |
|--------|------|
| Button 1 | GP6 |
| Button 2 | GP7 |
| Button 3 | GP8 |
| Button 4 | GP9 |

All buttons are connected between the GPIO pin and GND using the Pico's internal pull-up resistors.

## Required Libraries

- Arduino-Pico Core
- LittleFS
- BackgroundAudio
- I2S

## Audio Files

Upload the following files to LittleFS:

```
/song1.mp3
/song2.mp3
/song3.mp3
/song4.mp3
```

## Applications

- DIY Soundboard
- Talking Toys
- Greeting Cards
- Interactive Exhibits
- Voice Prompt Systems
- Educational Projects

## License

This project is released under the MIT License.

---

⭐ If you found this project useful, please consider giving it a Star!
