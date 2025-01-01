# Terminal  

A terminal-based retro first-person game built entirely in C++. Run, collect orbs, and avoid enemies in a command-prompt environment. This is the first of my one-week projects.

![image](https://github.com/user-attachments/assets/0dbf1e7e-5f0e-4044-b9da-35033c261432)

## Overview  

I was inspired to make Terminal after watching, ["I coded one project EVERY WEEK for a YEAR"](https://www.youtube.com/watch?v=nr8biZfSZ3Y). I was inspired by both the one-week projects and his earlier 3D games. I wanted to make a game out of pure code, and I got started from there. I thought of Pac-Man, and what it might seem like from a first person point of view. 

## Installation  

1. You'll need to be on Windows, and need to download static.wav and ambient.wav as well as terminal.cpp

2. Compile terminal in the same folder as the audio files `g++ terminal.cpp -o terminal -lwinmm`

3. Change your command prompt size to 300 x 90 characters, and font size to 6 or less.

## Controls  

- **Gameplay**:  
  - `W`, `A`, `S`, `D`: Move around  
  - **Mouse**: Move the camera  
  - `Shift`: Sprint  
  - `Esc`: Pause the game  

- **Menus**:  
  - `Up/Down/Left/Right`: Navigate  
  - `Enter`: Select  

## Code Highlights  

- **Pathfinding**: Enemy AI uses the A* algorithm (`findPath()`) to locate the player.
- **Delta Time**: Multiplied all functions which update per second by delta time to ensure framerate does not alter their speed (e.g. movement speed)

## Built With  

- **Language**: C++ and Windows Multimedia (linked with -lwinmm)

## Resources  

- Initial tutorial: [Command Line First Person Shooter Engine](https://www.youtube.com/watch?v=xW8skO7MFYw) by javidx9
- From there, just a lot of math, problem solving, and forum searching

## Limitations

- Window must be near exact dimensions, or else the game looks like nonsense
- Sounds can't play over each other
- Must be on Windows for the Windows Multimedia library and Windows API
- Cursor is visible

## Lessons

- Learned the A* algorithm and how to implement it, in this fairly simple case
- Crafted a lot of helper functions, greatly helping to minimize the amount of code I actually had to write (I initially printed words "pixel" by "pixel" before I got sick of it)
- Learned about raycasting

## License  

This project is licensed under the MIT License.  
