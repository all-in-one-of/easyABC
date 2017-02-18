#include "AbcWriter.h"

#include <iostream>

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreFactory/All.h>


struct AbcWriterImp
{
	std::shared_ptr<Alembic::AbcGeom::OPolyMesh> mesh;
	std::shared_ptr<Alembic::AbcGeom::OXform> transform;
};

void AbcWriter::setupObject(const std::string& xFormName, const std::string& meshName,
		const std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>& arbGeoProperties)
{
	m_data.emplace_back(std::make_shared<AbcWriterImp>());
	m_data.back()->transform = std::make_shared<Alembic::AbcGeom::OXform>(Alembic::Abc::OObject(m_archive->getTop()), xFormName);
	m_data.back()->mesh = std::make_shared<Alembic::AbcGeom::OPolyMesh>(*m_data.back()->transform, meshName);

	m_floatParams.push_back(std::vector<Alembic::AbcGeom::OFloatGeomParam>{});
	m_vectorParams.push_back(std::vector<Alembic::AbcGeom::OV3fGeomParam>{});
	m_colourParams.push_back(std::vector<Alembic::AbcGeom::OC3fGeomParam>{});
	m_arbScopes.push_back(std::vector<Alembic::AbcGeom::GeometryScope>{});
	m_arbNames.push_back(std::vector<std::string>{});

	Alembic::AbcGeom::OCompoundProperty arbGeomPs = m_data.back()->mesh->getSchema().getArbGeomParams();
	//create custom properties
	std::vector<std::string> vectorPropNames;
	std::vector<Alembic::AbcGeom::GeometryScope> vectorPropScopes;
	for(auto& p : arbGeoProperties)
	{
		Alembic::AbcGeom::GeometryScope scope;
		if(std::get<2>(p) == POINT)
		{
			scope = Alembic::AbcGeom::GeometryScope::kVaryingScope;
		}
		else if(std::get<2>(p) == VERTEX)
		{
			scope = Alembic::AbcGeom::GeometryScope::kVertexScope;
		}
		else if(std::get<2>(p) == FACE)
		{
			scope = Alembic::AbcGeom::GeometryScope::kFacevaryingScope;
		}
		else
		{
			std::cout << "ERROR: Unknown scope detected, this may crash. (" << std::get<0>(p) << ")" << std::endl;
		}

		if(std::get<1>(p) == PROP_TYPE::FLOAT)
		{
			m_floatParams.back().emplace_back(arbGeomPs, std::get<0>(p), false, scope, 1); 
			m_arbScopes.back().push_back(scope);
			m_arbNames.back().push_back(std::get<0>(p));
		}
		else if(std::get<1>(p) == PROP_TYPE::VECTOR)
		{
			if(std::get<0>(p) == "Cd")
			{
				m_colourParams.back().emplace_back(arbGeomPs, std::get<0>(p), false, scope, 1);
			}
			else
			{
				m_vectorParams.back().emplace_back(arbGeomPs, std::get<0>(p), false, scope, 1);
			}
			vectorPropScopes.push_back(scope);
			vectorPropNames.push_back(std::get<0>(p));
		}
		else
		{
			std::cout << "ERROR: Unrecognised Property Type. This will not be created! (" << std::get<0>(p) << ")" << std::endl;
			continue;
		}
	}

	m_arbNames.back().insert(m_arbNames.back().end(), vectorPropNames.begin(), vectorPropNames.end());
	m_arbScopes.back().insert(m_arbScopes.back().end(), vectorPropScopes.begin(), vectorPropScopes.end());
}

AbcWriter::AbcWriter(const std::string& file, const std::string& xFormName, const std::string& meshName,
		const std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>& arbGeoProperties) : m_archiveName(file)
{
	m_objectName.push_back(meshName);
	m_archive= std::make_shared<Alembic::Abc::OArchive>(Alembic::AbcCoreOgawa::WriteArchive(), m_archiveName);

	setupObject(xFormName, meshName, arbGeoProperties);

	if (m_archive->valid())
	{
		m_fileIsOpen = true;
	}
}

AbcWriter::AbcWriter(const std::string& file, const std::vector<std::string>& xFormNames, const std::vector<std::string>& meshNames,
		const std::vector<std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>>& arbGeoProperties) : m_archiveName(file)
{
	m_archive= std::make_shared<Alembic::Abc::OArchive>(Alembic::AbcCoreOgawa::WriteArchive(), m_archiveName);

	for(auto i = 0;  i < meshNames.size(); ++i)
	{
		m_objectName.push_back(meshNames[i]);
		setupObject(xFormNames[i], meshNames[i], arbGeoProperties[i]);
	}
	if (m_archive->valid())
	{
		m_fileIsOpen = true;
	}
}


AbcWriter::~AbcWriter()
{
	if (m_fileIsOpen)
	{
		std::cout << "Closing [ " << m_archive->getName() << " ]" << std::endl;
		for(auto i = 0; i < m_data.size(); ++i)
		{
			std::cout << "# samples saved for mesh " << m_objectName[i] << ": "
			<< m_data[i]->mesh->getSchema().getNumSamples() << std::endl;
		}
	}
}


