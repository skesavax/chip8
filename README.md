## How to Run CHIP-8
1. Install SDL2 using MinGW: download SDL2-devel-2.30.8-mingw.tar.
2. Create a src folder in the repository and copy the include and lib folders from the SDL2 package into it.
3. Copy SDL2.dll to the root of the repository.
4. Build and run the emulator using the following commands:
```
$ make clean
$ make all
$ ./chip8.exe ./ROM/ibm_logo.ch8
OR
$ ./chip8.exe ./ROM/Pong.ch8
```
![Ping Pong Game](<Screenshot 2025-12-31 215714.png>)

![IBM logo](<Screenshot 2025-12-31 215629.png>)

## Introduction:
This document is written to describe the work done while building a CHIP-8 emulator. CHIP-8 is a simple interpreter that reads each instruction and executes it without using a real CPU or machine code. It consists of components such as memory, graphics, keyboard, timers, and a memory map, which are sufficient to create games or display graphics. This document focuses on the implementation details of the CHIP-8 interpreter.

## System specifications:
```
| Component | Size  | Description   |
| --------- | ----- | ------------- |
| Memory    |  4KB  | System memory |
| Timers    |  8BIT | Delay & Sound |
| Registers |  8BIT | V0–VF(GPReg)  |
| Display   | 64×32 | Monochrome    |
| Stack     | 16BIT | 16 entries    |
| I         | 16BIT | Index register|
| PC        | 16BIT | Programcounter|
| SP        | 16BIT | Stack pointer |
| Buffer    | 2KB   | Frame buffer  |
```
## Memory map:
CHIP-8 uses a total of 4 KB of memory. In this memory layout, the addresses from 0x000 to 0x1FF are reserved for the interpreter.

CHIP-8 draws graphics on the screen using sprites. Each sprite is a group of bytes that represents the desired image. A font sprite is 5 bytes long and represents an 8×5 pixel character. Therefore, 80 bytes (16 fonts × 5 bytes) are required to store all font sprites.

These fonts are stored in the memory region from 0x000 to 0x1FF, allowing the index register (I) to access them.

Example for sprite '0' 8x5 pixel:
```
"0"	  Binary	Hex
****      11110000      0xF0
*  *      10010000      0x90
*  *      10010000      0x90
*  *      10010000      0x90
****      11110000      0xF0
```
ROM data is stored in memory starting at address 0x200 and continues up to the size of the ROM file.

## Project structure:
Repository layout:
```
chip8/
│   ├── chip8.c     # Emulator loop
│   ├── SDL2.dll    # SDL2 binary
│   ├── cpu.c       # Opcode execution
│   ├── memory.c    # RAM & ROM loading
│   ├── display.c   # SDL rendering
│   ├── input.c     # Keypad mapping
│   ├── timer.c     # Delay & sound timers
│   ├── main.c      # precheck input params
│   └── debugger.c  # Pause/Resume and dump registers
```
Keep these files separate, as this makes the code easier to maintain and modify for future use:
* main.c:
Handles pre-checks and parses input parameters before passing control to chip8.c.
* chip8.c:
Initializes the display, sound, memory, and timers before entering the main loop. Inside the loop, it handles input processing, CPU execution, display updates, and timer management.
* display.c:
Uses the SDL library to configure the window, rendering context, and to display pixels on the screen.
* sound.c:
Sets up sound configuration using the SDL library and supports features such as volume control, mute, and unmute.
* timer.c
Used for initializing and updating the delay and sound timers.
* memory.c
Sets up video memory used to draw sprites on the screen.
* cpu.c
Initializes CPU registers (I, PC, and SP) and performs the fetch, decode, and execute operations.
* input.c
Polls input events using SDL and handles keypad digit detection.

## CPU core design:

struct Cpu {
    uint16_t i; //index register
    uint16_t pc; //program counter
    uint16_t sp; //stack pointer
    uint16_t stack[STACK_SIZE]; //stack memory
    uint8_t v[V_REG_COUNT];//General purpose registers v[x]x:0toF

    Memory memory; //Fetch opcode using I register
    Timer timer;   //Set delay timer
    VMemory vmemory; //Draw sprites and status
};

