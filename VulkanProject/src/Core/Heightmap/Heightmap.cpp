#include "jaspch.h"
#include "Heightmap.h"

#include "stb/stb_image.h"

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

void Heightmap::loadImage(const std::string& path)
{
	int width, height;
	int channels;
	unsigned char* img = static_cast<unsigned char*>(stbi_load(path.c_str(), &width, &height, &channels, 1));
	ERROR_CHECK(img == nullptr, "Failed to load heightmap, could find file!");
	JAS_INFO("Loaded heightmap: {path} successfully!");
	
	HeightmapVertexData newVertexData = {};
	newVertexData.width = width;
	newVertexData.height = height;

	const unsigned maxValue = 255;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			float val = this->minZ + ((float)img[i * width + j] / maxValue) * (this->maxZ - this->minZ);
			float height = -std::min(this->maxZ, val * this->vertDist + this->minZ);
			newVertexData.verticies.push_back(glm::vec4(j * this->vertDist, height, i * this->vertDist, 1.0));
		}
	}

	for (int i = 0; i < height - 1; i++)
	{
		for (int j = 0; j < width - 1; j++)
		{
			// First triangle
			newVertexData.indicies.push_back(i * width + j);
			newVertexData.indicies.push_back((i + 1) * width + j);
			newVertexData.indicies.push_back((i + 1) * width + j + 1);
			// Second triangle
			newVertexData.indicies.push_back(i * width + j);
			newVertexData.indicies.push_back((i + 1) * width + j + 1);
			newVertexData.indicies.push_back(i * width + j + 1);
		}
	}

	this->vertexData.push_back(newVertexData);

	size_t size = width * height * sizeof(unsigned char);
	HeightmapRawData newRawData = { width, height, size, img };
	this->rawData.push_back(newRawData);
}

void Heightmap::cleanup()
{
	for (auto& data : this->rawData)
		delete data.data;
}
