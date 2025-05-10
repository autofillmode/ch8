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

#define MEMB 4096
#define SCREEN_W 640
#define SCREEN_H 320
#define MAX_ROM_LEN 65536
#define STACK_SIZE 255
#define FPS 60
#define TICKS_PER_FRAME 1000 / FPS

void drawScreen (SDL_Renderer *renderer, int screen[64][32]);

int
main (int argc, char *argv[])
{

  SDL_Window *gWindow = NULL;
  SDL_Renderer *renderer = NULL;

  int screen[64][32] = { 0 };

  FILE *rom;
  uint16_t I = 512;
  uint8_t registers[16];
  unsigned char program[MAX_ROM_LEN];
  uint8_t pixels[640 * 320];
  uint8_t memory[MEMB]
      = { 0xF0, 0x90, 0x90, 0xF0, 0x20, 0x60, 0x20, 0x70, 0xF0, 0xF0, 0x10,
          0xF0, 0x10, 0xF0, 0xF0, 0xF0, 0x10, 0x90, 0x90, 0xF0, 0x10, 0x10,
          0xF0, 0x80, 0xF0, 0x10, 0xF0, 0xF0, 0x80, 0xF0, 0x90, 0xF0, 0xF0,
          0x10, 0x20, 0x40, 0x40, 0xF0, 0x90, 0xF0, 0x90, 0xF0, 0xF0, 0x90,
          0xF0, 0x10, 0xF0, 0xF0, 0x90, 0xF0, 0x90, 0x90, 0xE0, 0x90, 0xE0,
          0x90, 0xE0, 0xF0, 0x80, 0x80, 0x80, 0xF0, 0xE0, 0x90, 0x90, 0x90,
          0xE0, 0xF0, 0x80, 0xF0, 0x80, 0xF0, 0xF0, 0x80, 0xF0, 0x80, 0x80 };
  uint16_t call_stack[255], stackp;
  int rom_len;
  int pc;

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
  rom_len = fread (program, 1, MAX_ROM_LEN, rom);

  SDL_CreateWindowAndRenderer (SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN, &gWindow,
                               &renderer);

  SDL_SetRenderDrawColor (renderer, 0, 0, 0, 0);
  // SDL_RenderSetScale (renderer, 10.0, 10.0);
  int quit = 0;
  SDL_Event e;
  int i = 0;

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
      uint16_t nnn;

      uint16_t instruction = (program[pc] << 8) | program[pc + 1];
      opcode = (instruction & 0xF000) >> 12;
      x = (instruction & 0x0FF0) >> 4;
      y = (instruction & 0x00F0) >> 4;
      n = instruction & 0x000F;
      nn = (instruction & 0x00FF);
      nnn = (instruction & 0x0FFF);

      pc += 2;

      switch (opcode)
        {
        case 0x0:
          {
            if (n == 0x0)
              {
                memset (screen, 0, sizeof (screen));
                printf ("0x%x: CLS\n", pc); /* clear the screen */
              }
            else if (nn == 0xEE)
              {
                pc = call_stack[stackp--]; /* return  */
                printf ("0x%x: RETURN\n", pc);
              }
          }
          break;
        case 0x1:
          {
            pc = nnn;
            printf ("0x%x: GOTO 0x%x\n", pc, nnn);
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
        case 0xA:
          {
            I = nnn;
            printf ("0x%x: I = 0x%x\n", pc, nnn);
          }
          break;
        case 0xD:
          printf ("0x%x: DISPLAY\n", pc);
          {
            int p_x = registers[x] % 64;
            int p_y = registers[y] % 32;
            int orig_x = p_x;
            int orig_y = p_y;
            registers[0xF] = 0;
            int index = I;
            printf ("%d %d ", orig_x, orig_y);

            for (int i = 0; i < n; i++)
              {
                if (orig_y + y == 32)
                  break;
                uint8_t sprite = memory[index++];

                for (int j = 0; j < 8; j++)
                  {
                    if (orig_x + x == 64)
                      break;
                    if (((sprite >> j) & 1) == 1)
                      {
                        if (screen[x][y] == 1)
                          {
                            screen[x][y] = 0;
                            registers[0xF] = 1;
                          }
                        if (screen[x][y] != 1)
                          {
                            screen[x][y] = 1;
                          }
                      }
                    x++;
                  }
                y++;
              }
          }
          break;
        }

      SDL_Delay (500);
    }
}
void
drawScreen (SDL_Renderer *renderer, int screen[64][32])
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
