# Terminal  

A terminal-based 80s-style first-person game built entirely in C++. Run, collect orbs, and avoid enemies in a retro, command-prompt environment. This is the first of my one-week projects.

![image](https://github.com/user-attachments/assets/0dbf1e7e-5f0e-4044-b9da-35033c261432)

## Overview  

I was inspired to make Terminal after watching, ["I coded one project EVERY WEEK for a YEAR"](https://www.youtube.com/watch?v=nr8biZfSZ3Y). I was inspired by both the one-week projects and his earlier 3D games. I wanted to make a game out of pure code, and I got started from there. I thought of Pac-Man, and what it might seem like from a first person point of view.
## Features  

- **Gameplay**: Run from enemies and collect orbs, managing your stamina and health. Enemies drain health when they catch you.  
- **Customization**: Adjust FOV and sensitivity settings.  
- **Graphics**: Rendered in an 80s-style aesthetic solely within the terminal.  
- **Controls**: Intuitive FPS controls—WASD for movement, mouse for camera, and more.  

## Installation  

1. Download the following files and place them in the same folder:  
   - `terminal.exe`  
   - `ambient.wav`  
   - `static.wav`
- **Platform**: Windows (requires the `winmm` library)

2. Change your command prompt font size to 6 or smaller, depending on your monitor size.
   - Open command prompt
   - Press CTRL + ,
   - Go to Defaults on the lefthand side
   - Go to Appearance
   - Change font size

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

- **Structure**: The main game loop is organized for readability, with subprocesses for each menu and gameplay elements.  
- **Pathfinding**: Enemy AI uses the A* algorithm (`moveActive()`) to locate the player.  

## Built With  

- **Language**: C++ and Windows Multimedia (linked with -lwinmm)

## Inspiration and Resources  

- Initial tutorial: [Command Line First Person Shooter Engine](https://www.youtube.com/watch?v=xW8skO7MFYw) by javidx9
- From there, just a lot of math, problem solving, and forum searching

## Planned Improvements  

- Noise-based enemy detection system, though I may just pick up on a new project, carrying the lessons from this one onto the next

## License  

This project is licensed under the MIT License.  