bool
AbcWriter::addSample(std::vector<Alembic::Abc::V3f>& vertices, std::vector<int>& faceIndices,
	std::vector<int>& faceCounts, size_t meshIdx)
{
	// Make sure that our file is open
	if (!m_fileIsOpen)
	{
		std::cout << "ERROR: Alembic Archive [" <<
			m_archiveName << "] is not open! Sample could not be written." << std::endl;
		return false;
	}

	//get schema
	Alembic::AbcGeom::OPolyMeshSchema& schema = m_data[meshIdx]->mesh->getSchema();

	//create a sample
	Alembic::AbcGeom::OPolyMeshSchema::Sample sample;

	//POSITION
	sample.setPositions(Alembic::Abc::P3fArraySample(&vertices[0], vertices.size()));

	//FACE-INDICES
	sample.setFaceIndices(Alembic::Abc::Int32ArraySample(&faceIndices[0], faceIndices.size()));

	//FACE COUNTS
	sample.setFaceCounts(Alembic::Abc::Int32ArraySample(&faceCounts[0], faceCounts.size()));

	//set mesh sample
	schema.set(sample);

	return true;
}

bool
AbcWriter::addSample(const std::vector<Alembic::Abc::V3f>& vertices,
		const std::vector<int>& faceIndices, const std::vector<int>& faceCounts,
		const std::vector<Alembic::Abc::V3f>& normals, PROP_SCOPE normalsScope,
		const std::vector<std::vector<float>> floatProps,
		const std::vector<std::vector<Alembic::Abc::V3f>> vectorProps,
		size_t meshIdx)
{
	Alembic::AbcGeom::GeometryScope normalScope;
	if(normalsScope == POINT)
	{
		normalScope = Alembic::AbcGeom::GeometryScope::kVaryingScope;
	}
	else if(normalsScope == VERTEX)
	{
		normalScope = Alembic::AbcGeom::GeometryScope::kVertexScope;
	}
	else if(normalsScope == FACE)
	{
		normalScope = Alembic::AbcGeom::GeometryScope::kFacevaryingScope;
	}
	else
	{
		std::cout << "ERROR: Unknown normal scope detected, this may crash." << std::endl;
	}

	// Make sure that our file is open
	if (!m_fileIsOpen)
	{
		std::cout << "ERROR: Alembic Archive [" <<
			m_archiveName << "] is not open! Sample could not be written." << std::endl;
		return false;
	}

	//get schema
	Alembic::AbcGeom::OPolyMeshSchema& schema = m_data[meshIdx]->mesh->getSchema();

	//create a sample
	Alembic::AbcGeom::OPolyMeshSchema::Sample sample;

	//GENERIC------------------------------------------------------------------------
	//POSITION
	sample.setPositions(Alembic::Abc::P3fArraySample(&vertices[0], vertices.size()));

	//FACE-INDICES
	sample.setFaceIndices(Alembic::Abc::Int32ArraySample(&faceIndices[0], faceIndices.size()));

	//FACE COUNTS
	sample.setFaceCounts(Alembic::Abc::Int32ArraySample(&faceCounts[0], faceCounts.size()));

	//NORMALS
	Alembic::AbcGeom::ON3fGeomParam::Sample normalsSamp;
	normalsSamp.setScope(normalScope);
	normalsSamp.setVals(Alembic::AbcGeom::N3fArraySample(&normals.front(), normals.size()));
	sample.setNormals(normalsSamp);

	//CUSTOM------------------------------------------------------------------------
	//std::cout << "Writing Float Props..." << std::endl;
	for(size_t i = 0; i < floatProps.size(); ++i)
	{
		//std::cout << m_arbNames[i] << std::endl;
		Alembic::AbcGeom::OFloatGeomParam::Sample floatSamp;
		floatSamp.setScope(m_arbScopes[meshIdx][i]);
		floatSamp.setVals(Alembic::AbcGeom::FloatArraySample(&floatProps[i].front(), floatProps[i].size()));
		m_floatParams[meshIdx][i].set(floatSamp);
	}

	//std::cout << "Writing Colour Props..." << std::endl;
	size_t colourIdx = 0;
	for(size_t i = 0; i < vectorProps.size(); ++i)
	{
		//std::cout <<  m_arbNames[floatProps.size() + i] << std::endl;
		if(m_arbNames[meshIdx][floatProps.size() + i] == "Cd")
		{
			Alembic::AbcGeom::OC3fGeomParam::Sample vectorSamp;
			vectorSamp.setScope(m_arbScopes[meshIdx][floatProps.size() + i]);
			vectorSamp.setVals(Alembic::AbcGeom::C3fArraySample( (const Imath::C3f *) &vectorProps[i].front(), vectorProps[i].size()));
			m_colourParams[meshIdx][colourIdx].set(vectorSamp);
			++colourIdx;
			continue;
		}
		Alembic::AbcGeom::OV3fGeomParam::Sample vectorSamp;
		vectorSamp.setScope(m_arbScopes[meshIdx][floatProps.size() + i + colourIdx]);
		vectorSamp.setVals(Alembic::AbcGeom::V3fArraySample(&vectorProps[i].front(), vectorProps[i].size()));
		m_vectorParams[meshIdx][i - colourIdx].set(vectorSamp);
	}

	//set mesh sample
	schema.set(sample);

	return true;
}
