#include "AbcWriter.h"

#include <iostream>

#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreFactory/All.h>


struct AbcWriterImp
{
	std::shared_ptr<Alembic::Abc::OArchive> archive;
	//std::shared_ptr<Alembic::Abc::OObject> topObject;
	std::shared_ptr<Alembic::AbcGeom::OPolyMesh> mesh;
	std::shared_ptr<Alembic::AbcGeom::OXform> transform;
};

AbcWriter::AbcWriter(const std::string& file, const std::string& xFormName, const std::string& meshName,
		const std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>& arbGeoProperties) : m_archiveName(file), m_objectName(meshName)
{
	m_data = std::make_shared<AbcWriterImp>();
	m_data->archive = std::make_shared<Alembic::Abc::OArchive>(Alembic::AbcCoreOgawa::WriteArchive(), m_archiveName);
	m_data->transform = std::make_shared<Alembic::AbcGeom::OXform>(Alembic::Abc::OObject(m_data->archive->getTop()), xFormName);
	m_data->mesh = std::make_shared<Alembic::AbcGeom::OPolyMesh>(*m_data->transform, m_objectName);

	Alembic::AbcGeom::OCompoundProperty arbGeomPs = m_data->mesh->getSchema().getArbGeomParams();
	//create custom properties
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
			m_floatParams.emplace_back(arbGeomPs, std::get<0>(p), false, scope, 1); 
		}
		else if(std::get<1>(p) == PROP_TYPE::VECTOR)
		{
			m_vectorParams.emplace_back(arbGeomPs, std::get<0>(p), false, scope, 1); 
		}
		else
		{
			std::cout << "ERROR: Unrecognised Property Type. This will not be created! (" << std::get<0>(p) << ")" << std::endl;
		}

	}

	if (m_data->archive->valid())
	{
		m_fileIsOpen = true;
	}
}


AbcWriter::~AbcWriter()
{
	if (m_fileIsOpen)
	{
		std::cout << "Writing [ " << m_data->archive->getName() << " ]" << std::endl;
		std::cout << "Num Samples Saved: " << m_data->mesh->getSchema().getNumSamples() << std::endl;
	}
}


bool
AbcWriter::addSample(std::vector<Alembic::Abc::V3f>& vertices, std::vector<int>& faceIndices, std::vector<int>& faceCounts)
{
	// Make sure that our file is open
	if (!m_fileIsOpen)
	{
		std::cout << "ERROR: Alembic Archive [" <<
			m_archiveName << "] is not open! Sample could not be written." << std::endl;
		return false;
	}

	//get schema
	Alembic::AbcGeom::OPolyMeshSchema& schema = m_data->mesh->getSchema();

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
		const std::vector<std::vector<Alembic::Abc::V3f>> vectorProps)
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
	Alembic::AbcGeom::OPolyMeshSchema& schema = m_data->mesh->getSchema();

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
	for(size_t i = 0; i < floatProps.size(); ++i)
	{
		Alembic::AbcGeom::OFloatGeomParam::Sample floatSamp;
		//floatSamp.setScope(floatPropsScope[i]);
		floatSamp.setVals(Alembic::AbcGeom::FloatArraySample(&floatProps[i].front(), floatProps[0].size()));
		m_floatParams[i].set(floatSamp);
	}

	for(size_t i = 0; i < vectorProps.size(); ++i)
	{
		Alembic::AbcGeom::OV3fGeomParam::Sample vectorSamp;
		//vectorSamp.setScope(vectorPropsScope[i]);
		vectorSamp.setVals(Alembic::AbcGeom::V3fArraySample(&vectorProps[i].front(), vectorProps[0].size()));
		m_vectorParams[i].set(vectorSamp);
	}

	//set mesh sample
	schema.set(sample);

	return true;
}
