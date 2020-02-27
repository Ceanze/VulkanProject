#include "jaspch.h"
#include "Heightmap.h"


Heightmap::Heightmap()
	: minZ(0), maxZ(1), vertDist(0.5)
{
}

Heightmap::~Heightmap()
{
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

Heightmap::HeightmapVertexData Heightmap::getVertexData(unsigned index)
{
	return this->vertexData[index];
}

Heightmap::HeightmapRawData Heightmap::getData(unsigned index)
{
	return this->rawData[index];
}

void Heightmap::addHeightmap(int dimX, int dimY, unsigned char* data)
{
	HeightmapVertexData newVertexData = {};
	newVertexData.dimX = dimX;
	newVertexData.dimY = dimY;

	const unsigned maxValue = 255;

	for (int i = 0; i < dimY; i++)
	{
		for (int j = 0; j < dimX; j++)
		{
			float val = this->minZ + ((float)data[i * dimX + j] / maxValue) * (this->maxZ - this->minZ);
			float height = -std::min(this->maxZ, val * this->vertDist + this->minZ);
			newVertexData.verticies.push_back(glm::vec4(j * this->vertDist, height, i * this->vertDist, 1.0));
		}
	}

	for (int i = 0; i < dimY - 1; i++)
	{
		for (int j = 0; j < dimX - 1; j++)
		{
			// First triangle
			newVertexData.indicies.push_back(i * dimX + j);
			newVertexData.indicies.push_back((i + 1) * dimX + j);
			newVertexData.indicies.push_back((i + 1) * dimX + j + 1);
			// Second triangle
			newVertexData.indicies.push_back(i * dimX + j);
			newVertexData.indicies.push_back((i + 1) * dimX + j + 1);
			newVertexData.indicies.push_back(i * dimX + j + 1);
		}
	}

	this->vertexData.push_back(newVertexData);

	size_t size = dimX * dimY * sizeof(unsigned char);
	HeightmapRawData newRawData = { dimX, dimY, size, data };
	this->rawData.push_back(newRawData);
}

void Heightmap::cleanup()
{
	for (auto& data : this->rawData)
		delete data.data;
}
