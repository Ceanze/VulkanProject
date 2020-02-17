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
	for (Mesh& mesh : this->meshes)
	{
		mesh.indices.cleanup();
		for (auto& attrib : mesh.attributes)
			attrib.second.cleanup();
		mesh.bufferMemory.cleanup();
	}

	if(hasImageMemory)
		this->imageMemory.cleanup();
	if(hasMaterialMemory)
		this->materialMemory.cleanup();
}
