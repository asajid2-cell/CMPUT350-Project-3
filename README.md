# CMPUT 350 HW 2

A small platformer on a component-based ECS engine.

## Build and run

From the repository root:

    cmake -S . -B build
    cmake --build build --config Debug

On Windows the executable is `build/platformer/Debug/platformer.exe`; on Linux it is
`build/platformer`. Run it from the repository root (or pass `-path <assets dir>`) so the
assets folder is found.

## Controls

    Left / Right    move
    Z               jump (hold for a higher jump, tap for a hop)
    C               dash (once a second)
    Down            fast fall

The player starts on the left side of the level. Collect the stars for points; the first
spike corridor is climbed by jumping between its walls, the wide one needs the dash.

See `ABOUT.TXT` for the implementation notes, the resource-lifetime rules and the team split.
