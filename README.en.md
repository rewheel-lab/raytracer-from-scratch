# raytracer-from-scratch

This repository contains the code for a video series on the Rewheel Lab YouTube channel. In the series, we build a ray tracer from scratch in C++. We start by sending rays into the screen and work up to rendering a photo-like image with metal and glass.

## Requirements

- Visual Studio Community with the "Desktop development with C++" workload

That is all you need. The projects use no other tools or libraries.

## How to run

1. Click the green "Code" button on this page and choose "Download ZIP".
2. Right-click the downloaded ZIP file and choose "Extract All...".
3. Open the folder for the episode you want to see, then double-click its `.sln` file.
4. When Visual Studio opens, press F5.

## About the folders

Each episode has two folders:

- `epXX_start` ... the code at the start of the episode. Follow along with the video and write the code from here.
- `epXX_done` ... the code at the end of the episode. Compare with this version if your program does not work.

If you get stuck in an episode, you can rejoin at the `_start` folder of a later episode.

## Experiment programs for the extra episodes

The `extra/` folder contains the experiment programs used in the extra episodes. Each folder is one Visual Studio project: double-click its `.sln` file and press F5.

`extra/random` (the extra episode "Random Numbers") plots points on the screen so you can see the patterns in random numbers. Switch how the numbers are made with the keys 1 to 6 (1: `rand()`, 2: the multiply-add-remainder method, 3: the xorshift from Raytracer #07, 4: xorshift with seed 0, 5: xorshift with a fixed seed of 1, which gives the same image every time, 6: xorshift seeded with the startup time, which gives a new image every time). The number sequence and other test results appear in the console window. Press Esc to exit. The full list of controls is at the top of `main.cpp`.

## License

This project is licensed under the MIT License. See `LICENSE`. You may freely modify it or include it in your own work.
