## Introduction:
This document is written to describe the work done when building a emulator CHIP-8, CHIP-8 is a simple interpreter that reads each instruction
and execute it with out the real CPU or Machine code. It has components like Memory, Graphics, keyboard, timers, Memory map that is enough to make 
games or display graphics. Following document will focus on the implementation details of CHIP-8 interpreter.

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
CHIP-8 uses total 4KB of memory, In this memory layout first 0x00 to 0x1FF are reserved for Interpreter. 
CHIP-8 draw graphic on the screen using sprite, each sprite is a group of bytes represent desire picture it is 5 bytes long for 8x5 pixels to represent
a font. so this requires 80bytes(16 fonts*5bytes) to store all sprints. These fonts are stored in memory region 0x00 to 0x1FF so the Index register(I)
can access it.

Example for sprite '0' 8x5 pixcel:
```
"0"	  Binary	Hex
****      11110000      0xF0
*  *      10010000      0x90
*  *      10010000      0x90
*  *      10010000      0x90
****      11110000      0xF0
```
ROM data are stored at address location starting at 0x200 to ROM file size.

## Project structure:
Repo layout:
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
Keep these files seperate as it is easy to maintain and change for future uses:

main.c handels precheck and parse input params before passing it to chip8.c
chip8.c initialize display, sound, memory, timer before entering into loop, inside it handles input, run cpu, display and timer management
display.c It uses SDL library to configure window, rendering and display pixcels on screen
sound.c It setup sound configuration using SDL libary, it support feature like volume, mute and unmute.
timer.c used for initialization and update delay and sound timers
memory.c Setup video memory to draw sprites on screen
cpu.c Initialize cpu registers I,PC, and SP and process fetch, decode and execute operations
input.c pool for input event using SDL used for keypad digit detection


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

CPU structure contain Index register (i), program counter (pc), stack pointer (sp), stack of 16 entries and general purpose registers (vx-vf).
It also holds Memory, timer and vmemory objects.

All the memory, timer and vmemory should already be initialized. CPU Index register(I) is initialized to 0x0000 memory location of RAM, 
program counter is set to PROGRAM_START(0x200) and stack pointer (SP) to zero, both stack and GPRs buffers are cleared to zero.

CPU must alway running in loop unless debugger break is set, it do fetch, decode and execute operations.
Fetch : Get 2 8bit instruction from RAM memory using PC, append it to form 16 bit big-endian instruction this is called opcode.
Decode : Extract opcode to following nibbles
n: 1st nibble from lower byte
y: 2nd nibble from lower byte
x: 3rd nibble from lower byte
kk: 8 bits from lower byte
nnn: 1st, 2nd and 3rd nibble from lower byte

Execute: Used the above (n, y, x, kk, nnn) Identify the type of instruction and execute:
NOTE: Not all instructions are discussed here, only those that need attention are documented. 
Please refer to all 34 instruction details provided in reference links below:

1.00E0 - CLS: Clear the display
clear vbuffer and set flag 'draw_flag' to true

2.00EE - RET: Return from a subroutine
CHIP-8 uses post incremental stack, decrement SP by 1 and update PC to STACK[SP]

3.1nnn - JP addr: Jump to the address given by nnn
Set PC to nnn

4.2nnn - CALL addr: call subroutine at address nnn
Save PC by doing STACK[SP] =PC, and increment SP by 1 and update PC to nnn

5.7xkk - ADD Vx, byte: Set Vx = Vx + kk.
overflow can happen 250+10=256 mod 256=4, but its intentional

6.Cxkk - RND Vx, byte: Set Vx = random byte AND kk.
used random number generator (uint8_t)(rand() % 256)

7.Dxyn - DRW Vx, Vy, nibble: Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
use I as start, n as sprite_height, v[x] and v[y] as x and y position to draw sprite on to vbuffer.
Go through session 1.1 Memory to understand more about the drawing sprite on to vbuffer.

8.Ex9E - SKP Vx: Skip next instruction if key with the value of Vx is pressed.
Check the input keypad(input[16]) with v[x] as index, if the keypad value set to 1 then increment PC+=2

9.Fx07 - LD Vx, DT: Set Vx = delay timer value.
v[x] is updated with kk(byte)

10.Fx33 - LD B, Vx: Store BCD representation of Vx in memory locations I, I+1, and I+2.
store vx in temp variable and extract and store 100th (temp/100), 10th ((temp/10)%10) and 1th (temp%10) variable to memory pointing to I register.


