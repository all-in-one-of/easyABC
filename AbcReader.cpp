#include "AbcReader.h"

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreFactory/All.h>

struct AbcReaderImp
{
	std::shared_ptr<Alembic::Abc::IArchive> archive;
	std::shared_ptr<Alembic::AbcGeom::IPolyMesh> mesh;
	std::shared_ptr<Alembic::AbcGeom::IXform> transform;

	int numSamples;
	int currentSample;
};

AbcReader::AbcReader()
{
	m_data = std::make_shared<AbcReaderImp>();
}


AbcReader::~AbcReader()
{
}


std::vector<float>& AbcReader::getFloatProperty(const std::string& name)
{
	return m_arbGeoFloatProperties[m_arbGeoPropertiesMap[name]];
}

std::vector<Alembic::Abc::V3f>& AbcReader::getVectorProperty(const std::string& name)
{
	return m_arbGeoVectorProperties[m_arbGeoPropertiesMap[name]];
}



bool
AbcReader::openArchive(const std::string& file, const std::string& xFormName, const std::string& meshName,
	const std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>& arbGeoProperties)
{
	Alembic::AbcCoreFactory::IFactory factory;
	factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);
	Alembic::AbcCoreFactory::IFactory::CoreType coreType;

	//Open archive
	m_data->archive =
		std::make_shared<Alembic::Abc::IArchive>(factory.getArchive(file, coreType));
	
	//get the top node
	Alembic::Abc::IObject topObject(*m_data->archive, Alembic::Abc::kTop);

	//get the transform
	m_data->transform =
		std::make_shared<Alembic::AbcGeom::IXform>(Alembic::AbcGeom::IObject(topObject, xFormName), Alembic::AbcGeom::kWrapExisting);

	//get the mesh
	m_data->mesh = std::make_shared<Alembic::AbcGeom::IPolyMesh>(Alembic::AbcGeom::IObject(*m_data->transform, meshName), Alembic::AbcGeom::kWrapExisting);

	//get mesh properties
	Alembic::AbcGeom::IPolyMeshSchema& schema = m_data->mesh->getSchema();

	//build internal dicationary
	m_numArbGeoFloatProps = 0;
	m_numArbGeoVectorProps = 0;
	for(auto& p : arbGeoProperties)
	{
		if(std::get<1>(p) == PROP_TYPE::FLOAT)
		{
			m_arbGeoPropertiesMap.emplace(std::make_pair(std::get<0>(p), m_numArbGeoFloatProps));
			++m_numArbGeoFloatProps;
		}
		else if(std::get<1>(p) == PROP_TYPE::VECTOR)
		{
			m_arbGeoPropertiesMap.emplace(std::make_pair(std::get<0>(p), m_numArbGeoVectorProps));
			++m_numArbGeoVectorProps;
		}
		else
		{
			std::cout << "ERROR: Unrecognised Property Type. This will not be read! (" << std::get<0>(p) << ")" << std::endl;
		}
	}

	//print some debug stuff
	std::cout << "Num Poly Mesh Schema Samples Read From file: " << schema.getNumSamples() << std::endl;
	m_data->numSamples = schema.getNumSamples();

	//make sure to read the first sample into memory
	sampleSpecific(0);

	return true;
}

