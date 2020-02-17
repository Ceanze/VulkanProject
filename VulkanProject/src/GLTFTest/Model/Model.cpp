#include "jaspch.h"
#include "Model.h"

Model::Model()
{

}

Model::~Model()
{

}

void Model::cleanup()
{
	if(this->indices.empty() == false)
		this->indexBuffer.cleanup();
	if (this->vertices.empty() == false)
		this->vertexBuffer.cleanup();

	if(this->indices.empty() == false && this->vertices.empty() == false)
		this->bufferMemory.cleanup();

	if(hasImageMemory)
		this->imageMemory.cleanup();
	if(hasMaterialMemory)
		this->materialMemory.cleanup();
}
