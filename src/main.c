#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// Draw in terminal, with faces winded in ClockWise order
#define TRIANGLE_RASTER_TERMINAL
#define TRIANGLE_RASTER_CW

// Window dimensions
#ifndef TRIANGLE_RASTER_SDL
#define SW 125
#define SH 62
#endif

// Rendering parameters
#define TRIANGLE_RASTER_SLEEPMS 0
#define TRIANGLE_RASTER_FULL_COLOR
#define TRIANGLE_RASTER_AMBIENT 0.5f
#define TRIANGLE_RASTER_DIFFUSE 0.5f

// The triange rasterization library
#include "triangleraster.h"

// The Wavefront (.obj) parsing library
#include "objparser.h"

Mesh load_mesh(char* path, Color c){
	// Parse the scene
	OBJ_Data obj_data;
	if(!OBJ_parse(&obj_data,path)) exit(1);

	// Load mesh from the scene
	Mesh result = OBJ_data_to_mesh(&obj_data,c);

	// Free the scene
	OBJ_free_data(&obj_data);
	return result;
}

int main(int argc, char* argv[]){
	// Load the car mesh
	Mesh car_mesh = load_mesh("car.obj",(Color){255,125,50});
	car_mesh.pos = (Vec3){-1.f,0.f,6.25f};
	// Load the suzanne mesh
	Mesh suzanne_mesh = load_mesh("suzanne.obj",(Color){255,255,255});
	suzanne_mesh.pos = (Vec3){1.f,0.f,4.25f};

	// Initialize the rasterization libary
	if(!init()) return 1;
	
	// Loop
	float frames = 0.f;
	while(true){
		// Handle events if we are using SDL2 to close the window if necessary
		handle_events();

		clear_screen();

		// Render the car and suzanne with shading
		render_mesh_shaded(suzanne_mesh,(Vec3){10.f,10.f,0.f});
		render_mesh_shaded(car_mesh,(Vec3){10.f,10.f,0.f});

		// Rotate the car
		car_mesh.rot.y += 0.02f;
		car_mesh.rot.x = 0.5f*sin(0.75f*frames);

		// Rotate the suzanne mesh
		suzanne_mesh.rot.x += 0.03f;
		suzanne_mesh.rot.z += 0.02f;

		// Update the screen
		update_screen();
		frames += 0.01f;
	}

	// Free the mesh we loaded at the start
	OBJ_free_mesh(&car_mesh);
	OBJ_free_mesh(&suzanne_mesh);

	quit_everything();
}