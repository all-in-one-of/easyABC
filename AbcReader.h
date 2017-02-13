#pragma once

#include <string>
#include <vector>
#include <tuple>

#include <Alembic/Abc/All.h>

#include "easyAbcUtil.h"

struct AbcReaderImp;

class AbcReader
{
public:
	AbcReader();
	~AbcReader();

	bool openArchive(const std::string& file, const std::string& xFormName, const std::string& meshName,
		const std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>& arbGeoProperties);

	bool sampleForward();
	bool sampleBackward();
	bool sampleSpecific(int sample);

	int getNumSamples();

	int getNumFaces() { return m_faceCounts.size(); }

	//Data Accessors
	std::vector<Alembic::Abc::V3f>& getPositions() { return m_positions; }
	std::vector<int>& getFaceIndices() { return m_faceIndices; }
	std::vector<int>& getFaceCounts() { return m_faceCounts; }
	std::vector<Alembic::Abc::V3f>& getNormals() { return m_normals; }

	std::vector<float>& getFloatProperty(const std::string& name);
	std::vector<Alembic::Abc::V3f>& getVectorProperty(const std::string& name);

private:

	void readCurrentSampleIntoMemory();

	std::shared_ptr<AbcReaderImp> m_data;

	std::vector<Alembic::Abc::V3f> m_positions;
	std::vector<Alembic::Abc::V3f> m_velocities;
	std::vector<int> m_faceIndices;
	std::vector<int> m_faceCounts;
	std::vector<Alembic::Abc::V3f> m_normals;
	//for arbitrary float properties added to the mesh
	std::unordered_map<std::string, size_t> m_arbGeoPropertiesMap;
	size_t m_numArbGeoFloatProps;
	size_t m_numArbGeoVectorProps;
	std::vector<std::vector<float>> m_arbGeoFloatProperties;
	std::vector<std::vector<Alembic::Abc::V3f>> m_arbGeoVectorProperties;
};