The CPU structure contains an index register (I), a program counter (PC), a stack pointer (SP), a stack with 16 entries, and general-purpose registers (V0–VF). It also holds references to the memory, timer, and video memory objects.

All memory, timer, and video memory components should already be initialized. The CPU index register (I) is initialized to the 0x0000 memory location in RAM, the program counter (PC) is set to PROGRAM_START (0x200), and the stack pointer (SP) is set to zero. Both the stack and the general-purpose register buffers are cleared to zero.

The CPU must always run in a loop unless a debugger break is set. In each iteration, it performs the fetch, decode, and execute operations.

Fetch:
Fetch two 8-bit values from RAM using the program counter (PC) and combine them to form a single 16-bit big-endian instruction, known as an opcode.

Decode:
Decode the opcode by extracting the following fields:
n: 1st nibble of the lower byte
y: 2nd nibble of the lower byte
x: 3rd nibble of the lower byte
kk: 8-bit immediate value from the lower byte
nnn: Address formed by the lower 12 bits of the opcode

Execute:
Using the decoded fields (n, y, x, kk, nnn), identify the instruction type and execute the corresponding operation.
Note:
Not all instructions are discussed here. Only those that require special attention are documented.

Please refer to the reference links below for details on all 34 CHIP-8 instructions. The following section highlights only the instructions that require special attention.

1. 00E0 – CLS: Clear the display
Clears the video buffer and sets the draw_flag to true.

2. 00EE – RET: Return from a subroutine
CHIP-8 uses a post-incrementing stack. Decrement the stack pointer (SP) by 1 and update the program counter (PC) with the value stored at STACK[SP].

3. 1nnn – JP addr: Jump to address nnn
Sets the program counter (PC) to nnn.

4. 2nnn – CALL addr: Call subroutine at address nnn
Saves the current PC to the stack using STACK[SP] = PC, increments the stack pointer (SP) by 1, and then updates the PC to nnn.

5. 7xkk – ADD Vx, byte: Set Vx = Vx + kk
Overflow may occur (e.g., 250 + 10 = 260 → 260 mod 256 = 4), which is expected and intentional. The carry flag (VF) is not affected.

6. Cxkk – RND Vx, byte: Set Vx = random byte AND kk
Uses a random number generator, for example:
(uint8_t)(rand() % 256).

7. Dxyn – DRW Vx, Vy, nibble: Draw sprite
Displays an n-byte sprite starting at memory location I at coordinates (Vx, Vy) and sets VF if a collision occurs.

The index register I is used as the sprite start address, n represents the sprite height, and Vx and Vy define the x and y positions for drawing the sprite onto the video buffer (vbuffer).
Refer to Section 1.1: Memory for more details on sprite rendering.

8. Ex9E – SKP Vx: Skip next instruction if key with value Vx is pressed
Checks the input keypad array (input[16]) using Vx as the index. If the corresponding key is pressed (value is 1), increment the PC by 2.

9. Fx07 – LD Vx, DT: Set Vx = delay timer value
Updates Vx with the current value of the delay timer.

10. Fx33 – LD B, Vx: Store BCD representation of Vx
Stores the binary-coded decimal (BCD) representation of Vx in memory locations I, I+1, and I+2. A temporary variable is used to extract and store the hundreds (temp / 100), tens ((temp / 10) % 10), and ones (temp % 10) digits.

If an unknown instruction is detected, set the 'unrecognized' flag to 1, log the error to standard error, and return -1. In this error case, updating the video memory buffer is skipped; however, it does not affect overall CPU execution.

## Display system SDL:

typedef struct {
    SDL_Window  *window;
    SDL_Renderer* renderer;
    SDL_Color primary_color;
    SDL_Color secondary_color;
    uint32_t scale;       //user scale input
    uint8_t *draw_pixels; //frame buffer
    
} DisplayHandler;

SDL display initialization:
typedef struct {
    uint8_t pr, pg, pb; /* primary color */
    uint8_t sr, sg, sb; /* secondary color */
} ColorTheme;

#define DEFAULT_SCALE 10U
#define DEFAULT_THEME ((ColorTheme){255, 255, 255, 0, 0, 0})

