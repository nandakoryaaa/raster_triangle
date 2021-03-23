#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int rnd(int max) {
    return rand() % max;
}

void draw_alpha_line(
	SDL_Surface *surf, int x, int y, int w, int color
) {
	unsigned char *pixels = (unsigned char *) surf->pixels;
	int	alpha = color & 255;
	int alpha1 = 255 - alpha;
	int	r = ((color >> 24) & 255) * alpha;
	int	g = ((color >> 16) & 255) * alpha;
	int	b = ((color >> 8) & 255) * alpha;
	int addr = y * surf->pitch + x * 4;
	for (int i = 0; i < w; i++) {
		int r1 = pixels[addr] * alpha1;
		int	g1 = pixels[addr + 1] * alpha1;
		int	b1 = pixels[addr + 2] * alpha1;
		pixels[addr++] = (r + r1) / 255;
		pixels[addr++] = (g + g1) / 255;
		pixels[addr++] = (b + b1) / 255;
		addr++;
	}
}

void draw_triangle(SDL_Surface *surf, int x0, int y0, int x1, int y1, int x2, int y2, int color) {
    int tmp;
    if (y0 > y1) {
        tmp = y0;
        y0 = y1;
        y1 = tmp;
        tmp = x0;
        x0 = x1;
        x1 = tmp;
    }

    if (y0 > y2) {
        tmp = y0;
        y0 = y2;
        y2 = tmp;
        tmp = x0;
        x0 = x2;
        x2 = tmp;
    }

    if (y1 > y2) {
        tmp = y1;
        y1 = y2;
        y2 = tmp;
        tmp = x1;
        x1 = x2;
        x2 = tmp;
    }

    int cross_x1;
    int cross_x2;
    int dx1 = x1 - x0;
    int dy1 = y1 - y0;
    int dx2 = x2 - x0;
    int dy2 = y2 - y0;

    int top_y = y0;

    while(top_y < y1) {
        cross_x1 = x0 + dx1 * (top_y - y0) / dy1;
        cross_x2 = x0 + dx2 * (top_y - y0) / dy2;
        if (cross_x1 > cross_x2) {
            draw_alpha_line(surf, cross_x2, top_y, cross_x1 - cross_x2, color);
        } else {
            draw_alpha_line(surf, cross_x1, top_y, cross_x2 - cross_x1, color);
        }

        top_y++;
    }

    dx1 = x2 - x1;
    dy1 = y2 - y1;
    while(top_y < y2) {
        cross_x1 = x1 + dx1 * (top_y - y1) / dy1;
        cross_x2 = x0 + dx2 * (top_y - y0) / dy2;
        if (cross_x1 > cross_x2) {
            draw_alpha_line(surf, cross_x2, top_y, cross_x1 - cross_x2, color);
        } else {
            draw_alpha_line(surf, cross_x1, top_y, cross_x2 - cross_x1, color);
        }

        top_y++;
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    int w = 640;
    int h = 480;

    SDL_Window *window = SDL_CreateWindow(
        "Triangles",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        w,
        h,
        SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (window == NULL) {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    int colors[] = {0x0000ff30, 0x00ff0030, 0x00ffff30, 0xff000030, 0xff00ff30, 0xffff0030, 0xffffff30};
    int color_num = 0;

    SDL_Surface *screenSurface = SDL_GetWindowSurface(window);

    srand(time(0));

    SDL_Event event;

    int ticks = SDL_GetTicks();
    int count = 0;
	SDL_Rect rect = {0, 0, w, h};
    while(1) {
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }

        int x0 = rnd(w);
        int y0 = rnd(h);
        int x1 = rnd(w);
        int y1 = rnd(h);
        int x2 = rnd(w);
        int y2 = rnd(h);

        int color = colors[color_num];
        color_num++;
        if (color_num > 6) {
            color_num = 0;
        }

        draw_triangle(screenSurface, x0, y0, x1, y1, x2, y2, color);
        SDL_UpdateWindowSurface(window);

        count++;

        if (count > 99999) {
           break;
        }
    }

    ticks = SDL_GetTicks() - ticks;

    printf("%d triangles in %d ms\n", count, ticks);

    SDL_FreeSurface(screenSurface);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
