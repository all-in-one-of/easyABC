#include <string>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "AbcReader.h"
#include "AbcWriter.h"

int main(int argc, char* argv[])
{
	std::string inputMeshName("testGeo/non_animated.abc");
	std::string outputMeshName("testGeo/non_animated_copy.abc");
	std::string xFormName("gabc");
	std::string meshName("gabc");
	std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>> customProperties;
	customProperties.emplace_back("Cd", FLOAT, POINT);
	customProperties.emplace_back("noise", FLOAT, POINT);
	customProperties.emplace_back("vector_noise", VECTOR, POINT);

	//1. Read Alembic Archive
	AbcReader inputMesh;
	inputMesh.openArchive(inputMeshName, xFormName, meshName, customProperties);

	//Access efficiently:
	/*
	const std::vector<float>& noise = inputMesh.getFloatProperty("noise");
	const std::vector<float>& Cd = inputMesh.getFloatProperty("Cd");
	const std::vector<Alembic::Abc::V3f>& vector_noise = inputMesh.getVectorProperty("vector_noise");
	*/

	//copying works like this:
	std::vector<std::vector<float>> floatProps;
	std::vector<std::vector<Alembic::Abc::V3f>> vectorProps;

	floatProps.push_back(inputMesh.getFloatProperty("noise"));
	floatProps.push_back(inputMesh.getFloatProperty("Cd"));
	vectorProps.push_back(inputMesh.getVectorProperty("vector_noise"));

	
	const std::vector<Alembic::Abc::V3f>& points = inputMesh.getPositions();
	const std::vector<int>& faceIndices = inputMesh.getFaceIndices();
	const std::vector<int>& faceCounts = inputMesh.getFaceCounts();
	const std::vector<Alembic::Abc::V3f>& normals = inputMesh.getNormals();

	//now write them again
	AbcWriter outputMesh(outputMeshName, xFormName, meshName, customProperties);
	outputMesh.addSample(points, faceIndices, faceCounts, normals, PROP_SCOPE::VERTEX,
		floatProps,
		vectorProps);
}