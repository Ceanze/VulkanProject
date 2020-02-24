#include "jaspch.h"
#include "Heightmap.h"

#include "stb/stb_image.h"

Heightmap::Heightmap()
{
}

Heightmap::~Heightmap()
{
}

Heightmap::HeightmapData Heightmap::getData(unsigned index)
{
	return this->data[index];
}

void Heightmap::loadImage(const std::string& path)
{
	int width, height;
	int channels;
	unsigned char* img = static_cast<unsigned char*>(stbi_load(path.c_str(), &width, &height, &channels, 1));

	size_t size = width * height * sizeof(unsigned char);
	HeightmapData newHeightmap = { width, height, size, img };
	this->data.push_back(newHeightmap);
	JAS_INFO("Loaded heightmap: {path} successfully!");
}

void Heightmap::cleanup()
{
	for (auto& data : this->data)
		delete data.data;
}
