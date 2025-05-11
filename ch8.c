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

void drawScreen (SDL_Renderer *renderer, uint8_t screen[SCREEN_W][SCREEN_H]);

int
main (int argc, char *argv[])
{
  /* TODO: FX{5,6}5 FX33, FX29 FX0A FX1E FX07 FX1{5,8} EX{9E,A1} */

  SDL_Window *gWindow = NULL;
  SDL_Renderer *renderer = NULL;
  FILE *rom;

  uint8_t screen[SCREEN_W][SCREEN_H] = { 0 };

  uint16_t I; /* Index register */
  uint16_t call_stack[STACK_SIZE], stackp;
  uint8_t registers[16];                   /* Registers V0 through VF */
  uint8_t memory[MEMB];                    /* 4096 Bytes of ram */
  uint8_t *startptr = memory + START_ADDR; /* the ROM is loaded here */
  int pc = START_ADDR;                     /* The program counter */
  int last_goto; /* Last goto, useful for not printing inf. loops */

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
      SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);
      SDL_RenderClear (renderer);
      drawScreen (renderer, screen);
      SDL_RenderPresent (renderer);

      uint8_t x, y, n, nn, opcode;
      uint16_t nnn, instruction;

      instruction = (memory[pc] << 8) | memory[pc + 1];
      opcode = (instruction & 0xF000) >> 12;
      x = (instruction & 0x0F00) >> 8;
      y = (instruction & 0x00F0) >> 4;
      n = instruction & 0x000F;
      nn = (instruction & 0x00FF);
      nnn = (instruction & 0x0FFF);

      pc += 2;

      switch (opcode)
        {
        case 0x0:
          {
            if (nn == 0x00)
              {
                memset (screen, 0, sizeof (screen));
                printf ("0x%x: CLS\n", pc); /* clear the screen */
              }
            else if (nn == 0xEE)
              {
                pc = call_stack[--stackp]; /* return  */
                printf ("0x%x: RETURN\n", pc);
              }
          }
          break;
        case 0x1:
          {
            int old = pc;
            pc = nnn;
            if (last_goto != nnn) /* don't print infinite loops */
              printf ("0x%x: GOTO 0x%x\n", old, pc);
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
              pc += 2;
          }
          break;
        case 0x4:
          {
            if (registers[x] != nn)
              pc += 2;
          }
          break;
        case 0x5:
          {
            if (registers[x] == registers[y])
              pc += 2;
          }
          break;
        case 0x6:
          {
            registers[x] = nn;
            printf ("0x%x: V%x = 0x%x\n", pc, x, nn);
          }
          break;
        case 0x7:
          {
            registers[x] += nn;
            printf ("0x%x: V%x += 0x%x = %x\n", pc, x, nn, registers[x]);
          }
          break;
        case 0x8:
          {
            printf ("0x%x:", pc);
            switch (n)
              {
              case 0x0:
                {
                  printf ("V%x = V%x\n", x, y);
                  registers[x] = registers[y];
                }
                break;
              case 0x1:
                {
                  printf ("V%x OR V%x\n", x, y);
                  registers[x] |= registers[y];
                }
                break;
              case 0x2:
                {
                  printf ("V%x =AND V%x\n", x, y);
                  registers[x] &= registers[y];
                }
                break;
              case 0x3:
                {
                  printf ("V%x =XOR V%x\n", x, y);
                  registers[x] ^= registers[y];
                }
                break;
              case 0x4:
                {
                  uint16_t sum;
                  printf ("V%x += V%x\n", x, y);
                  sum = registers[x] += registers[y];
                  if (sum > 255)
                    registers[0xF] = 1;
                  else
                    registers[0xF] = 0;
                }
                break;
              case 0x5:
                {
                  printf ("V%x -= V%x\n", x, y);
                  registers[x] -= registers[y];
                }
                break;
              case 0x6:
                {
                  printf ("V%x >> 1\n", x);
                  registers[x] = registers[x] >> 1;
                }
                break;
              case 0x7:
                {
                  printf ("V%x = V%x -V%x\n", x, x, y);
                  registers[x] = registers[y] - registers[x];
                }
                break;
              case 0xE:
                {
                  printf ("V%x << 1\n", x);
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
            printf ("0x%x: I = 0x%x\n", pc, nnn);
          }
          break;
        case 0xB:
          {
            int old = pc;
            pc = nnn + registers[0x0];
            printf ("0x%xGOTO-OFFSET pc = 0x%x", old, pc);
          }
          break;
        case 0xC:
          {
            registers[x] = (rand () % nn) & nn;
          }
        case 0xD:
          printf ("0x%x: DISPLAY\n", pc);
          {
            uint8_t p_x = registers[x] % 64;
            uint8_t p_y = registers[y] % 32;
            registers[0xF] = 0;
            int index = I;

            for (unsigned int r = 0; r < n; ++r)
              {
                uint8_t sprite = memory[index + r];

                for (unsigned char c = 0; c < 8; ++c)
                  {
                    uint8_t spritepxl = sprite & (0x80u >> c);
                    uint8_t screenpxl = screen[p_x + c][p_y + r];
                    if (spritepxl)
                      {
                        if (screenpxl)
                          {
                            registers[0xF] = 1;
                            screen[p_x + c][p_y + r] = 0;
                          }
                        else
                          {
                            screen[p_x + c][p_y + r] = 1;
                          }
                      }
                  }
              }
          }
          break;
        }
      SDL_Delay (50);
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
