#ifndef TRIANGLE_RASTER_H
#define TRIANGLE_RASTER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// Boolean values
#define bool _Bool
#define true 1
#define false 0

typedef struct {
	uint8_t r,g,b;
} Color;

// Functions
bool init();
void quit_everything();
void handle_events();
void clear_screen();
void update_screen();
void set_pixel(Color color, int x, int y);

// ------------------

#if defined(TRIANGLE_RASTER_SDL)
// Will render the triangles with SDL2
#include <SDL2/SDL.h>

// Resolution of the SDL Window (in pixels)
#ifndef SW
#define SW 125
#endif
#ifndef SH
#define SH 125
#endif

// Window and renderer
SDL_Window* win = NULL;
SDL_Renderer* rend = NULL;
SDL_Event event;

// Initialize SDL2
bool init(){
	if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
		printf("SDL_Init error: %s\n",SDL_GetError());
		return false;
	}
	win = SDL_CreateWindow("Triangle Rasterizer",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,SW,SH,SDL_WINDOW_SHOWN);
	if(win == NULL){
		printf("SDL_CreateWindow error: %s\n",SDL_GetError());
		return false;
	}
	rend = SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
	if(rend == NULL){
		printf("SDL_CreateRenderer error: %s\n",SDL_GetError());
		return false;
	}
	return true;
}

// Quits libraries
void quit_everything(){
	SDL_DestroyRenderer(rend);
	SDL_DestroyWindow(win);
	SDL_Quit();
	exit(0);
}

// Clear the screen
void clear_screen(){
	SDL_SetRenderDrawColor(rend,0,0,0,255);
	SDL_RenderClear(rend);
}

// Handle events in case user presses X
void handle_events(){
	while(SDL_PollEvent(&event) != 0){
		if(event.type == SDL_QUIT)
			quit_everything();
	}
}

// Update the screen
void update_screen(){
	SDL_RenderPresent(rend);
}

// Set a pixel to a specific color
void set_pixel(Color color, int x, int y){
	SDL_SetRenderDrawColor(rend,color.r,color.g,color.b,255);
	SDL_RenderDrawPoint(rend,x,y);
}


// ------------------

#elif defined(TRIANGLE_RASTER_TERMINAL)
// Will render the triangles on the terminal using ASCII characters

// Sleeping to wait for next frame
#if defined(_WIN32)
#include <windows.h>
#define sleepms(ms) Sleep((ms))
#elif defined(__linux__)
#include <unistd.h>
#define sleepms(ms) usleep((ms)*1000)
#endif

// Resolution
#ifndef SW
#define SW 100
#endif
#ifndef SH
#define SH 50
#endif

// Screen buffer for characters to be rendered to the screen
char screen_buffer[SH][SW+1];

const char grayscale[] = "`.-':_,^=;><+!rc*/z?sLTv)J7(|Fi{C}fI31tlu[neoZ5Yxjya]2ESwqkP6h9d4VpOGbUAKXHm8RD#$Bg0MNWQ%&@";
float grayscale_len = 0.f;

// Initialize the screen buffer
bool init(){
	grayscale_len = (float)strlen(grayscale);
	return true;
}

// Just exit the program
void quit_everything(){
	exit(0);
}

// Clear the screen
void clear_screen_buffer(){
	for(int i = 0; i < SH; i++){
		memset(screen_buffer[i],' ',SW);
		screen_buffer[i][SW] = '\0';
	}
}
#if defined(_WIN32)
void clear_screen(){
	clear_screen_buffer();
	system("cls");
}
#elif defined(__linux__)
void clear_screen(){
	clear_screen_buffer();
	printf("\e[2J\e[H");
}
#endif

// Just do nothing (no events to poll or handle)
void handle_events(){}

#ifndef TRIANGLE_RASTER_SLEEPMS
#define TRIANGLE_RASTER_SLEEPMS 2
#endif

// Update the screen
void update_screen(){
	for(int i = 0; i < SH; i++){
		printf("%s\n",screen_buffer[i]);
	}
	sleepms(TRIANGLE_RASTER_SLEEPMS);
}

