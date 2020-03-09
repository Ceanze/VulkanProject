#include "jaspch.h"
#include "Heightmap.h"


Heightmap::Heightmap()
	: minZ(0),
	maxZ(1),
	vertDist(0.1),
	origin(0.f, 0.f, 0.f),
	regionCount(0),
	regionWidthCount(0),
	regionSize(2),
	proxDim(1),
	proxVertDim(0)
{
}

Heightmap::~Heightmap()
{
}

void Heightmap::init(const glm::vec3& origin, int regionSize, int dataWidth, int dataHeight, unsigned char* data)
{
	this->origin = origin;

	if (regionSize > this->regionSize) {
		this->regionSize = regionSize;
	}

	const int quads = this->regionSize - 1;
	this->indiciesPerRegion = (quads * quads) * 6;

	int regionCountWidth = 1 + this->proxDim * 2;
	this->proxVertDim = this->regionSize * regionCountWidth - regionCountWidth + 1;

	int B = dataWidth;
	int b = regionSize;
	this->regionWidthCount = static_cast<int>(ceilf(((B - 1) / (float)(b - 1))));
	this->regionCount = this->regionWidthCount * this->regionWidthCount;
	int padX = 0;
	int padY = 0;
	padX = padY = this->regionWidthCount * (b-1) + 1 - dataWidth;

	this->heightmapWidth = dataWidth + padX;
	this->heightmapHeight = dataHeight + padY;
	this->verticies.resize(this->heightmapWidth * this->heightmapHeight);

	const unsigned maxValue = 0xFF;
	float zDist = abs(this->maxZ - this->minZ);
	for (int y = 0; y < this->heightmapHeight; y++)
	{
		for (int x = 0; x < this->heightmapWidth; x++)
		{
			float height = 0;
			if (y < dataHeight && x < dataWidth)
				height = this->minZ + ((float)data[x + y * dataWidth] / maxValue) * zDist;

			float xPos = this->origin.x + x * this->vertDist;
			float zPos = this->origin.z + y * this->vertDist;

			this->verticies[x + y * this->heightmapHeight] = glm::vec4(xPos, height, zPos, 1.0);
		}
	}
}

void Heightmap::setMinZ(float val)
{
	this->minZ = minZ;
}

void Heightmap::setMaxZ(float value)
{
	this->maxZ = value;
}

void Heightmap::setVertexDist(float value)
{
	this->vertDist = value;
}

void Heightmap::setProximitySize(int size)
{
	this->proxDim = size;
}
glm::ivec2 Heightmap::getRegionFromPos(const glm::vec3& position)
{
	float regionWorldSize = (this->regionSize - 1) * this->vertDist;

	float xDistance = position.x - this->origin.x;
	float zDistance = position.z - this->origin.z;

	glm::vec2 index(0);
	// Calculate region index from position
	index.x = static_cast<int>(xDistance / regionWorldSize);
	index.y = static_cast<int>(zDistance / regionWorldSize);

	return index;
}
int Heightmap::getProximityVertexDim()
{
	return this->proxVertDim;
}
void Heightmap::getProximityVerticies(const glm::vec3& position, std::vector<glm::vec4>& verticies)
{
	float xDistance = position.x - this->origin.x;
	float zDistance = position.z - this->origin.z;

	// Calculate vertex index from position
	int xIndex = static_cast<int>(xDistance / this->vertDist);
	int zIndex = static_cast<int>(zDistance / this->vertDist);

	if (this->proxVertDim % 2 != 0) {
		xIndex -= this->proxVertDim / 2;
		zIndex -= this->proxVertDim / 2;
	}
	else {
		xIndex -= (this->proxVertDim / 2) - 1;
		zIndex -= (this->proxVertDim / 2) - 1;
	}

	for (int j = 0; j < this->proxVertDim; j++)
	{
		int zTemp = zIndex + j;
		if (zTemp >= 0 && zTemp < this->heightmapHeight) {
			for (int i = 0; i < this->proxVertDim; i++)
			{
				int xTemp = xIndex + i;
				if (xTemp >= 0 && xTemp < this->heightmapWidth) {
					int offset = xTemp + zTemp*this->heightmapWidth;
					verticies[i + j * this->proxVertDim] = this->verticies[offset];
				}
			}
		}
	}
}

const std::vector<glm::vec4>& Heightmap::getVerticies()
{
	return this->verticies;
}

const std::vector<unsigned>& Heightmap::getIndicies()
{
	return this->indicies;
}

int Heightmap::getVerticiesSize()
{
	return this->verticies.size();
}

int Heightmap::getIndiciesSize()
{
	return this->indicies.size();
}

std::vector<unsigned> Heightmap::generateIndicies(int proximitySize, int regionSize)
{
	std::vector<unsigned> indexData;

	const int numQuads = regionSize - 1;

	int B = proximitySize;
	int b = regionSize;
	int regionCount = static_cast<int>(ceilf(((B - 1) / (float)(b - 1))));

	int vertOffsetX = 0;
	int vertOffsetZ = 0;

	int index = 0;
	uint32_t size = regionCount * regionCount * numQuads * numQuads*6;
	indexData.resize(size);
	for (int j = 0; j < regionCount; j++)
	{
		for (int i = 0; i < regionCount; i++)
		{
			for (int z = vertOffsetZ; z < vertOffsetZ + numQuads; z++)
			{
				for (int x = vertOffsetX; x < vertOffsetX + numQuads; x++)
				{
					// First triangle
					indexData[index++] = (z * proximitySize + x);
					indexData[index++] = ((z + 1) * proximitySize + x);
					indexData[index++] = ((z + 1) * proximitySize + x + 1);
					// Second triangle
					indexData[index++] = (z * proximitySize + x);
					indexData[index++] = ((z + 1) * proximitySize + x + 1);
					indexData[index++] = (z * proximitySize + x + 1);
				}
			}
			vertOffsetX = (vertOffsetX + numQuads) % (proximitySize - 1);
		}
		vertOffsetZ += numQuads;
	}


	return indexData;
}

int Heightmap::getRegionCount()
{
	return this->regionCount;
}

int Heightmap::getRegionWidthCount()
{
	return this->regionWidthCount;
}

int Heightmap::getRegionSize()
{
	return this->regionSize;
}

int Heightmap::getIndiciesPerRegion()
{
	return this->indiciesPerRegion;
}

int Heightmap::getProximityRegionCount()
{
	int width = getProximityWidthRegionCount();
	return width*width;
}

int Heightmap::getProximityWidthRegionCount()
{
	return (1 + this->proxDim * 2);
}

int Heightmap::getWidth()
{
	return this->heightmapWidth;
}

int Heightmap::getHeight()
{
	return this->heightmapHeight;
}