Black and white are chosen as the default theme. CHIP-8 is a monochrome system, but the host display is not. Therefore, the primary color is used for pixel ON and the secondary color is used for pixel OFF.

Library functions from the SDL library:
1. SDL_InitSubSystem
Initializes the SDL library. The SDL_INIT_VIDEO flag is used to initialize the video subsystem.
This function returns 0 on success and a negative value on failure.
On failure, an error message describing the specific issue can be retrieved.

2. SDL_CreateWindow
Creates a window with the specified title, position, dimensions, and flags.
Title: "Welcome to Chip8 Emulator"
x: X-position of the window, set to SDL_WINDOWPOS_CENTERED
y: Y-position of the window, set to SDL_WINDOWPOS_CENTERED
w: Width of the window (can be scaled up or down)
h: Height of the window (can be scaled up or down)
flags:
SDL_WINDOW_SHOWN — makes the window visible
SDL_WINDOW_RESIZABLE — allows the window to be resized

This function returns a pointer to an SDL_Window object on success, or NULL on failure.

What is a Renderer?
Rendering is the process of converting data into visible pixels on the screen.

3. SDL_CreateRenderer
Creates a 2D rendering context for a window.
SDL_Window: Pass the object returned by SDL_CreateWindow
index: Set to -1 to initialize the first renderer that supports the requested flags
flags:
SDL_RENDERER_ACCELERATED — uses hardware acceleration
SDL_RENDERER_PRESENTVSYNC — presentation is synchronized with the display refresh rate

This function returns a valid rendering context on success, or NULL if an error occurs.

4. SDL_DestroyWindow
Destroys a window. This is typically used in error scenarios when SDL_CreateRenderer fails.
SDL_Window: Pass the object returned by SDL_CreateWindow

5. SDL_SetRenderDrawColor
Sets the color used for drawing operations such as rectangles, lines, and clearing the render target.
SDL_Renderer: Renderer object returned by SDL_CreateRenderer
r: Red component used for drawing on the render target (secondary color)
g: Green component used for drawing on the render target (secondary color)
b: Blue component used for drawing on the render target (secondary color)
a: Alpha value, default is SDL_ALPHA_OPAQUE (255)

6. SDL_RenderClear
Clears the current rendering target using the currently set drawing color.
SDL_Renderer: Renderer object returned by SDL_CreateRenderer

7. SDL_RenderPresent
Updates the screen with all rendering operations performed since the previous call.
SDL_Renderer: Renderer object returned by SDL_CreateRenderer

## SDL drawing pixel:
int display_draw(DisplayHandler *dh);

Converts the CHIP-8 framebuffer (draw_pixels) into visible pixels on the screen using the SDL renderer.

It uses three SDL calls:
* SDL_SetRenderDrawColor:
Sets the background color (secondary color by default).
* SDL_RenderClear:
Clears the screen using the currently set background color.
* SDL_SetRenderDrawColor:
Sets the foreground (primary) color used to draw active pixels.

CHIP-8 redraws the entire screen every frame; therefore, pixels from the previous frame must be cleared before drawing the new frame.

```
Set background color
        ↓
Clear screen (fill background)
        ↓
Set foreground color
        ↓
Draw pixels (rectangles)
        ↓
Present frame
```
Pixels can be drawn by scanning the framebuffer pixel by pixel. For every pixel that is ON, draw a scaled rectangle using 'SDL_RenderFillRect'.

Important: SDL_RenderPresent(dh->renderer) is the final and mandatory step of rendering. Without this call, nothing will appear on the screen, even if all drawing operations were performed correctly.

void display_shutdown(DisplayHandler *dh)
Cleans up and correctly releases SDL display resources.

Steps performed:
* SDL_DestroyRenderer → destroys the renderer.
* SDL_DestroyWindow → destroys the associated window.

Notes:
* The renderer must be explicitly destroyed first to avoid memory leaks.
* Since the renderer is attached to the window, destroying the window before the renderer could invalidate it.
* 'SDL_Quit' is not called in this function; it is typically called when exiting the emulator.


## Frame Buffer update (Video Buffer):

uint8_t vmemory_draw_sprite_no_wrap(VMemory *vm, uint8_t x_pos, uint8_t y_pos, const uint8_t *sprite, int sprite_height)

