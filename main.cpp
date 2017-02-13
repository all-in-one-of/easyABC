#include <string>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

#include "AbcReader.h"
#include "AbcWriter.h"

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cout << "Please provide: surface mesh name, alembic archive name, target alembic archive name!" << std::endl;
		return 0;
	}

	std::string inputMeshName(argv[1]);
	std::string outputMeshName(argv[2]);

	//1. Read Alembic Archive
	AbcReader inputMesh;
	inputMesh.openArchive(inputMeshName);

	//2. Write Alembic Archive
	AbcWriter outputMesh(outputMeshName, "animatedMesh");
	std::vector<Imath::V3f> vertexPositions;
	std::vector<int> faceIndices;
	std::vector<int> faceCounts;


	std::cout << faceIndices.size() << " Face Indices Recorded." << std::endl;
	std::cout << faceCounts.size() << " Face Counts Recorded." << std::endl;


	std::cout << "Finished IO, converting sequence..." << std::endl;


	for (int s = 0; s < inputMesh.getNumSamples(); ++s)
	{

	}

	std::cout << "Tidying up, ..." << std::endl;
}