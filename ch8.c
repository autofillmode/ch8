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

/* Memory size */
#define MEMB 4096
#define WINDOW_W 640 /* For SDL */
#define WINDOW_H 320 /* For SDL */
#define SCREEN_W 64
#define SCREEN_H 32
#define STACK_SIZE 255
/* Offset from which ROM is loaded */
#define START_ADDR 0x200
/* End of memory */
#define END_ADDR 0xFFF
/* Maximum amount for fread() to read */
#define ROM_MAX (0xFFF - 0x200)

#define FNT_START 0x50

/* Globals for simplicity */
uint16_t I; /* Index register */
uint16_t call_stack[STACK_SIZE], stackp;
uint8_t registers[16]; /* Registers V0 through VF */
uint8_t screen[SCREEN_W][SCREEN_H] = { 0 };
uint8_t memory[MEMB];                    /* 4096 Bytes of ram */
uint8_t *startptr = memory + START_ADDR; /* the ROM is loaded here */
int pc = START_ADDR;                     /* The program counter */
int last_goto; /* Last goto, useful for not printing inf. loops */
int old_pc;
int drawFlag = 0; /* whether any drawing is to be done */
void drawScreen (SDL_Renderer *renderer, uint8_t screen[SCREEN_W][SCREEN_H]);
void processCycle (void);

int
main (int argc, char *argv[])
{
  /* TODO: FX0A FX07 FX1{5,8} EX{9E,A1} */

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
      while (SDL_PollEvent (&e) != 0)
        {
          if (e.type == SDL_QUIT)
            {
              quit = 1;
            }
        }
      processCycle ();
      SDL_RenderClear (renderer);
      if (drawFlag)
        drawScreen (renderer, screen);
      SDL_RenderPresent (renderer);
      SDL_Delay (5);
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

  /* don't print 1st pc or inf. loops */
  if (old_pc != 0x200 && last_goto != nnn)
    printf ("0x%02X:", old_pc);

  switch (opcode)
    {
    case 0x0:
      {
        if (nn == 0x00)
          {
            memset (screen, 0, sizeof (screen));
            printf (" CLS\n"); /* clear the screen */
          }
        else if (nn == 0xEE)
          {
            pc = call_stack[--stackp]; /* return  */
            printf ("RET TO %02X\n", pc);
          }
      }
      break;
    case 0x1:
      {
        int old = pc;
        pc = nnn;
        if (last_goto != nnn) /* don't print infinite loops */
          printf (" GOTO 0x%03X\n", pc);
        last_goto = nnn;
      }
      break;
    case 0x2:
      {
        printf (" CALL %02X PUSH %02X \n", nnn, pc);
        call_stack[stackp] = pc;
        ++stackp;
        pc = nnn;
      }
      break;
    case 0x3:
      {
        if (registers[x] == nn)
          {
            printf (" SKIP %02X\n", pc);
            pc += 2;
          }
      }
      break;
    case 0x4:
      {
        if (registers[x] != nn)
          {
            printf (" SKIP %02X\n", pc);
            pc += 2;
          }
      }
      break;
    case 0x5:
      {
        if (registers[x] == registers[y])
          {
            printf (" SKIP %02X\n", pc);
            pc += 2;
          }
      }
      break;
    case 0x6:
      {
        registers[x] = nn;
        printf (" SET V%X 0x02%X\n", x, nn);
      }
      break;
    case 0x7:
      {
        registers[x] += nn;
        printf (" INCR V%X BY 0x%02X\n", x, nn);
      }
      break;
    case 0x8:
      {
        switch (n)
          {
          case 0x0:
            {
              printf (" SET V%X V%X\n", x, y);
              registers[x] = registers[y];
            }
            break;
          case 0x1:
            {
              printf (" V%X OR V%X\n", x, y);
              registers[x] |= registers[y];
            }
            break;
          case 0x2:
            {
              printf (" V%X AND V%X\n", x, y);
              registers[x] &= registers[y];
            }
            break;
          case 0x3:
            {
              printf ("V%X XOR V%X\n", x, y);
              registers[x] ^= registers[y];
            }
            break;
          case 0x4:
            {
              uint16_t sum;
              printf (" V%X += V%X\n", x, y);
              sum = registers[x] += registers[y];
              if (sum > 255)
                registers[0xF] = 1;
              else
                registers[0xF] = 0;
            }
            break;
          case 0x5:
            {
              printf (" V%X -= V%X\n", x, y);
              registers[x] -= registers[y];
            }
            break;
          case 0x6:
            {
              printf (" RSHIFT V%X\n", x);
              registers[x] = registers[x] >> 1;
            }
            break;
          case 0x7:
            {
              printf (" V%X = V%X -V%X\n", x, x, y);
              registers[x] = registers[y] - registers[x];
            }
            break;
          case 0xE:
            {
              printf (" LSHIFT V%X\n", x);
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
        printf (" SET I 0x%02X\n", nnn);
      }
      break;
    case 0xB:
      {
        pc = nnn + registers[0x0];
        printf (" GOTO-OFFSET 0x%02X", pc);
      }
      break;
    case 0xC:
      {
        registers[x] = (rand () % nn) & nn;
      }
    case 0xD:
      printf (" DISPLAY\n");
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
        drawFlag = 1;
      }
      break;
    case 0xF:
      {
        switch (nn) /* second byte */
          {
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
              printf (" FX55\n");
              for (int i = 0; i <= x; i++)
                {
                  memory[I + i] = registers[i];
                  printf ("0x%03X  |SET 0x%02x V%X\n", old_pc, i + I, i);
                }
            }
            break;
          case 0x65:
            {
              printf (" FX65\n");
              for (int i = 0; i <= x; i++)
                {
                  registers[i] = memory[I + i];
                  printf ("0x%03X  |SET V%X 0x%02X\n", old_pc, i, i + I);
                }
            }
            break;
          default:
            printf (" UNDEFINED INSTRUCTION %04X\n", instruction);
          }
      }
      break;
    }
}
