#include "jaspch.h"
#include "Heightmap.h"


Heightmap::Heightmap()
	: minZ(0),
	maxZ(1),
	vertDist(0.1),
	origin(0.f, 0.f, 0.f),
	regionCountX(0),
	regionCountZ(0),
	regionSize(2),
	proxDim(1),
	rawData(),
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

	this->heightmapWidth = dataWidth;
	this->heightmapHeight = dataHeight;

	int regionCountWidth = 1 + this->proxDim * 2;
	this->proxVertDim = this->regionSize * regionCountWidth - regionCountWidth + 1;

	int vertOffsetX = 0;
	int vertOffsetY = 0;

	int B = dataWidth;
	int b = regionSize;
	this->regionCountX = this->regionCountZ = static_cast<int>(ceilf(((B - 1) / (float)(b - 1))));

	int padX = 0;
	int padY = 0;
	padX = padY = this->regionCountX * (b-1) + 1 - dataWidth;

	int paddedSizeX = dataWidth + padX;
	int paddedSizeZ = dataHeight + padY;

	const unsigned maxValue = 255;
	for (int i = 0; i < paddedSizeZ; i++)
	{
		for (int j = 0; j < paddedSizeX; j++)
		{
			float height = 0;
			if (i < dataHeight && j < dataWidth)
				height = this->minZ + ((float)data[i * dataWidth + j] / maxValue) * (abs(this->maxZ - this->minZ));

			float xPos = this->origin.x + j * this->vertDist;
			float zPos = this->origin.z + i * this->vertDist;

			this->verticies.push_back(glm::vec4(xPos, height, zPos, 1.0));
		}
	}

	//B = dataHeight;
	//this->regionCountZ = static_cast<int>(ceilf(((B - b) / (float)(b - 1)) + 1)); // Must be changed
	
	const int numQuads = regionSize - 1;
	for (int y = 0; y < this->regionCountX; y++)
	{
		for (int x = 0; x < this->regionCountZ; x++)
		{
			for (int i = vertOffsetY; i < vertOffsetY + numQuads; i++)
			{
				for (int j = vertOffsetX; j < vertOffsetX + numQuads; j++)
				{
					this->indicies.push_back(i * paddedSizeX + j);
					this->indicies.push_back((i + 1) * paddedSizeX + j);
					this->indicies.push_back((i + 1) * paddedSizeX + j + 1);
					// Second triangle
					this->indicies.push_back(i * paddedSizeX + j);
					this->indicies.push_back((i + 1) * paddedSizeX + j + 1);
					this->indicies.push_back(i * paddedSizeX + j + 1);
				}
			}
			vertOffsetX = (vertOffsetX + numQuads) % (paddedSizeX - 1);
		}
		vertOffsetY += numQuads;
	}

	//size_t size = dimX * dimY * sizeof(unsigned char);
	HeightmapRawData newRawData = { paddedSizeX, paddedSizeZ, dataWidth * dataHeight * sizeof(int), data };
	this->rawData = newRawData;
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
int Heightmap::getProximityVertexDim()
{
	return this->proxVertDim;
}
void Heightmap::getProximityVerticies(const glm::vec3& position, std::vector<glm::vec4>& verticies)
{
	float xDistance = position.x - this->origin.x;
	float zDistance = position.z - this->origin.z;

	// Calculate region index from position
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
					int offset = xTemp + (zTemp)*this->heightmapWidth;
					verticies[i + j * this->proxVertDim] = this->verticies[offset];
				}
			}
		}
	}
}

std::vector<unsigned> Heightmap::getProximityIndicies(const glm::vec3& position)
{
	float worldSize = (this->regionSize - 1) * this->vertDist;

	float xDistance = position.x - this->origin.x;
	float zDistance = position.z - this->origin.z;

	std::vector<unsigned int> data;
	if (xDistance >= 0.f &&
		xDistance <= this->regionCountX * worldSize &&
		zDistance >= 0.f &&
		zDistance <= this->regionCountZ * worldSize)
	{
		// Calculate vertex index from position
		int xIndex = static_cast<int>(xDistance / worldSize);
		int zIndex = static_cast<int>(zDistance / worldSize);

 		for (int z = -proxDim; z <= proxDim; z++)
		{
			int zTemp = zIndex + z;
			for (int x = -proxDim; x <= proxDim; x++)
			{
				int xTemp = xIndex + x;
				if (xTemp >= 0 && xTemp < regionCountX && zTemp >= 0 && zTemp < regionCountZ) {
					// Copy over indicies of region to return vector
					unsigned int vecOffset = data.size();
					data.resize(data.size() + this->indiciesPerRegion);

					unsigned int* vecData = data.data();

					int offset = xTemp * this->indiciesPerRegion + zTemp * this->indiciesPerRegion * regionCountZ;
					memcpy((void*)(vecData + vecOffset), (void*)(&this->indicies[offset]), this->indiciesPerRegion * sizeof(unsigned int));
				}
			}
		}
	}

	return data;
}

const std::vector<glm::vec4>& Heightmap::getVerticies()
{
	return this->verticies;
}

const std::vector<unsigned>& Heightmap::getIndicies()
{
	return this->indicies;
}

std::vector<unsigned> Heightmap::generateIndicies(int proximitySize, int regionSize)
{
	std::vector<unsigned> indexData;

	const int numQuads = proximitySize - 1;

	int B = proximitySize;
	int b = regionSize;
	int regionCount = static_cast<int>(ceilf(((B - 1) / (float)(b - 1))));

	int vertOffsetX = 0;
	int vertOffsetZ = 0;

	for (int j = 0; j < regionCount; j++)
	{
		for (int i = 0; i < regionCount; i++)
		{
			for (int z = vertOffsetZ; z < vertOffsetZ + numQuads; z++)
			{
				for (int x = vertOffsetX; x < vertOffsetX + numQuads; x++)
				{
					// First triangle
					indexData.push_back(z * proximitySize + x);
					indexData.push_back((z + 1) * proximitySize + x);
					indexData.push_back((z + 1) * proximitySize + x + 1);
					// Second triangle
					indexData.push_back(z * proximitySize + x);
					indexData.push_back((z + 1) * proximitySize + x + 1);
					indexData.push_back(z * proximitySize + x + 1);
				}
			}
			vertOffsetX = (vertOffsetX + numQuads) % (proximitySize - 1);
		}
		vertOffsetZ += numQuads;
	}

	return indexData;
}

int Heightmap::getRegionSize()
{
	return this->regionSize;
}

int Heightmap::getIndiciesPerRegion()
{
	return this->indiciesPerRegion;
}

void Heightmap::cleanup()
{
	delete[] this->rawData.data;
}