This function is called when the DRW instruction is executed. Refer to Section X for more details on the DRW instruction.
The function performs XOR drawing: if any pixel is turned off during the drawing process, the collision flag VF is set to 1. By default, VF is initialized to 0.
VF is the collision detection flag used for updating the V[F] register.

Algorithm for Drawing a Sprite

1. Set draw_flag to true. This flag is later used by cpu.c to update the screen.
2. Normalize the x and y positions to the 64×32 display area to ensure safe screen indices (norm_x, norm_y).
3. Initialize the collision flag VF to 0 and set cur_y to norm_y.
4. Iterate over each row of the sprite up to its height (sprite_height):
   4.1 If cur_y exceeds the screen height (32), stop processing. Set cur_x to norm_x.
   4.2 Iterate over each column of the row (8 pixels per sprite row):
       4.2.1 Scan each bit from the leftmost pixel (MSB) in the sprite buffer.
       4.2.2 XOR the bit with the corresponding frame buffer bit vm[IDX[cur_x, cur_y]] and store the result back in the frame buffer.
       4.2.3 If a pixel is turned off during XOR, set the collision flag VF to 1. Once set, it is not reset in this loop.
       4.2.4 Move to the next pixel (cur_x++) and continue iterating columns.
   4.3 Move to the next row (cur_y++) and continue iterating rows.
5. After completing the drawing, both the collision flag VF and the frame buffer vm are updated.
6. Return the collision flag VF.

## Input Handling

Structures Used in input.c
* Quit Event: Used to exit the emulator. This event is triggered when the SDL event SDL_QUIT is detected or a specific key is pressed.
* Restart Event: Refreshes the timers and restarts the emulator.

CHIP-8 uses a 16-key hexadecimal keypad, which is mapped to the keys 0–F as follows:
```
1 2 3 4                   1 2 3 C
Q W E R => (mapped to) => 4 5 6 D
A S D F                   7 8 9 E
Z X C V                   A 0 B F
```
These are an additional 5 keys used for debugging purposes. They will be discussed in detail in Section 'Debugger'.
O => debug_paused
U => debug_resume
I => debug_step
B => debug_break
N => debug_clear_break

The key SDL_SCANCODE_ESCAPE is used to trigger the Quit event, and SDL_SCANCODE_SPACE is used to trigger the Restart event.
```
typedef struct {//@TODO: make it bool
    int quit;               /* bool */
    int restart;            /* bool */
    uint8_t keypad[16];     /* keypad state (0/1) */

    /* Debugger commands */
    int dbg_pause;
    int dbg_resume;
    int dbg_step;
    int dbg_break;
    int dbg_clear_break;
} InputEvent;

typedef struct {
    SDL_Event event;
    InputEvent ev;
} InputHandler;
```
These keypad input values are also used by certain instructions, such as SKP Vx, SKNP Vx, and LD Vx, K.

## Timers
CHIP-8 has two timers: the delay timer and the sound timer. Both timers are implemented and initialized to zero during system startup.

CHIP-8 provides instructions to set these timers:
* LD DT, Vx — sets the delay timer from register Vx
* LD ST, Vx — sets the sound timer from register Vx
By default, both timers are zero. Until the user explicitly sets them, the timers remain inactive.

Both timers decrement at a rate of 60 Hz until they reach zero. The sound timer activates a beep sound while its value is greater than zero, allowing games to generate sound for short intervals.

## Emulator Flow Chart
Below is the flow chart for Emulator.
The emulator has three main loops:
1. Initialization Loop:
Initializes memory, timers, video memory (vmemory), and CPU registers.
2. Timer Loop:
Processes the delay and sound timers, manages frequency, and handles T0 period bookkeeping.
3. CPU/Input Loop:
Handles input polling, fetch-decode-execute cycles, debugging, display updates, and CPU speed control.

### Timing and CPU Configuration:
* The CHIP-8 clock is configured at 60 Hz (16.67 ms per cycle).
* If a timer exceeds 16.67 ms, the inner CPU/Input loop breaks, and processing resumes from the Timer Loop.
* The default CPU executes 600 instructions per second. If the CPU runs too fast, delays are introduced to slow it down.

