#include <string>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "AbcReader.h"
#include "AbcWriter.h"

int main(int argc, char* argv[])
{
	/*if (argc < 4)
	{
		std::cout << "Please provide: surface mesh name, alembic archive name, target alembic archive name!" << std::endl;
		return 0;
	}

	std::string inputMeshName(argv[1]);
	std::string outputMeshName(argv[2]);
	*/
	std::string inputMeshName("testGeo/non_animated.abc");
	std::string xFormName("gabc");
	std::string meshName("gabc");
	std::vector<std::tuple<std::string, AbcReader::PROP_TYPE, AbcReader::PROP_SCOPE>> customProperties;
	customProperties.emplace_back("Cd", AbcReader::FLOAT, AbcReader::POINT);
	customProperties.emplace_back("noise", AbcReader::FLOAT, AbcReader::POINT);
	customProperties.emplace_back("vector_noise", AbcReader::VECTOR, AbcReader::POINT);

	//1. Read Alembic Archive
	AbcReader inputMesh;
	std::cout << "Opening input mesh..." << std::endl;
	inputMesh.openArchive(inputMeshName, xFormName, meshName, customProperties);

	const std::vector<float>& noise = inputMesh.getFloatProperty("noise");
	const std::vector<float>& Cd = inputMesh.getFloatProperty("Cd");
	const std::vector<Eigen::Vector3f>& vector_noise = inputMesh.getVectorProperty("vector_noise");

	const std::vector<Eigen::Vector3f>& points = inputMesh.getPositions();
	const std::vector<int>& faceIndices = inputMesh.getFaceIndices();
	const std::vector<int>& faceCounts = inputMesh.getFaceCounts();

	//2. Write Alembic Archive
	/*AbcWriter outputMesh(outputMeshName, "animatedMesh");
	std::vector<Imath::V3f> vertexPositions;
	std::vector<int> faceIndices;
	std::vector<int> faceCounts;


	std::cout << faceIndices.size() << " Face Indices Recorded." << std::endl;
	std::cout << faceCounts.size() << " Face Counts Recorded." << std::endl;


	std::cout << "Finished IO, converting sequence..." << std::endl;


	for (int s = 0; s < inputMesh.getNumSamples(); ++s)
	{

	}

	std::cout << "Tidying up, ..." << std::endl;*/
}