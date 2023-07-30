#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define QUEUE_SIZE 256

struct ThreadState {
	pthread_t thread_id;
};

struct RGBA {
	unsigned char b, g, r, alpha;
};

struct Vertex {
	int x, y;
	union {
		RGBA rgba;
		unsigned long long value;
	} pixel;
};

struct Triangle {
	Vertex* v0;
	Vertex* v1;
	Vertex* v2;
	Vertex vertices[3];
};

struct Workload {
	unsigned char* addr;
	int width;
	unsigned char start_r;
	unsigned char start_g;
	unsigned char start_b;
	unsigned char start_alpha;
	unsigned char end_r;
	unsigned char end_g;
	unsigned char end_b;
	unsigned char end_alpha;
};

static Workload workbuffer[QUEUE_SIZE];
static volatile int work_head = 0;
static volatile int work_tail = 0;
static volatile int working = 1;
pthread_mutex_t mutex;

int rnd(int max)
{
	return rand() % max;
}

void normalize_triangle(Triangle *t)
{
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

void* draw_line(void* arg)
{
	ThreadState* state = (ThreadState*) arg;
	printf("thread start: %d\n", state->thread_id);
	while (1) {
		pthread_mutex_lock(&mutex);
		while (work_tail == work_head) {
			if (!working) {
				pthread_mutex_unlock(&mutex);
				printf("thread end: %d\n", state->thread_id);
				return NULL;
			}
			//_sleep(1);
		}

		Workload w = workbuffer[work_tail];

		if (work_tail + 1 == QUEUE_SIZE) {
			work_tail = 0;
		} else {
			++work_tail;
		}

		pthread_mutex_unlock(&mutex);

		int range_r = w.end_r - w.start_r;
		int range_g = w.end_g - w.start_g;
		int range_b = w.end_b - w.start_b;
		int range_alpha = w.end_alpha - w.start_alpha;

		for (int i = 0; i < w.width; i++) {
			unsigned int src_alpha = w.start_alpha + range_alpha * i / w.width;

			int ab = (w.start_b + range_b * i / w.width) * src_alpha;
			int ag = (w.start_g + range_g * i / w.width) * src_alpha;
			int ar = (w.start_r + range_r * i / w.width) * src_alpha;

			w.addr[0] = (ab + w.addr[0] * (255 - src_alpha)) / 255;
			w.addr[1] = (ag + w.addr[1] * (255 - src_alpha)) / 255;
			w.addr[2] = (ar + w.addr[2] * (255 - src_alpha)) / 255;

			w.addr += 4;
		}
	}
}

void queue(Workload w)
{
	int next_head = work_head + 1;
	if (next_head == QUEUE_SIZE) {
		next_head = 0;
	}
	while (next_head == work_tail) {
		//_sleep(1);
	}
	workbuffer[work_head] = w;
	work_head = next_head;
}

void fill_triangle(SDL_Surface *surf, Triangle *t)
{
	int cross_x1;
	int cross_x2;
	int start_x;
	int width;
	int progress_y;
	unsigned char start_r, start_g, start_b, start_alpha, end_r, end_g, end_b, end_alpha;

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

			queue({
				(unsigned char *)surf->pixels + top_y * surf->pitch + start_x * 4,
				width,
				start_r, start_g, start_b, start_alpha,
				end_r, end_g, end_b, end_alpha
			});

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

			queue({
				(unsigned char *)surf->pixels + top_y * surf->pitch + start_x * 4,
				width,
				start_r, start_g, start_b, start_alpha,
				end_r, end_g, end_b, end_alpha
			});

			top_y++;
		}
	}
}

int main(int argc, char* argv[])
{
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

	ThreadState thread_state1;
	ThreadState thread_state2;
	ThreadState thread_state3;
	ThreadState thread_state4;

	pthread_mutex_init(&mutex, NULL);

	pthread_create(&thread_state1.thread_id, NULL, draw_line, &thread_state1);
	pthread_create(&thread_state2.thread_id, NULL, draw_line, &thread_state2);
	pthread_create(&thread_state3.thread_id, NULL, draw_line, &thread_state3);
	pthread_create(&thread_state4.thread_id, NULL, draw_line, &thread_state4);

	if (argc > 1 && !strcmp(argv[1], "bench")) {
		int ticks = SDL_GetTicks();
		int count = 0;

		tri1.vertices[0].x = 0;
		tri1.vertices[0].y = 0;
		tri1.vertices[1].x = w - 1;
		tri1.vertices[1].y = 10;
		tri1.vertices[2].x = 10;
		tri1.vertices[2].y = h - 1;

		tri2.vertices[0].x = 0;
		tri2.vertices[0].y = h - 10;
		tri2.vertices[1].x = w - 10;
		tri2.vertices[1].y = 0;
		tri2.vertices[2].x = w - 1;
		tri2.vertices[2].y = h - 1;

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
				
			count += 2;
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

	working = 0;

	pthread_join(thread_state1.thread_id, NULL);
	pthread_join(thread_state2.thread_id, NULL);
	pthread_join(thread_state3.thread_id, NULL);
	pthread_join(thread_state4.thread_id, NULL);

	SDL_FreeSurface(screenSurface);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