If any unknown instruction is detected set flag 'unrecognized' to 1 and log the standard error and return -1. In this error case it avoid
updating vmemory buffer but it doesn't affect the cpu execution.

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
Black and white is choosen as default theme. CHIP-8 is monochrome, but the host display is not, 
so we use primary colour for pixel ON and secondary color for pixel OFF


library functions from SDL libary:
1. SDL_InitSubSystem: 
-Initialize the SDL library, used flag SDL_INIT_VIDEO to initialize video subsystem.
-This function return 0 on success and -ve error code on failure. Error code returns a message with information 
about the specific error that occurred.

2. SDL_CreateWindow
Create a window with the specified position, dimensions, and flags.
title: title of the window set to "Welcome to Chip8 Emulator"
x: x-position of window, SDL_WINDOWPOS_CENTERED
y: y-position of window, SDL_WINDOWPOS_CENTERED
w: width can scaled to higher or lower
h: height can scaled to higher or lower
flags: SDL_WINDOW_SHOWN (window is visible)| SDL_WINDOW_RESIZABLE (window can be resized)
-This function return object of SDL_Window, otherwise NULL on failure.

What is a Render?
Rendering is the process of converting data into visible pixels on the screen.

3. SDL_CreateRenderer
Create a 2D rendering context for a window.
SDL_Window: Pass output object of SDL_CreateWindow
index: -1 to initialize the first one supporting the requested flags.
flags: SDL_RENDERER_ACCELERATED (The renderer uses hardware acceleration)| (SDL_RENDERER_PRESENTVSYNC) Present is synchronized with the refresh rate
-This function returns a valid rendering context or NULL if there was an error

4. SDL_DestroyWindow
Destroy a window. used for error senario when SDL_CreateRenderer failed.
SDL_Window: pass the object returned by SDL_CreateWindow

5. SDL_SetRenderDrawColor
Set the color used for drawing operations (Rect, Line and Clear).
SDL_Renderer: render obj returned by SDL_CreateRenderer
r: the red value used to draw on the rendering target. secondary colour
g: the green value used to draw on the rendering target. secondary colour
b: the blue value used to draw on the rendering target. secondary colour
a: aplha value default SDL_ALPHA_OPAQUE (255)

6. SDL_RenderClear
Clear the current rendering target with the drawing color.
SDL_Renderer : render obj returned by SDL_CreateRenderer

7. SDL_RenderPresent
Update the screen with any rendering performed since the previous call.
SDL_Renderer: pass the object returned by SDL_CreateWindow

SDL drawing pixel:

int display_draw(DisplayHandler *dh);
Convert CHIP-8 framebuffer (draw_pixels) into visible pixcels on the screen using SDL render.

In uses 3 SDL calls:
SDL_SetRenderDrawColor : set background colour (back default), secondary colours
SDL_RenderClear : when clearing the screen, use this colour
SDL_SetRenderDrawColor :set background colour, primary colours

CHIP-8 redrew the screen every frame, old pixcels must be erased.
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
Draw pixcel, it can be done by scanning frambuffer pixcel by pixcel, for every pixcel that is ON draw scaled rectangle using SDL_RenderFillRect.
SDL_RenderPresent(dh->renderer) is the final and mandatory step of rendering. Without it, nothing appears on the screen, 
even though you “draw” everything correctly


void display_shutdown(DisplayHandler *dh)
SDL_DestroyRenderer->SDL_DestroyWindow

This function is about clean and correct resource teardown.
Safely releases SDL display resources
render must be explicitly destroyed to avoid leaks
Renderer is attached to the window, Destroying the window first could invalidate the renderer

SDL_Quit is not called in display file, it is used wile exiting emulator.


## Frame Buffer update (Video Memory):
uint8_t vmemory_draw_sprite_no_wrap(VMemory *vm, uint8_t x_pos, uint8_t y_pos, const uint8_t *sprite, int sprite_height);

This function is called when DRW instruction executed. Read session x for more details on DRW instruction.

This function do xor drawing, if any pixcel is turned off during drawing, set VF=1 by default VF=0.
VF is collection detection flag used in updating V[F] register. Following details provide
algorithm:

