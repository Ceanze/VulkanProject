#pragma once

#include "jaspch.h"

class Heightmap 
{
public:
	struct Vertex
	{
		glm::vec4 position;
		glm::vec4 normal;
	};
public:
	Heightmap();
	~Heightmap();

	void init(const glm::vec3& origin, int regionSize, int dataWidth, int dataHeight, unsigned char* data);

	void setMinZ(float value);
	void setMaxZ(float value);
	void setVertexDist(float value);
	void setProximitySize(int size);

	glm::ivec2 getRegionFromPos(const glm::vec3& position);

	int getProximityVertexDim();
	void getProximityVerticies(const glm::vec3& position, std::vector<Vertex>& verticies);
	const std::vector<Vertex>& getVerticies();
	const std::vector<unsigned>& getIndicies();
	int getVerticiesSize();
	int getIndiciesSize();
	
	static std::vector<unsigned> generateIndicies(int proximitySize, int regionSize);

	int getRegionCount();
	int getRegionWidthCount();
	int getRegionSize();
	int getIndiciesPerRegion();
	int getProximityRegionCount();
	int getProximityWidthRegionCount();

	int getWidth();
	int getHeight();

private:
	glm::vec3 origin;
	int proxDim;

	int proxVertDim;
	int indiciesPerRegion;
	int regionSize;
	int regionCount;
	int regionWidthCount;

	float minZ;
	float maxZ;
	float vertDist;
	int heightmapWidth;
	int heightmapHeight;

	std::vector<unsigned> indicies;
	std::vector<Vertex> verticies;
};