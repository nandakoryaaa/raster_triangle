#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct RGBA {
	unsigned char b, g, r, alpha;
} RGBA;

typedef struct {
	int x, y;
	union {
		RGBA rgba;
		unsigned long long value;
	} pixel;
} Vertex;

typedef struct {
	Vertex *v0;
	Vertex *v1;
	Vertex *v2;
	Vertex vertices[3];
} Triangle;

int rnd(int max) {
	return rand() % max;
}

void draw_rect(SDL_Surface *surf, int x, int y, int w, int h, int color) {
	SDL_Rect rect = {x, y, w, h};
	SDL_FillRect(surf, &rect, color);
}

void normalize_triangle(Triangle *t) {
	t->v0 = &t->vertices[0];
	t->v1 = &t->vertices[1];
	t->v2 = &t->vertices[2];
	if (t->v0->y > t->v1->y) {
		t->v1 = t->v0;
		t->v0 = &t->vertices[1];
	}

	if (t->v0->y > t->v2->y) {
		t->v2 = t->v0;
		t->v0 = &t->vertices[2];
	}

	if (t->v1->y > t->v2->y) {
		Vertex *tmp = t->v1;
		t->v1 = t->v2;
		t->v2 = tmp;
	}
}

void fill_triangle(SDL_Surface *surf, Triangle *t) {

	int cross_x1;
	int cross_x2;
	int start_x;
	int width;
	int progress_y;
	unsigned char start_r, start_g, start_b, start_alpha, end_r, end_g, end_b, end_alpha;
	int range_r, range_g, range_b, range_alpha, ar, ag, ab;

	normalize_triangle(t);
	Vertex *v0 = t->v0, *v1 = t->v1, *v2 = t->v2;

	int dx1 = v1->x - v0->x;
	int dy1 = v1->y - v0->y;
	int dx2 = v2->x - v0->x;
	int dy2 = v2->y - v0->y;
	int top_y = v0->y;

	unsigned char v0_r = v0->pixel.rgba.r;
	unsigned char v0_g = v0->pixel.rgba.g;
	unsigned char v0_b = v0->pixel.rgba.b;
	unsigned char v0_alpha = v0->pixel.rgba.alpha;

	unsigned char v1_r = v1->pixel.rgba.r;
	unsigned char v1_g = v1->pixel.rgba.g;
	unsigned char v1_b = v1->pixel.rgba.b;
	unsigned char v1_alpha = v1->pixel.rgba.alpha;

	unsigned char v2_r = v2->pixel.rgba.r;
	unsigned char v2_g = v2->pixel.rgba.g;
	unsigned char v2_b = v2->pixel.rgba.b;
	unsigned char v2_alpha = v2->pixel.rgba.alpha;

	unsigned int src_alpha;//, dst_alpha;

	int v0v2_r = v2_r - v0_r;
	int v0v2_g = v2_g - v0_g;
	int v0v2_b = v2_b - v0_b;
	int v0v2_alpha = v2_alpha - v0_alpha;

	int v0v1_r = v1_r - v0_r;
	int v0v1_g = v1_g - v0_g;
	int v0v1_b = v1_b - v0_b;
	int v0v1_alpha = v1_alpha - v0_alpha;

	if (dy1) {
		while(top_y <= v1->y) {
			progress_y = top_y - v0->y;
			cross_x1 = v0->x + dx1 * progress_y / dy1;
			cross_x2 = v0->x + dx2 * progress_y / dy2;

			if (cross_x1 > cross_x2) {
				start_x = cross_x2;
				width = (cross_x1 - cross_x2) + 1;

				start_r = v0_r + v0v2_r * progress_y / dy2;
				start_g = v0_g + v0v2_g * progress_y / dy2;
				start_b = v0_b + v0v2_b * progress_y / dy2;
				start_alpha = v0_alpha + v0v2_alpha * progress_y / dy2;

				end_r = v0_r + v0v1_r * progress_y / dy1;
				end_g = v0_g + v0v1_g * progress_y / dy1;
				end_b = v0_b + v0v1_b * progress_y / dy1;
				end_alpha = v0_alpha + v0v1_alpha * progress_y / dy1;			
			} else {
				start_x = cross_x1;
				width = (cross_x2 - cross_x1) + 1;

				start_r = v0_r + v0v1_r * progress_y / dy1;
				start_g = v0_g + v0v1_g * progress_y / dy1;
				start_b = v0_b + v0v1_b * progress_y / dy1;
				start_alpha = v0_alpha + v0v1_alpha * progress_y / dy1;			

				end_r = v0_r + v0v2_r * progress_y / dy2;
				end_g = v0_g + v0v2_g * progress_y / dy2;
				end_b = v0_b + v0v2_b * progress_y / dy2;
				end_alpha = v0_alpha + v0v2_alpha * progress_y / dy2;
			}

			range_r = end_r - start_r;
			range_g = end_g - start_g;
			range_b = end_b - start_b;
			range_alpha = end_alpha - start_alpha; 

			unsigned char *addr = (unsigned char *)surf->pixels + top_y * surf->pitch + start_x * 4;
			for (int i = 0; i < width; i++) {
				src_alpha = start_alpha + range_alpha * i / width;
				//dst_alpha = (255 - src_alpha);

				ab = (start_b + range_b * i / width) * src_alpha;
				ag = (start_g + range_g * i / width) * src_alpha;
				ar = (start_r + range_r * i / width) * src_alpha;

				addr[0] = (ab + addr[0] * (255 - src_alpha)) / 255;
				addr[1] = (ag + addr[1] * (255 - src_alpha)) / 255;
				addr[2] = (ar + addr[2] * (255 - src_alpha)) / 255;

				addr += 4;
			}

			top_y++;
		}
	}

	dx1 = v2->x - v1->x;
	dy1 = v2->y - v1->y;

	if (dy1) {
		while(top_y <= v2->y) {
			progress_y = top_y - v0->y;
			int progress_y1 = top_y - v1->y;
			cross_x1 = v1->x + dx1 * progress_y1 / dy1;
			cross_x2 = v0->x + dx2 * progress_y / dy2;
			if (cross_x1 > cross_x2) {
				start_x = cross_x2;
				width = (cross_x1 - cross_x2) + 1;

				start_r = v0_r + v0v2_r * progress_y / dy2;
				start_g = v0_g + v0v2_g * progress_y / dy2;
				start_b = v0_b + v0v2_b * progress_y / dy2;
				start_alpha = v0_alpha + v0v2_alpha * progress_y / dy2;

				end_r = v1_r + (v2_r - v1_r) * progress_y1 / dy1;
				end_g = v1_g + (v2_g - v1_g) * progress_y1 / dy1;
				end_b = v1_b + (v2_b - v1_b) * progress_y1 / dy1;
				end_alpha = v1_alpha + (v2_alpha - v1_alpha) * progress_y1 / dy1;
			} else {
				start_x = cross_x1;
				width = (cross_x2 - cross_x1) + 1;

				start_r = v1_r + (v2_r - v1_r) * progress_y1 / dy1;
				start_g = v1_g + (v2_g - v1_g) * progress_y1 / dy1;
				start_b = v1_b + (v2_b - v1_b) * progress_y1 / dy1;
				start_alpha = v1_alpha + (v2_alpha - v1_alpha) * progress_y1 / dy1;

				end_r = v0_r + v0v2_r * progress_y / dy2;
				end_g = v0_g + v0v2_g * progress_y / dy2;
				end_b = v0_b + v0v2_b * progress_y / dy2;
				end_alpha = v0_alpha + v0v2_alpha * progress_y / dy2;
			}

			range_r = end_r - start_r;
			range_g = end_g - start_g;
			range_b = end_b - start_b; 
			range_alpha = end_alpha - start_alpha;

			unsigned char *addr = (unsigned char *)surf->pixels + top_y * surf->pitch + start_x * 4;
			for (int i = 0; i < width; i++) {
				src_alpha = start_alpha + range_alpha * i / width;
				//dst_alpha = (255 - src_alpha);

				ab = (start_b + range_b * i / width) * src_alpha;
				ag = (start_g + range_g * i / width) * src_alpha;
				ar = (start_r + range_r * i / width) * src_alpha;

				addr[0] = (ab + addr[0] * (255 - src_alpha)) / 255;
				addr[1] = (ag + addr[1] * (255 - src_alpha)) / 255;
				addr[2] = (ar + addr[2] * (255 - src_alpha)) / 255;

				addr += 4;
			}
			top_y++;
		}
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

	SDL_Surface *screenSurface = SDL_GetWindowSurface(window);
	srand(time(0));

	Triangle tri, tri1, tri2;

	SDL_Event event;

	if (argc > 1 && !strcmp(argv[1], "bench")) {
		int ticks = SDL_GetTicks();
		int count = 0;

		tri1.vertices[0].x = 0;
		tri1.vertices[0].y = 0;
		tri1.vertices[1].x = w-1;
		tri1.vertices[1].y = 10;
		tri1.vertices[2].x = 10;
		tri1.vertices[2].y = h-1;

		tri2.vertices[0].x = 0;
		tri2.vertices[0].y = h-10;
		tri2.vertices[1].x = w-10;
		tri2.vertices[1].y = 0;
		tri2.vertices[2].x = w-1;
		tri2.vertices[2].y = h-1;

		while(1) {
			SDL_PollEvent(&event);
			if (event.type == SDL_QUIT) {
				break;
			}
			tri1.vertices[0].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
			tri1.vertices[1].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
			tri1.vertices[2].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
			tri2.vertices[0].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
			tri2.vertices[1].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
			tri2.vertices[2].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
			fill_triangle(screenSurface, &tri1);
			fill_triangle(screenSurface, &tri2);
			SDL_UpdateWindowSurface(window);
				
			count++;

			if (count > 9999) {
				break;
			}
		}

		ticks = SDL_GetTicks() - ticks;
		printf("%d triangles in %d ms\n", count, ticks);
	} else {
		while(1) {

			SDL_WaitEvent(&event);
			if (event.type == SDL_QUIT) {
				break;
			}
			if (event.type == SDL_KEYDOWN) {
				tri.vertices[0].x = rnd(w);
				tri.vertices[0].y = rnd(h);
				tri.vertices[0].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
				tri.vertices[1].x = rnd(w);
				tri.vertices[1].y = rnd(h);
				tri.vertices[1].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
				tri.vertices[2].x = rnd(w);
				tri.vertices[2].y = rnd(h);
				tri.vertices[2].pixel.value = (rnd(256) << 24) + (rnd(256) << 16) + (rnd(256) << 8) + rnd(256);
				fill_triangle(screenSurface, &tri);
				SDL_UpdateWindowSurface(window);
			}		
		}
		
	}

	SDL_FreeSurface(screenSurface);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
