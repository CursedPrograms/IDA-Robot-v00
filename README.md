<p align="center"><img src="images/avatar_round.png" width="160" alt="IDA"></p>

<h1 align="center">IDA</h1>
<h3 align="center">Integrated Drive Automaton</h3>
<p align="center"><i>A DREAM Robotics Agent</i></p>

<div align="center">

[![Twitter: @NorowaretaGemu](https://img.shields.io/badge/X-@NorowaretaGemu-blue.svg?style=flat)](https://x.com/NorowaretaGemu) [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

</div>

<div align="center">
  <a href="https://ko-fi.com/cursedentertainment">
    <img src="https://ko-fi.com/img/githubbutton_sm.svg" alt="ko-fi" width="180"/>
  </a>
</div>

<div align="center">
  <img alt="C++" src="https://img.shields.io/badge/c++%20-%23323330.svg?&style=for-the-badge&logo=c%2B%2B&logoColor=white"/>
  <img alt="Arduino" src="https://img.shields.io/badge/-Arduino-323330?style=for-the-badge&logo=arduino&logoColor=white"/>
  <img alt="Git" src="https://img.shields.io/badge/git%20-%23323330.svg?&style=for-the-badge&logo=git&logoColor=white"/>
  <img alt="Shell" src="https://img.shields.io/badge/Shell-%23323330.svg?&style=for-the-badge&logo=gnu-bash&logoColor=white"/>
</div>

---

- Robot Type: Tank

<div align="center">
  <img src="images/ida_avatar.jpg" alt="IDA avatar: a human representation of the robot" width="320"/>
  <p><i>IDA</i></p>
</div>

<div align="center">
  <img src="images/ida-robot-front.png" alt="IDA Robot front view" width="400"/>
  <img src="images/ida-robot-top.png" alt="IDA Robot top view" width="400"/>
</div>

---

### Software
- [Arduino IDE](https://docs.arduino.cc/software/ide/)
- Libraries: `Servo` (bundled with the IDE) and [`IRremote`](https://github.com/Arduino-IRremote/Arduino-IRremote) v4+ (install from the Library Manager)

---

**Windows controller:** [`builds/IDA-Controller.exe`](builds/IDA-Controller.exe) shows IDA's own web page in a window, so it always matches the page. IDA has no WiFi, so it opens NORA's page, which drives her over IR. Source: `scripts/web_controller`.

---

**Other languages:** `scripts/controllers` has this robot's controller in C#, F#, Rust, Go and Julia, all reading one `robot.json`: W A S D to drive, the mode buttons and the sensors. See its README for how to run each.

## Related Projects


<table>
  <tr>
    <td align="center"><a href="https://github.com/CursedPrograms/ARIA-Robot-v00"><img src="https://raw.githubusercontent.com/CursedPrograms/ARIA-Robot-v00/main/images/avatar_round.png" width="72" alt="ARIA"><br>ARIA</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/KIDA-Robot-v01"><img src="https://raw.githubusercontent.com/CursedPrograms/KIDA-Robot-v01/main/images/avatar_round.png" width="72" alt="KIDA-01"><br>KIDA-01</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/KIDA-Robot-v00"><img src="https://raw.githubusercontent.com/CursedPrograms/KIDA-Robot-v00/main/images/avatar_round.png" width="72" alt="KIDA-00"><br>KIDA-00</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/WHIP-Robot-v00"><img src="https://raw.githubusercontent.com/CursedPrograms/WHIP-Robot-v00/main/images/avatar_round.png" width="72" alt="WHIP"><br>WHIP</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/NORA-Robot-v00"><img src="https://raw.githubusercontent.com/CursedPrograms/NORA-Robot-v00/main/images/avatar_round.png" width="72" alt="NORA"><br>NORA</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/MILA-Robot-v00"><img src="https://raw.githubusercontent.com/CursedPrograms/MILA-Robot-v00/main/images/avatar_round.png" width="72" alt="MILA"><br>MILA</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/NIKA-Robot-v00"><img src="https://raw.githubusercontent.com/CursedPrograms/NIKA-Robot-v00/main/images/avatar_round.png" width="72" alt="NIKA"><br>NIKA</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/CursedPrograms/ARM-Robot-v01"><img src="https://raw.githubusercontent.com/CursedPrograms/ARM-Robot-v01/main/images/avatar_round.png" width="72" alt="ARM"><br>ARM</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/DREAM"><img src="https://raw.githubusercontent.com/CursedPrograms/DREAM/main/images/avatar_round.png" width="72" alt="DREAM"><br>DREAM</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/RIFT"><img src="https://raw.githubusercontent.com/CursedPrograms/RIFT/main/images/avatar_round.png" width="72" alt="RIFT"><br>RIFT</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/RITA"><img src="https://raw.githubusercontent.com/CursedPrograms/RITA/main/images/avatar_round.png" width="72" alt="RITA"><br>RITA</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/NINA"><img src="https://raw.githubusercontent.com/CursedPrograms/NINA/main/images/avatar_round.png" width="72" alt="NINA"><br>NINA</a></td>
    <td align="center"><a href="https://github.com/CursedPrograms/TINA"><img src="https://raw.githubusercontent.com/CursedPrograms/TINA/main/images/avatar_round.png" width="72" alt="TINA"><br>TINA</a></td>
  </tr>
</table>

---

## Overview

IDA is MILA's little sibling: the same tank chassis cut down to a plain Arduino, an ultrasonic sensor on a servo, an IR receiver and an L298N. There is no WiFi or Bluetooth. Everything is driven from an IR remote, or by [NORA](https://github.com/CursedPrograms/NORA-Robot-v00) over the IR link below.

- **Drive modes:** OBSTACLE (autonomous, the default at boot), WASD (car-style) and TANK (independent tracks).
- **Obstacle avoidance:** while driving, the sensor sweeps ahead and to both diagonals so walls met at an angle are caught too. When something is within 35 cm ahead or on a diagonal, IDA stops, backs up a little so the tracks have room to pivot, looks left and right and turns towards the more open side. The turn is a random size based on how much room there is: about 45–90° into open space, up to a full 180° when boxed in.
- **Buzzer:** a high beep at power-up and a low beep whenever she stops for an obstacle or the collision guard trips.
- **Safety:** in WASD and TANK modes a collision guard force-stops the robot if something gets within 15 cm while it is driving forward. In WASD mode it also stops as soon as you let go of the arrow key.
- **Speed:** cycle 100 / 75 / 50 / 25 % with the OK button.
- **FOLLOW, HELLO and DANCE:** on her remote's 4, 5 and 6 (guessed codes): she keeps your hand 30 cm from her sonar, or shakes her sonar head and dances on the spot.
- **Ticks near walls:** driving forward, the buzzer ticks faster as a wall gets close, like a parking sensor (from 25 cm before she'd stop).
- **Her mind (through NORA):** **H** on NORA's Python controller runs IDA's LLM mind and drives her over the IR link. She can't report anything back, so her mind knows she's blind: one short step at a time, and her own collision guard still stops her. She's timid and devoted to NORA.
- **Serial output:** distances, turns, mode changes and every IR code are printed on the Serial Monitor at 115200 baud.

---

## Hardware

- Small Wooden Tank Robot Chassis
- Arduino Uno (or Nano)
- 2S 18650
- L298N
- MG95 Servo
- HC-SR04 Ultrasonic Sensor
- 5V DC Motors
- IR Receiver + IR Remote (the same remote as MILA)
- Buzzer

---

## Pinouts

<details>
<summary><b>Arduino pin assignments</b></summary>

### Sensors, Servo & Buzzer

| Signal | Pin |
|---|---|
| TRIG | 8 |
| ECHO | 9 |
| Servo | 10 |
| IR Receiver | 11 |
| Buzzer | A0 |

### L298N

| Signal | Pin | |
|---|---|---|
| ENA | 5 (PWM) | Right motor speed |
| IN1 | 2 | Right motor |
| IN2 | 3 | Right motor |
| ENB | 6 (PWM) | Left motor speed |
| IN3 | 4 | Left motor |
| IN4 | 7 | Left motor |

</details>

> [!IMPORTANT]
> Remove the ENA / ENB jumpers on the L298N, otherwise the speed setting does nothing. Pins 5 and 6 are the only PWM pins left free: the Servo library uses Timer1 (pins 9/10) and IRremote uses Timer2 (pins 3/11).

---

## Controls

<!-- keymap:start -->
<!-- generated by keymap.py --readme; edit the list there -->

**Keyboard: scripts/controllers**

| Key | Does |
|---|---|
| W / S (hold) | forward / back |
| A / D (hold) | turn left / right |
| Space | stop |

**Keyboard: NORA's page**

| Key | Does |
|---|---|
| I J K L | drive IDA from NORA's page |

**IR remote**

| Key | Does |
|---|---|
| 1 | OBSTACLE mode |
| 2 | WASD mode: hold an arrow to drive, release to stop |
| 3 | TANK mode: each colour button switches one track on or off |
| 4 / 5 / 6 | FOLLOW mode / HELLO / DANCE |
| OK | cycle speed 100 / 75 / 50 / 25 % |
| RED / GREEN | left track forward / back (TANK mode) |
| YELLOW / BLUE | right track forward / back (TANK mode) |

<!-- keymap:end -->

## Controls (IR remote)

| Button | |
|---|---|
| `1` | OBSTACLE mode: drives itself and avoids obstacles |
| `4` / `5` / `6` | FOLLOW mode (keeps your hand 30 cm from her sonar) / HELLO / DANCE, on the spot (codes guessed: 0x3, 0x4, 0x5) |
| `2` | WASD mode: hold an arrow to drive, release to stop |
| `3` | TANK mode: each colour button switches one track on or off |
| `OK` | Cycle speed 100 / 75 / 50 / 25 % |

**TANK mode buttons**

| Button | Track |
|---|---|
| RED | Left forward |
| GREEN | Left backward |
| YELLOW | Right forward |
| BLUE | Right backward |

RED + YELLOW drives straight ahead. Press a button again to switch that track off. The receiver can only read one button at a time, so the tracks latch on and off instead of needing to be held.

### IDA link (driven by NORA)

NORA can drive IDA through her IR transmitter, from NORA's web page, Python controller or Bluetooth link. The link uses its own protocol so nothing else in the fleet reacts to it: **Samsung-format IR (38 kHz), address `0x0DA1`**, with command codes no fleet remote uses.

| Code | Command |
|---|---|
| `0x48` | Forward |
| `0x49` | Backward |
| `0x4A` | Left |
| `0x4B` | Right |
| `0x4C` | Stop (also leaves OBSTACLE mode) |
| `0x4D` | OBSTACLE mode |
| `0x4E` | WASD mode |
| `0x4F` | Cycle speed |

A drive command switches IDA into WASD mode by itself. NORA re-sends it every 150 ms while the button is held, and IDA stops once the link has been quiet for 500 ms. Link frames print on the Serial Monitor as `LINK:0x..`.

### Talking with NORA
NORA also chats with IDA over the link (`0x40`–`0x46`: hello, how are you, happy, curious, sleepy, let's play, bye; `0x47` is NORA's silent "I am here" beacon). The robots talk in **Brainfuck**: every phrase is a Brainfuck program that prints its words (hello is `++++++++[>+++++++++++++<-]>.+.` → `hi`), and a robot says it by beeping the program on its buzzer, one tone per symbol, pitched to its own voice. The phrasebook (`talk_bf.h`) is generated by RIFT's `Fleet/brainfuck_talk.py`: phrases 0–6 (hello, how are you, happy, curious, sleepy, let's play, bye) and their replies 7–13. IDA answers phrase *n* with reply *7 + n*, higher than NORA's voice, and prints `TALK:<phrase>` on the Serial Monitor. When NORA's beacon comes back after a minute of silence, IDA greets her (`TALK:NORA is back`). While she's driving (including OBSTACLE mode) she only gives a 40 ms chirp, so answering never holds up the obstacle checks.

> [!TIP]
> Using a different remote? Press its buttons with the Serial Monitor open: each one prints as `IR:0x..`. Copy those codes into the `IR_...` defines at the top of `IDA.ino`.

---

## Quick start

1. Install the `IRremote` library from the Arduino IDE Library Manager.
2. **Check the motors first:** upload `scripts/motor_test/motor_test.ino`. It repeats forward, backward, left and right for 1.5 s each and prints each step on the Serial Monitor. If a track spins the wrong way, swap that motor's two wires on the L298N (OUT1/OUT2 or OUT3/OUT4) instead of changing the code.
3. Upload `scripts/IDA/IDA.ino`. On power-up the servo sweeps left and right, then IDA starts in OBSTACLE mode.
4. Press `2` or `3` on the remote to take over.

### Tuning

These values are at the top of `IDA.ino`. Edit them and re-upload over USB.

| Setting | Default | |
|---|---|---|
| `turnTime` | 550 ms | About a 180° pivot. Obstacle turns are a random 1/4 to all of this. Increase if her 180 falls short |
| `stopDist` | 35 cm | How close an obstacle straight ahead gets before IDA stops and scans |
| `diagStopDist` | 35 cm | The same, for the diagonal looks |
| `backupTime` | 250 ms | How long IDA reverses before scanning (0 turns it off) |
| `guardDist` | 15 cm | How close an obstacle gets before the guard stops a manual drive |

---

<div align="center">&copy; Cursed Entertainment 2026</div>

<div align="center">
  <a href="https://cursed-entertainment.itch.io/">
    <img src="https://github.com/CursedPrograms/cursedentertainment/raw/main/images/logos/logo-wide-grey.png" alt="Cursed Entertainment" width="250"/>
  </a>
</div>

<div align="center">
  <a href="https://github.com/SynthWomb">
    <img src="https://github.com/SynthWomb/synth.womb/blob/main/logos/synthwomb07.png?raw=true" alt="SynthWomb" width="200"/>
  </a>
</div>

---

<!-- DREAM-ECOSYSTEM:START -->
## The DREAM Robotics ecosystem

**Minds** — [DREAM](https://github.com/CursedPrograms/DREAM) companion · [TINA](https://github.com/CursedPrograms/TINA) engineer · [NINA](https://github.com/CursedPrograms/NINA) caretaker · [RIFT](https://github.com/CursedPrograms/RIFT) fleet hub · [LYCEA](https://github.com/CursedPrograms/LYCEA) training school  
**Robots** — [NORA](https://github.com/CursedPrograms/NORA-Robot-v00) · [MILA](https://github.com/CursedPrograms/MILA-Robot-v00) · [WHIP](https://github.com/CursedPrograms/WHIP-Robot-v00) · [KIDA-00](https://github.com/CursedPrograms/KIDA-Robot-v00) · [KIDA-01](https://github.com/CursedPrograms/KIDA-Robot-v01) · [IDA](https://github.com/CursedPrograms/IDA-Robot-v00) · [ARM](https://github.com/CursedPrograms/ARM-Robot-v01) · [NIKA](https://github.com/CursedPrograms/NIKA-Robot-v00) · [ARIA](https://github.com/CursedPrograms/ARIA-Robot-v00)  
**Remotes** — [RITA](https://github.com/CursedPrograms/RITA) handheld remote  
<!-- DREAM-ECOSYSTEM:END -->
