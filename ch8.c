#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_video.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

/* Memory size */
#define MEMB 4096
#define WINDOW_W 640 /* For SDL */
#define WINDOW_H 320 /* For SDL */
#define SCREEN_W 64
#define SCREEN_H 32
#define STACK_SIZE 255
#define START_ADDR 0x200
#define END_ADDR 0xFFF
#define ROM_MAX (0xFFF - 0x200)
#define FNT_START 0x50
#define FPS 60
#define FRAMETIME (1000 / FPS)

/* Globals for simplicity */
uint16_t I; /* Index register */
uint16_t call_stack[STACK_SIZE], stackp;
uint8_t registers[16]; /* Registers V0 through VF */
uint8_t screen[SCREEN_W][SCREEN_H] = { 0 };
uint8_t memory[MEMB]; /* 4096 Bytes of ram */
uint8_t keyboard[16]; /* 16 keys */
uint8_t delay_timer;
uint8_t sound_timer;
uint8_t *startptr = memory + START_ADDR; /* the ROM is loaded here */
int pc = START_ADDR;                     /* The program counter */
int last_goto; /* Last goto, useful for not printing inf. loops */
int old_pc;
int draw_flag = 0;   /* whether any drawing is to be done */
int key_pressed = 0; /* value of the last keypress. -1 if no key pressed */
void drawScreen (SDL_Renderer *renderer, uint8_t screen[SCREEN_W][SCREEN_H]);
void processCycle (void);

int
main (int argc, char *argv[])
{

  SDL_Window *gWindow = NULL;
  SDL_Renderer *renderer = NULL;
  FILE *rom;

  uint8_t fnt[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
  };

  /* 0x50-0x9F is a common location for the font  */
  memcpy (startptr + FNT_START, fnt, 80);

  stackp = 0;

  if (argc != 2)
    {
      printf ("USAGE: %s <rom>", argv[0]);
      exit (EXIT_SUCCESS);
    }

  rom = fopen (argv[1], "rb");

  if (!rom)
    {
      printf ("couldn't open file %s!", argv[1]);
      exit (EXIT_FAILURE);
    }
  int romlen = fread (startptr, 1, ROM_MAX, rom);
  fclose (rom);

  SDL_CreateWindowAndRenderer (WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN, &gWindow,
                               &renderer);
  SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);

  int quit = 0;
  SDL_Event e;

  while (!quit)
    {

      uint32_t frame_start = SDL_GetTicks ();

      while (SDL_PollEvent (&e) != 0)
        {
          if (e.type == SDL_QUIT)
            {
              quit = 1;
            }
          if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
            {
              switch (e.key.keysym.scancode)
                {
                case SDL_SCANCODE_1:
                  keyboard[0] = !keyboard[0];
                  key_pressed = 14;
                  break;
                case SDL_SCANCODE_2:
                  keyboard[1] = !keyboard[1];
                  key_pressed = 1;
                  break;
                case SDL_SCANCODE_3:
                  keyboard[2] = !keyboard[1];
                  key_pressed = 2;
                  break;
                case SDL_SCANCODE_4:
                  keyboard[3] = !keyboard[3];
                  key_pressed = 3;
                  break;
                case SDL_SCANCODE_Q:
                  keyboard[4] = !keyboard[4];
                  key_pressed = 4;
                  break;
                case SDL_SCANCODE_W:
                  keyboard[5] = !keyboard[5];
                  key_pressed = 5;
                  break;
                case SDL_SCANCODE_E:
                  keyboard[6] = !keyboard[6];
                  key_pressed = 6;
                  break;
                case SDL_SCANCODE_R:
                  keyboard[7] = !keyboard[7];
                  key_pressed = 7;
                  break;
                case SDL_SCANCODE_A:
                  keyboard[8] = !keyboard[8];
                  key_pressed = 8;
                  break;
                case SDL_SCANCODE_S:
                  keyboard[9] = !keyboard[9];
                  key_pressed = 9;
                  break;
                case SDL_SCANCODE_D:
                  keyboard[10] = !keyboard[10];
                  key_pressed = 10;
                  break;
                case SDL_SCANCODE_F:
                  keyboard[11] = !keyboard[11];
                  key_pressed = 11;
                  break;
                case SDL_SCANCODE_Z:
                  keyboard[12] = !keyboard[12];
                  key_pressed = 12;
                  break;
                case SDL_SCANCODE_X:
                  keyboard[13] = !keyboard[13];
                  key_pressed = 13;
                  break;
                case SDL_SCANCODE_C:
                  keyboard[14] = !keyboard[14];
                  key_pressed = 14;
                  break;
                case SDL_SCANCODE_V:
                  keyboard[15] = !keyboard[15];
                  key_pressed = 15;
                  break;
                default:
                  break;
                }
              if (e.key.type == SDL_KEYUP)
                key_pressed = -1; /* not pressed if up */
            }
        }
      processCycle ();
      SDL_RenderClear (renderer);
      if (draw_flag)
        {
          drawScreen (renderer, screen);
        }
      SDL_RenderPresent (renderer);

      uint32_t frame_time = SDL_GetTicks () - frame_start;

      if (frame_time < FRAMETIME)
        SDL_Delay (FRAMETIME - frame_time);
      if (pc == (START_ADDR + romlen))
        SDL_Quit ();
    }
}
void
drawScreen (SDL_Renderer *renderer, uint8_t screen[64][32])
{
  for (int x = 0; x < 64; x++)
    {
      for (int y = 0; y < 32; y++)
        {
          SDL_SetRenderDrawColor (
              renderer, screen[x][y] ? 255 : 0, screen[x][y] ? 255 : 0,
              screen[x][y] ? 255 : 0, screen[x][y] ? 255 : 0);
          SDL_Rect rect = { x * 10, y * 10, 10, 10 };
          SDL_RenderFillRect (renderer, &rect);
        }
    }
}

