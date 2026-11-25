# Cross-platform GraalShop animation editor recreated with C++ and SDL3

GraalShop opens Graal `.gani` animation documents in a native SDL3 window, renders frames from the game's sprite sheets, and reports document errors with line numbers.

## Format coverage

Documents may start with a `GANI0001` or `GANI0002` header; files written by older editors omit it. Supported directives:

- `SPRITE <id> <type> <x> <y> <width> <height> [name]`
- `DEFAULTHEAD <image>`, `DEFAULTBODY <image>`, `DEFAULTPARAM<n> <value>`, `DEFAULTATTR<n> <value>`
- `ATTACHSPRITE <sprite> <sprite> <dx> <dy>`
- `FRAMES <count>`, `SINGLEDIRECTION`, `LOOP`, `CONTINUOUS`, `SETBACKTO <animation>`
- `SCRIPT ... SCRIPTEND`
- `ANI ... ANIEND` with one row per direction (four rows, or one with `SINGLEDIRECTION`)
- Frame rows of `sprite dx dy` entries separated by commas, where the sprite is an id or a sprite type name
- `WAIT <repeats>` or `WAIT<repeats>`, and `PLAYSOUND <file> <dx> <dy>`
- `MOVIE ... MOVIEEND` with `ACTOR <type> <name>`, `FRAME <n> key=value,...`, and `ACTOREND`

Parsing is strict: malformed rows, duplicate definitions, unknown directives, missing terminators, and references to undefined sprites are reported with a line number.

## Build

Requires CMake 3.24 or newer and a C++20 compiler. SDL3 3.4.20 and stb_image are fetched at configure time.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

On Windows the executable is written to `bin/GraalShop.exe`; on Linux it is written to `bin/GraalShop`.

## WebAssembly

With the Emscripten SDK installed at `build/emsdk`, run `build_wasm.ps1`. The output is `bin/GraalShop.html` with its `.js`, `.wasm`, and preloaded asset data next to it. Serve that folder over HTTP; the editor fetches `levels/` from the same hosted directory, so `bin/levels` is used directly without rebuilding the page. It opens `levels/ganis/idle.gani` from that folder automatically.

## Test

```sh
ctest --test-dir build -C Release --output-on-failure
```

## Run

```sh
./bin/GraalShop [--game <folder>] [--direction 0..3] [--frame <n>] [document.gani | folder]
```

A folder argument opens `levels/ganis/idle.gani` from that folder. The window is the editor:

- toolbar: new, open, save, save as, undo, redo, add a sprite from an image, and the Movie Mode switch
- left panel: every sprite in the document; click to select, drag onto the canvas to place it, double-click to add it; the wheel or the scrollbar scrolls the list
- right panel: direction and selected-sprite dropdowns, sprite position buttons, and the document defaults; `+` and `-` add or remove the sprite in the frame, the arrows nudge it, and clicking a default row chooses that image
- canvas: the current frame of the current direction; drag a sprite to move it in that frame
- bottom bar: play/pause, stop, previous/next frame, insert/duplicate/delete frame, save
- Movie Mode: confirms, then replaces the document with a movie document and shows its attribute panel; drag sprites from the strip to add actors

Keys: `1`-`4` choose the direction, Left/Right step frames, Space toggles playback, `+`/`-` change zoom, `Ctrl+S` writes the document back to its `.gani` path (asking for a path when the document is new), `Ctrl+Shift+N` starts a new document, Escape closes the window. Editing: double-click a list entry to add that sprite to the current frame, `Delete` removes the selected sprite from the frame, `Ctrl+D` duplicates the frame, `Ctrl+N` inserts a new frame, `Ctrl+Delete` deletes the frame, `Ctrl+Z` undoes the last edit. `Open` loads another document; `Add` appends a sprite from an image file.

GANI0002 movie documents open in the same window: the right panel lists the actors, the canvas shows each actor at the current movie frame, `Ctrl+arrows` moves the selected actor's keyframe, `Ctrl+K` adds a keyframe at the current frame, `Ctrl+T` adds a text actor, `Enter` edits the selected text actor, `Delete` removes the actor, and double-clicking a sprite adds a `SPRITE` actor at the current frame.

Additional modes:

```sh
./bin/GraalShop --check <document.gani>...
./bin/GraalShop --render <out.bmp> [--direction <n>] [--frame <n>] <document.gani>
./bin/GraalShop --shot <out.bmp> [--add-sprite <image>] [--save] [--game <folder>] <document.gani | folder>
```

`--check` validates documents and prints a per-file summary. `--render` writes one frame without opening a window. `--shot` draws the editor once into a BMP, optionally after adding a sprite from an image file. `--save` writes the document back to its path.

Sprite sheets are resolved from the document's folder, then `levels/heads`, `levels/bodies`, `levels/hats`, `levels/shields`, `levels/swords`, `levels/images/downloads`, and `levels`.

## Layout

- `include/gani/gani.hpp` — document, sprite, defaults, attachment, frame, and movie types
- `src/gani_parser.cpp` — text parser
- `include/app/sheets.hpp`, `src/sheets.cpp` — sprite sheet resolution, body palette, and texture cache
- `include/app/canvas.hpp`, `src/canvas.cpp` — frame compositing
- `include/app/editor.hpp`, `src/editor.cpp` — window layout, sprite strip, canvas, property panel, timeline, and input
- `include/app/font.hpp`, `src/font.cpp` — bitmap text; Windows uses the system MS Sans Serif, other platforms use the bundled DejaVu Sans
- `assets/` — application icon and bundled fonts; CMake copies the folder next to the executable
- `src/main.cpp` — SDL3 window, playback, and command line modes
- `tests/gani_tests.cpp` — model and parser tests
