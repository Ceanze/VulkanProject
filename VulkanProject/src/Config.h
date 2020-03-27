#pragma once

#define MULTI_THREADED true	

#define WIREFRAME true			
#define FULLSCREEN true

#define SIMULATED_JOB_COUNT 1				// Secondary command buffer record repeats
#define SIMULATED_JOB_SIZE 0				// Sleep time (microseconds)

#define TREE_COUNT 10000

#define CAMERA_SPEED 40
#define CAMERA_SPRINT_SPEED_MULTIPLIER 2
#define FRUSTUM_SHRINK_FACTOR -5.f			// Postive number equals smaller frustum, which provides visibility of culling

#define VERTEX_DISTANCE 2.f					// Distance between each vertices in heightmap
#define REGION_SIZE 8						// Number of vertices in a region
#define MAX_HEIGHT 20.f
#define MIN_HEIGHT 0.f
#define PROXIMITY_SIZE 30					// Number of loaded regions equals PROXIMITY_SIZE * 2 + 1
#define TRANSFER_PROXIMITY_THRESHOLD 10

#define PROFILER_JSON_FILE_NAME "Result.json"