void
processCycle (void)
{
  uint8_t x, y, n, nn, opcode;
  uint16_t nnn, instruction;

  instruction = (memory[pc] << 8) | memory[pc + 1];
  opcode = (instruction & 0xF000) >> 12;
  x = (instruction & 0x0F00) >> 8;
  y = (instruction & 0x00F0) >> 4;
  n = instruction & 0x000F;
  nn = (instruction & 0x00FF);
  nnn = (instruction & 0x0FFF);

  old_pc = pc; /* pc where instruction was actally called */
  pc += 2;

  switch (opcode)
    {
    case 0x0:
      {
        if (nn == 0x00)
          {
            memset (screen, 0, sizeof (screen));
          }
        else if (nn == 0xEE)
          {
            pc = call_stack[--stackp]; /* return  */
          }
      }
      break;
    case 0x1:
      {
        int old = pc;
        pc = nnn;
        last_goto = nnn;
      }
      break;
    case 0x2:
      {
        call_stack[stackp] = pc;
        ++stackp;
        pc = nnn;
      }
      break;
    case 0x3:
      {
        if (registers[x] == nn)
          {
            pc += 2;
          }
      }
      break;
    case 0x4:
      {
        if (registers[x] != nn)
          {
            pc += 2;
          }
      }
      break;
    case 0x5:
      {
        if (registers[x] == registers[y])
          {
            pc += 2;
          }
      }
      break;
    case 0x6:
      {
        registers[x] = nn;
      }
      break;
    case 0x7:
      {
        registers[x] += nn;
      }
      break;
    case 0x8:
      {
        switch (n)
          {
          case 0x0:
            {
              registers[x] = registers[y];
            }
            break;
          case 0x1:
            {
              registers[x] |= registers[y];
            }
            break;
          case 0x2:
            {
              registers[x] &= registers[y];
            }
            break;
          case 0x3:
            {
              registers[x] ^= registers[y];
            }
            break;
          case 0x4:
            {
              uint16_t sum;
              sum = registers[x] += registers[y];
              if (sum > 255)
                registers[0xF] = 1;
              else
                registers[0xF] = 0;
            }
            break;
          case 0x5:
            {
              registers[x] -= registers[y];
            }
            break;
          case 0x6:
            {
              registers[x] = registers[x] >> 1;
            }
            break;
          case 0x7:
            {
              registers[x] = registers[y] - registers[x];
            }
            break;
          case 0xE:
            {
              registers[x] = registers[x] << 1;
            }
            break;
          }
      }
      break;
    case 0x9:
      {
        if (registers[x] != registers[y])
          pc += 2;
      }
      break;
    case 0xA:
      {
        I = nnn;
      }
      break;
    case 0xB:
      {
        pc = nnn + registers[0x0];
      }
      break;
    case 0xC:
      {
        srandom (time (NULL));
        int rand = (random ()) & nn;
        registers[x] = rand;
      }
      break;
    case 0xD:
      {
        uint8_t p_x = registers[x] % 64;
        uint8_t p_y = registers[y] % 32;
        int index = I;

        for (unsigned int row = 0; row < n; ++row)
          {
            uint8_t sprite = memory[index + row];

            for (unsigned char col = 0; col < 8; ++col)
              {
                uint8_t spritepxl = sprite & (0x80u >> col);
                uint8_t *screenpxl = &screen[p_x + col][p_y + row];
                registers[0xF] = *screenpxl; /* 1 if on, 0 if off */
                *screenpxl ^= spritepxl;
              }
          }
        draw_flag = 1;
      }
      break;
    case 0xe:
      {
        switch (nn)
          {
          case 0x9E:
            {
              if (keyboard[x])
                pc += 2;
            }
            break;
          case 0xA1:
            {
              if (!keyboard[x])
                pc += 2;
            }
            break;
          }
      }
      break;
    case 0xF:
      {
        switch (nn) /* second byte */
          {
          case 0x0A:
            {
              if (key_pressed != -1)
                {
                  registers[x] = key_pressed;
                }
              else
                {
                  /* if no key pressed keep looping */
                  pc -= 2;
                }
              break;
            }
            break;
          case 0x1E:
            {
              I += x;
            }
            break;
          case 0x29:
            {
              /* each font takes up 5 bytes, like so:
               * FNT_START: '0'
               * FNT_START+(1*5) '1'
               * FNT_START+(2*5) '2'
               * ...etc
               * */
              I = FNT_START + ((registers[x]) * 5);
            }
            break;
          case 0x33:
            {
              uint8_t in = registers[x];
              memory[I] = in / 100;
              in = in % 100;
              memory[I + 1] = in / 10;
              in = in % 10;
              memory[I + 2] = in;
            }
            break;
            /* TODO: Add an option to toggle counting by mutating I,
             * for compatibility with older ROMs
             */
          case 0x55:
            {
              for (int i = 0; i <= x; i++)
                {
                  memory[I + i] = registers[i];
                }
            }
            break;
          case 0x65:
            {
              for (int i = 0; i <= x; i++)
                {
                  registers[i] = memory[I + i];
                }
            }
          }
      }
    }
}