1. Set 'draw_flag' to True, this will later used by cpu.c to update screen.
2. Normalize x and y position to 64x32 for safe screen indices, norm_x, norm_y
3. Set colliction flag 'vf' to zero, set cur_y to norm_y
4. Iterate each row till horizon size of the sprite 'sprite_height':
        4.1 If cur_y size is beyond 32 screen height, stop processing. Set cur_x to norm_x
        4.2 Iterate col till 8 pixcels (1 sprite row), set curr_x = normalized(x)
                4.2.1 Start scanning each bit from left most pixcel (MSB) in sprite buffer
                4.2.2 XOR the bits in 4.2.1 with frame buffer bit vm[IDX[x,y]] and store it back to frame buffer
                4.2.3 Set VF flag, once set it will never reset back in this loop
                4.2.4 Move to next pixcel curr_x++, continue 4.2
        4.3 Move to next row curr_y++, continue 5
5. Both VF flag and frame buffer 'vm' are updated
6. return VF flag

## Input Handling

Following are two structures used in input.c file:
Quit event is to exit the emulator, it is set when SDL event 'SDL_QUIT' is detected with key press.
Restart event will refresh sounder timer and restart.

CHIP-8 uses 16 digit key pad, it is mapped as 0-F key digits:
```
1 2 3 4                   1 2 3 C
Q W E R => (mapped to) => 4 5 6 D
A S D F                   7 8 9 E
Z X C V                   A 0 B F
```
These are additional 5 digits used for debugging purpose, this will discuss in detail in session x
O => debug_paused
U => debug_resume
I => debug_step
B => debug_break
N => debug_clear_break

SDL_SCANCODE_ESCAPE is used for Quit and SDL_SCANCODE_SPACE is used as Restart.

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

These input keypad values are also used in some instruction executions. SKP Vx, SKNP Vx and LD Vx, K.

## Timers
CHIP-8 has two timers delay and sound timer, both timer are implemented. During initialization
both timers are set to zero value.

CHIP-8 provide instructions to set both delay and sound timer using: LD DT, Vx and LD ST, Vx. By default these timers are set with zero value, so untill user set it explictly these timer would be active.

Both timers will keep decrement to zero, sound timer will set beep sound to True till
its value reach zero. This help the game to create sound for few ticks intervals.

## Main Emulation Loop
Below is the flow chart for Emulator.
It has 3 loop:
1. First loop is to initialize Memory, timers, vmemory, cpu registers
2. Second loop is to process sound and delay timers, frequency and T0 period(book keeping).
3. Third loop is pool for input events, Fetch, decode, execute, debug processing, display and cpu speed control.
CHIP-8 clock is configured at 60HZ, 16.67msec. Clock should reset when timer reach more than 16.67 msec, in this case inner 3rd loop will break and process should continue from Second loop.
Default CPU cycles 600 instructions per sec, if CPU run too fast then slow down is handled by giving delay.
When debug input event is detected, CPU call for Fetch, decode and execute will not processed.
When Quit input event is detected, 'Running' flag is set to False this will exit the all three loops
and freeing up memory, display and exit SDL. Where as Restart input event will only exit from innter loop 

# Flow chart of CHIP8 Emulator:
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
Few Debug variables are added to InputEvent structure, it do pause, resume, step, break opeations.
Those these events are not part of CHIP-8, a new set of digits are used to enable these debug
operations.

Following operations are dicussed briefly:
1. Pause: It don't allow CPU to execute fetch, decode and execute operation. This is enabled by setting 'dbg_pause' flag.
2. Resume: It clear the previously set dbg_pause flag so CPU can continue fetch, decode and execute operations
3. Step: It set both flags 'dbg_pause' and 'dbg_step', it allow to print current PC register on console also it not allow CPU to continue with fetch,decode and execute becausd dbg_pause is set, but 'dbg_step' will immediately cleared.
4. Break: It breaks at current PC, it uses 'dbg_pause' not allowing to CPU to run. Resume can be used to clear 'dbg_pause' but, break can happen if the PC previously locked for break is appeared again.
5. This break flag 'dbg_break' is updated with current PC, and it will only cleared by using clear
command.
6. Clear: It cleare the dbg_break to zero, so it allow CPU not to hit break point at any PC.

bool debugger_should_execute(Cpu *c);
This function will deside whether CPU can run or paused. It used above debug variable like
Break, Step and Pause and also it print all debug data like PC, I, 16 GP registers and delay and
sound timer data.

## References
1. https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
2. http://devernay.free.fr/hacks/chip8/C8TECH10.HTM