### Event Handling:
* Debug Event: When detected, fetch-decode-execute cycles are paused.
* Quit Event: Sets the Running flag to False, exiting all three loops and freeing memory, display, and SDL resources.
* Restart Event: Exits only the inner CPU/Input loop to refresh timers and restart the emulator without full shutdown.
### Flow chart of CHIP8 Emulator:
```
                                        +----------------------+
                                        |   Start Emulator     |
                                        +----------+-----------+
                                                |
                                                v
                                        +----------------------+
                                        |  Init SDL Systems    |
                                        |  (Video / Audio /    |
                                        |   Timer)             |
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        | Init Display/Input/  |
                                        | Sound Handlers       |
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        |   Load ROM from      |
                                        |   File into Memory   |
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        | Loop 1:              |
                                        | Init CHIP-8 State    |
                                        | - Memory (4KB)       |
                             (3)------->| - CPU                |
                                        | - Timers             |
                                        | - Video Memory       |
                                        | - Debugger           |
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        |     Running == True  |
                                        +----+-------------+---+
                                        |                      |
                                        | YES                  | NO
                                        V                      V
                                        +----------------+  +------------------+
                                        | Loop:2         |  |                  |
                               (2)----->| Update Timers  |  | Cleanup & Exit   |
                                        | (60 Hz)        |  | - Free ROM Memory|
                                        +-------+--------+  | - Shutdown SDL   |
                                                |           +------------------+
                                                V
                                        +----------------------+
                                        | Sound Control        |
                                        | (Beep if timer > 0) |
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        | Loop:3               |->[Quit]->[Running=False]->(go to 3)
                                        |Poll Input Events:    |
                                (1)---->|  - Quit / Restart    |
                                        |  - Debug Commands    |->[Restart]-->[break]-->(go to 2)
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        | Debugger Allows      |
                                        | can CPU Execute ?    |
                                        +----+--------------+--+
                                        | YES                  | NO
                                        V                      |
                                        +------------------+   |
                                        | Execute CPU      |   |
                                        | Cycle            |   |
                                        | - Fetch Opcode   |   |
                                        | - Decode         |   |
                                        | - Execute        |   |
                                        +--------+---------+   |
                                                |              |
                                                V              V
                                        +----------------------+ 
                                        | Update Display SDL   |
                                        | (if draw_pixels)     |
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        | CPU Speed Control    |
                                        | Sleep if too fast    |
                                        | (~600 Hz)            |
                                        +----------+-----------+
                                                |
                                                V
                                        +----------------------+
                                        | Timer Tick Reached ? |
                                        | (~16.7 ms)           |
                                        +----+-------------+---+
                                        | YES                 | NO
                                        |                     |
                                        V                     V
                                       (go to 2)              (go to 1)            
```

## Debugger
A few debug variables have been added to the InputEvent structure to support pause, resume, step, and break operations.
These debug events are not part of the original CHIP-8 specification; a separate set of keys is used to enable these debug operations.

Debug Operations
1. Pause: Prevents the CPU from executing fetch-decode-execute operations. Enabled by setting the dbg_pause flag.
2. Resume: Clears the dbg_pause flag, allowing the CPU to continue fetch-decode-execute operations.
3. Step: Sets both dbg_pause and dbg_step flags. This allows printing the current PC register to the console. The CPU does not continue execution because dbg_pause is set, but dbg_step is immediately cleared after a single 5. instruction.
4. Break: Pauses the CPU at the current PC using the dbg_pause flag. The CPU remains paused until Resume is used.A break can occur again if execution reaches a previously set break point.
5. Break Flag (dbg_break): Stores the current PC for the break. It is only cleared using the Clear command.
6. Clear: Resets dbg_break to zero, removing any breakpoints so the CPU will not pause at any PC.
```
bool debugger_should_execute(Cpu *c);
```
This function determines whether the CPU should run or remain paused. It uses the debug variables Break, Step, and Pause, and it also prints all debug data, including the PC, I, the 16 general-purpose registers, and the delay and sound timer values.

## References
1. https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
2. http://devernay.free.fr/hacks/chip8/C8TECH10.HTM