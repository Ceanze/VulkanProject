#include "jaspch.h"
#include "HeightmapGen.h"

HeightmapGen::HeightmapGen()
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

HeightmapGen::~HeightmapGen()
{
}

void HeightmapGen::init(const glm::vec3& origin, int regionSize, int dataWidth, int dataHeight, unsigned char* data)
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
	padX = padY = this->regionWidthCount * (b - 1) + 1 - dataWidth;

	this->heightmapWidth = dataWidth + padX;
	this->heightmapHeight = dataHeight + padY;
	this->verticies.resize(this->heightmapWidth * this->heightmapHeight);
}

void HeightmapGen::setMinZ(float val)
{
	this->minZ = minZ;
}

void HeightmapGen::setMaxZ(float value)
{
	this->maxZ = value;
}

void HeightmapGen::setVertexDist(float value)
{
	this->vertDist = value;
}

void HeightmapGen::setProximitySize(int size)
{
	this->proxDim = size;
}

glm::ivec2 HeightmapGen::getRegionFromPos(const glm::vec3 & position)
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

int HeightmapGen::getProximityVertexDim()
{
	return this->proxVertDim;
}

void HeightmapGen::getProximityVerticies(const glm::vec3 & position, std::vector<Vertex> & verticies)
{
	float xDistance = position.x - this->origin.x;
	float zDistance = position.z - this->origin.z;

	// Calculate vertex index from position
	int xIndex = static_cast<int>(xDistance / this->vertDist);
	int zIndex = static_cast<int>(zDistance / this->vertDist);

	// Handle both odd and even vertex count on height map
	if (this->proxVertDim % 2 != 0) {
		// Odd
		xIndex -= this->proxVertDim / 2;
		zIndex -= this->proxVertDim / 2;
	}
	else {
		// Even
		xIndex -= (this->proxVertDim / 2) - 1;
		zIndex -= (this->proxVertDim / 2) - 1;
	}

	// Return verticies within proximity off position. {UNPADDED VERSION}
	for (int j = 0; j < this->proxVertDim; j++)
	{
		const float vd = this->vertDist;
		int z = zIndex + j;
		for (int i = 0; i < this->proxVertDim; i++)
		{
			int x = xIndex + i;
			float xPos = this->origin.x + x * vd;
			float zPos = this->origin.z + z * vd;

			float mid	= getHeight(xPos, zPos);
			Vertex vert;
			vert.position = glm::vec4(xPos, mid, zPos, 1.0);
			verticies[i + j * this->proxVertDim] = vert;
		}
	}

	// Add normals
	for (int j = 0; j < this->proxVertDim; j++)
	{
		const float vd = this->vertDist;
		int z = zIndex + j;
		for (int i = 0; i < this->proxVertDim; i++)
		{
			int x = xIndex + i;
			float xPos = this->origin.x + x * vd;
			float zPos = this->origin.z + z * vd;

			// Fetch height from vertex if not at the border.
			float east  = (i + 1) < this->proxVertDim ? verticies[(i+1) + j * this->proxVertDim].position.y : getHeight(xPos+vd, zPos);
			float west  = (i - 1) >= 0				  ? verticies[(i-1) + j * this->proxVertDim].position.y : getHeight(xPos - vd, zPos);
			float north = (j + 1) < this->proxVertDim ? verticies[i + (j+1) * this->proxVertDim].position.y : getHeight(xPos, zPos + vd);
			float south = (j - 1) >= 0			      ? verticies[i + (j-1) * this->proxVertDim].position.y : getHeight(xPos, zPos - vd);
			verticies[i + j * this->proxVertDim].normal = glm::normalize(glm::vec3(west - east, 2.f, south - north));
		}
	}
}

const std::vector<HeightmapGen::Vertex>& HeightmapGen::getVerticies()
{
	return this->verticies;
}

const std::vector<unsigned>& HeightmapGen::getIndicies()
{
	return this->indicies;
}

int HeightmapGen::getVerticiesSize()
{
	return this->verticies.size();
}

int HeightmapGen::getIndiciesSize()
{
	return this->indicies.size();
}

int HeightmapGen::getProximityIndiciesSize()
{
	const int numQuads = this->regionSize - 1;
	int B = this->proxVertDim;
	int b = this->regionSize;
	int regionCount = static_cast<int>(ceilf(((B - 1) / (float)(b - 1))));
	uint32_t size = regionCount * regionCount * numQuads * numQuads * 6;
	return size;
}

int HeightmapGen::getRegionCount()
{
	return this->regionCount;
}

int HeightmapGen::getRegionWidthCount()
{
	return this->regionWidthCount;
}

int HeightmapGen::getRegionSize()
{
	return this->regionSize;
}

int HeightmapGen::getIndiciesPerRegion()
{
	return this->indiciesPerRegion;
}

int HeightmapGen::getProximityRegionCount()
{
	int width = getProximityWidthRegionCount();
	return width * width;
}

int HeightmapGen::getProximityWidthRegionCount()
{
	return (1 + this->proxDim * 2);
}

int HeightmapGen::getWidth()
{
	return this->heightmapWidth;
}

int HeightmapGen::getHeight()
{
	return this->heightmapHeight;
}

float HeightmapGen::getHeight(float x, float y) const
{
	auto random = [](glm::vec2 st) {
		return glm::fract(sin(glm::dot(st, glm::vec2(12.9898, 78.233))) * 43758.5453123);
	};

	auto noise = [&](glm::vec2 st) {
		glm::vec2 i = glm::floor(st);
		glm::vec2 f = glm::fract(st);

		// Four corners in 2D of a tile
		float a = random(i);
		float b = random(i + glm::vec2(1.0f, 0.0f));
		float c = random(i + glm::vec2(0.0f, 1.0f));
		float d = random(i + glm::vec2(1.0f, 1.0f));

		glm::vec2 u = f * f * (3.0f - 2.0f * f);

		return glm::mix(a, b, u.x) +
			(c - a) * u.y * (1.0f - u.x) +
			(d - b) * u.x * u.y;
	};

	#define OCTAVES 3
	auto fbm = [&](glm::vec2 st) {
		// Initial values
		float value = 0.0f;
		float amplitude = .5f;
		float frequency = 0.f;
		
		// Loop of octaves
		for (int i = 0; i < OCTAVES; i++) {
			value += amplitude * noise(st);
			st *= 2.f;
			amplitude *= .5f;
		}
		return value;
	};

	float dist = glm::abs(this->maxZ - this->minZ);
	return this->minZ + fbm(glm::vec2{x, y }*0.01f) * dist;
}
