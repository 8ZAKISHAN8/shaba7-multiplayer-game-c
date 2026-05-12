# El Shabah 🎮

A 2D platformer action game inspired by the Egyptian movie **El Shabah (الشبح)**, developed in **C++ using Raylib**.  
The game features both **Single Player** and **Online Multiplayer** modes with platform movement, enemy AI, knife attacks, sound effects, animated cutscenes, and networking using WinSock.

---
## Media 🎥📸

### Gameplay Video
//////////////////////////////////////////////////////////////////// [▶️ Watch Gameplay](./El%20Shabah%20game%20record_compressed.mp4) \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

### Main Menu Screenshot
![El Shabah Main Menu](./El%20Shabah%20main%20menu.png)

## Features ✨

- 🎥 Intro cinematic/trailer system with 700+ frames
- 🕹️ Single Player mode
- 🌐 Online Multiplayer mode using sockets
- 🔪 Knife attack system
- 🏃 Character movement and jumping physics
- 🧠 Enemy AI behavior
- 🎵 Background music and sound effects
- 📖 Interactive "How To Play" menu
- 👨‍💻 Developer credits section
- 🔊 In-game sound settings and volume controls
- 🏙️ Procedurally generated platform terrain
- 🎨 Custom textures, sprites, and UI

---

## Technologies Used 🛠️

- **C++**
- **Raylib**
- **WinSock2**
- **Multithreading**
- **TCP Networking**
- **Windows API**

---

## Gameplay 🎮

### Single Player
- Play as Ahmed while escaping from Samra
- Survive as long as possible
- Avoid falling and dodge knives
- Score increases over time

### Multiplayer
- Host or join a game using local IP
- Choose between Ahmed or Samra
- Real-time movement synchronization
- Knife attacks and player interactions

---

## Controls ⌨️

### Ahmed
| Key | Action |
|------|--------|
| ↑ | Jump |
| ↓ | Fast Fall |
| → | Move Right |

### Samra
| Key | Action |
|------|--------|
| ↑ | Jump |
| ↓ | Fast Fall |
| → | Move Right |
| Space | Throw Knife |

---

## Networking 🌐

The multiplayer system was implemented using:
- TCP sockets
- WinSock2
- Client-server architecture
- Separate networking thread for receiving messages

Players can:
- Host a server
- Share their local IP
- Join using the host IP

---

## Game Trailer 🎥

Watch the gameplay trailer here:

[▶️ Gameplay Video](PUT_YOUR_VIDEO_LINK_HERE)

---

## Screenshots 📸

### Main Menu
![Main Menu](assets/menu.png)

### Gameplay
![Gameplay](assets/gameplay.png)

## Project Structure 📂

ElShabah/
│
├── assets/
├── main.cpp
├── networking.cpp
├── networking.h
└── README.md


## Team 👨‍💻
Karim Zaki
Karim Amr
Noura Hesham
Omar Ahmed