// Set a pixel to a specific color
void set_pixel(Color color, int x, int y){
	if(x < 0 || x >= SW || y < 0 || y >= SH)
		return;
	float brightness = (float)color.r*0.299f+(float)color.g*0.587f+(float)color.b*0.114f;
	int grayscale_index = (int)round(brightness/600.f*grayscale_len);
	screen_buffer[y][x] = grayscale[grayscale_index];
}


// ---------------------------

#else
#error "Please define TRIANGLE_RASTER_SDL or TRIANGLE_RASTER_TERMINAL to chose what rendering interface to use."
#endif

// ---------------------------

// Custom made math library to fit this rasterizer's needs
#include "trianglemath.h"

// Render triangles
void render_triangles(Triangle* triangle_array, int triangle_count){
	// Copy of triangles given to transform for projection
	Triangle* triangles = (Triangle*) malloc(sizeof(Triangle)*triangle_count);
	// Get bounds and store them
	struct TriangleBounds* bounds = (struct TriangleBounds*) malloc(sizeof(struct TriangleBounds)*triangle_count);
	struct TriangleBounds global_bounds;
	for(int i = 0; i < triangle_count; i++) {
		triangles[i] = project_triangle(triangle_array[i]);
		bounds[i] = get_triangle_bounds(triangles[i]);
		if(i == 0) { global_bounds = bounds[i]; continue; }
		global_bounds.xmin = min(global_bounds.xmin,bounds[i].xmin);
		global_bounds.xmax = max(global_bounds.xmax,bounds[i].xmax);
		global_bounds.ymin = min(global_bounds.ymin,bounds[i].ymin);
		global_bounds.ymax = max(global_bounds.ymax,bounds[i].ymax);
	}
	//printf("bounds: %d, %d, %d, %d\n",global_bounds.xmin,global_bounds.xmax,global_bounds.ymin,global_bounds.ymax);
	
	// Loop through all triangles' bounds and check for each pixel
	for(int y = global_bounds.ymin; y < global_bounds.ymax; y++){
		for(int x = global_bounds.xmin; x < global_bounds.xmax; x++){
			// Current pixel's color
			Color fragment_color = (Color){0,0,0};
			// Depth of the closest triangle
			float closest_depth = 100.f;
			for(int i = 0; i < triangle_count; i++){
				// Current triangle
				Triangle tri = triangles[i];
				// Check if in bounds of the current triangle
				if(x < bounds[i].xmin || x > bounds[i].xmax || y < bounds[i].ymin || y > bounds[i].ymax) continue;
				// Check if pixel is inside triangle
				Vec3 p = barycentric(tri.v[0],tri.v[1],tri.v[2],px_to_screen_space(x,y));
				// If yes, fill the pixel with the interpolated color
				if(p.x != -1.f){
					float depth = interpolate_depth(tri.v[0],tri.v[1],tri.v[2],p);
					// If another triangle is closer, dont render atop
					if(depth < closest_depth && depth > 0.1f){
						fragment_color = interpolate_colors(tri.c[0],tri.c[1],tri.c[2],p);
						closest_depth = depth; // Update the closest triangle
					}
				}
			}
			// If the fragment's color isnt black, render the pixel
			if(fragment_color.r != 0 || fragment_color.g != 0 || fragment_color.b != 0){
				set_pixel(fragment_color,x,y);
				//printf("color: %d, %d, %d\n",fragment_color.r,fragment_color.g,fragment_color.b);
			}
		}
	}
	// Free resources
	free(triangles);
	free(bounds);
}

// Transform triangles and render mesh
void render_mesh(Mesh m){
	if(m.triangle_count == 0) return;
	Triangle* triangles = (Triangle*) malloc(sizeof(Triangle)*m.triangle_count);
	for(int i = 0; i < m.triangle_count; i++){
		triangles[i] = transform_triangle(m.triangles[i],m.pos,m.rot);
	}
	render_triangles(triangles,m.triangle_count);
	free(triangles);
}

// Transform triangles and render mesh with shading
void render_mesh_shaded(Mesh m, Vec3 light){
	if(m.triangle_count == 0) return;
	Triangle* triangles = (Triangle*) malloc(sizeof(Triangle)*m.triangle_count);
	for(int i = 0; i < m.triangle_count; i++){
		triangles[i] = transform_triangle(m.triangles[i],m.pos,m.rot);
		shade_triangle(&triangles[i],light);
	}
	render_triangles(triangles,m.triangle_count);
	free(triangles);
}

#endif