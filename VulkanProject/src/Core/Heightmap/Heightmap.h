#pragma once

#include "jaspch.h"

class Heightmap 
{
public:
	struct HeightmapRawData {
		int width, height;
		size_t size;
		unsigned char* data;
	};

	struct HeightmapVertexData {
		int width, height;
		std::vector<unsigned> indicies;
		std::vector<glm::vec4> verticies;
	};

public:
	Heightmap();
	~Heightmap();

	void setMinZ(float value);
	void setMaxZ(float value);
	void setVertexDist(float value);

	HeightmapVertexData getVertexData(unsigned index);
	HeightmapRawData getData(unsigned index);
	void loadImage(const std::string& path);

	void cleanup();

private:
	float minZ;
	float maxZ;
	float vertDist;

	std::vector<HeightmapVertexData> vertexData;
	std::vector<HeightmapRawData> rawData;
};