void
AbcReader::readCurrentSampleIntoMemory()
{
	//create a sample selector
	Alembic::AbcGeom::ISampleSelector sampleSelector((Alembic::Abc::index_t)m_data->currentSample);

	//PREDEFINED_PROPERTIES-----------------------------------------------------
	Alembic::AbcGeom::IPolyMeshSchema::Sample sample;

	m_data->mesh->getSchema().get(sample, sampleSelector);

	//1. Positions
	int numPositions = sample.getPositions()->size();
	m_positions.resize(numPositions);
	for (int i = 0; i < numPositions; ++i)
	{
		m_positions[i][0] = (*sample.getPositions())[i][0];
		m_positions[i][1] = (*sample.getPositions())[i][1];
		m_positions[i][2] = (*sample.getPositions())[i][2];
	}

	//2. FaceIndices
	int numFaceIndices = sample.getFaceIndices()->size();
	m_faceIndices.resize(numFaceIndices);
	Alembic::Abc::Int32ArraySamplePtr faceIndices = sample.getFaceIndices();
	for (int i = 0; i < numFaceIndices; ++i)
	{
		m_faceIndices[i] = (*faceIndices)[i];
	}

	//3. FaceCounts
	int numFaceCounts = sample.getFaceCounts()->size();
	m_faceCounts.resize(numFaceCounts);
	Alembic::Abc::Int32ArraySamplePtr faceCounts = sample.getFaceCounts();
	for (int i = 0; i < numFaceCounts; ++i)
	{
		m_faceCounts[i]= (*faceCounts)[i];
	}

	//std::cout << "Reading Normals..." << std::endl;
	//NORMALS -----------------------------------------------------------------
	for(size_t i = 0; i < m_data->mesh->getSchema().getNumProperties(); ++i)
	{
		const Alembic::AbcGeom::PropertyHeader& header = m_data->mesh->getSchema().getPropertyHeader(i);
		const std::string& name = header.getName();

		if(name == "N")
		{
			Alembic::AbcGeom::IV3fGeomParam param(m_data->mesh->getSchema(), name);
			Alembic::AbcGeom::IV3fGeomParam::prop_type::sample_ptr_type valuePtr = param.getExpandedValue(sampleSelector).getVals();

			m_normals.resize(valuePtr->size());
			for(size_t j = 0; j < valuePtr->size(); ++j)
			{
				m_normals[j][0] = valuePtr->get()[j][0];
				m_normals[j][1] = valuePtr->get()[j][1];
				m_normals[j][2] = valuePtr->get()[j][2];
			}
		}
	}

	//std::cout << "Parsing Custom Params..." << std::endl;
	//CUSTOM PROPERTIES--------------------------------------------------------
	Alembic::AbcGeom::ICompoundProperty arbGeomPs = m_data->mesh->getSchema().getArbGeomParams();
	//loop over properties, find the ones we want (this is a safety to make sure properties exist!)
	//could lose the loop easily for clarity and performance
	std::vector<Alembic::AbcGeom::IFloatGeomParam::prop_type::sample_ptr_type> floatSampleVals(m_numArbGeoFloatProps);
	std::vector<Alembic::AbcGeom::IV3fGeomParam::prop_type::sample_ptr_type> vectorSampleVals(m_numArbGeoVectorProps);
	for(size_t i = 0; i < arbGeomPs.getNumProperties(); ++i)
	{
		const Alembic::AbcGeom::PropertyHeader& header = arbGeomPs.getPropertyHeader(i);
		const std::string& name = header.getName();

		for(auto& p_map : m_arbGeoPropertiesMap)
		{
			if(p_map.first == name)
			{
				if(Alembic::AbcGeom::IFloatGeomParam::matches(header))
				{
					Alembic::AbcGeom::IFloatGeomParam param(arbGeomPs, name);
					floatSampleVals[p_map.second] = param.getExpandedValue(sampleSelector).getVals();
					//std::cout << "Found float param: " << name << std::endl;
				}
				else if (Alembic::AbcGeom::IV3fGeomParam::matches(header) || Alembic::AbcGeom::IC3fGeomParam::matches(header))
				{
					Alembic::AbcGeom::IV3fGeomParam param(arbGeomPs, name);
					vectorSampleVals[p_map.second] = param.getExpandedValue(sampleSelector).getVals();
					//std::cout << "Found vector param: " << name << std::endl;
				}
			}
		}
	}

	//std::cout << "Reading floats params..." << std::endl;
	//now parse values

	//for float properties
	m_arbGeoFloatProperties.resize(floatSampleVals.size());
	for(size_t i = 0; i < floatSampleVals.size(); ++i)
	{
		m_arbGeoFloatProperties[i].resize(floatSampleVals[i]->size());
		for(size_t j = 0; j < floatSampleVals[i]->size(); ++j)
		{
			m_arbGeoFloatProperties[i][j] = floatSampleVals[i]->get()[j];
		}
	}

	//std::cout << "Reading Vector params..." << std::endl;	
	//for vector properties
	m_arbGeoVectorProperties.resize(vectorSampleVals.size());
	for(size_t i = 0; i < vectorSampleVals.size(); ++i)
	{
		m_arbGeoVectorProperties[i].resize(vectorSampleVals[i]->size());
		for(size_t j = 0; j < vectorSampleVals[i]->size(); ++j)
		{
			m_arbGeoVectorProperties[i][j][0] = vectorSampleVals[i]->get()[j][0];
			m_arbGeoVectorProperties[i][j][1] = vectorSampleVals[i]->get()[j][1];
			m_arbGeoVectorProperties[i][j][2] = vectorSampleVals[i]->get()[j][2];
		}
	}
}

bool
AbcReader::sampleForward()
{
	if (m_data->currentSample < m_data->numSamples)
	{
		m_data->currentSample += 1;
		readCurrentSampleIntoMemory();
		return true;
	}
	else
	{
		return false;
	}
}

bool
AbcReader::sampleBackward()
{
	if (m_data->currentSample >= 0)
	{
		m_data->currentSample -= 1;
		readCurrentSampleIntoMemory();
		return true;
	}
	else
	{
		return false;
	}
}

bool
AbcReader::sampleSpecific(int sample)
{
	if (sample >= 0 && sample < m_data->numSamples)
	{
		m_data->currentSample = sample;
		readCurrentSampleIntoMemory();
		return true;
	}
	else
	{
		return false;
	}
}

int
AbcReader::getNumSamples()
{
	return m_data->numSamples;
}