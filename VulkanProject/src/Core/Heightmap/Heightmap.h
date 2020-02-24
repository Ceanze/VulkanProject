#pragma once

#include "jaspch.h"

class Heightmap 
{
public:
	struct HeightmapData {
		int width, height;
		size_t size;
		unsigned char* data;
	};

public:
	Heightmap();
	~Heightmap();

	HeightmapData getData(unsigned index);
	void loadImage(const std::string& path);

	void cleanup();


private:
	std::vector<HeightmapData> data;
};