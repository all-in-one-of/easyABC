#pragma once

#include <string>
#include <vector>
#include <memory>

#include <Alembic/Abc/All.h>
#include <Alembic/AbcGeom/All.h>

#include "easyAbcUtil.h"

struct AbcWriterImp;

class AbcWriter
{
public:
	AbcWriter(const std::string& file, const std::string& xFormName, const std::string& meshName,
		const std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>& arbGeoProperties);

	AbcWriter(const std::string& file, const std::vector<std::string>& xFormNames, const std::vector<std::string>& meshNames,
		const std::vector<std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>>& arbGeoProperties);

	~AbcWriter();

	bool addSample(std::vector<Alembic::Abc::V3f>& vertices,
		std::vector<int>& faceIndices, std::vector<int>& faceCounts, size_t meshIdx = 0);

	bool addSample(const std::vector<Alembic::Abc::V3f>& vertices,
		const std::vector<int>& faceIndices, const std::vector<int>& faceCounts,
		const std::vector<Alembic::Abc::V3f>& normals, PROP_SCOPE normalsScope,
		const std::vector<std::vector<float>> floatProps,
		const std::vector<std::vector<Alembic::Abc::V3f>> vectorProps, size_t meshIdx = 0);
	
	// Three different ways to write a transform sample
	//! Order of ops: translate, scale, rotateX, rotateY, rotateZ
	void addXFormSample(const Alembic::Abc::V3d& translate, const Alembic::Abc::V3d& scale, 
		const double angleInDegreesX, const double angleInDegreesY, const double angleInDegreesZ, size_t meshIdx = 0);

	//! Order of ops: translate, scale, rotate
	void addXFormSample(const Alembic::Abc::V3d& translate, const Alembic::Abc::V3d& scale, 
		const Alembic::Abc::V3d& rotationAxis, const double angleInDegrees, size_t meshIdx = 0);

	void addXFormSample(const Alembic::Abc::M44d& transformMatrix, size_t meshIdx = 0);

private:
	void setupObject(const std::string& xFormName, const std::string& meshName,
		const std::vector<std::tuple<std::string, PROP_TYPE, PROP_SCOPE>>& arbGeoProperties);
	std::string m_archiveName;
	std::vector<std::string> m_objectName;

	bool m_fileIsOpen;

	std::shared_ptr<Alembic::Abc::OArchive> m_archive;
	std::vector<std::shared_ptr<AbcWriterImp>> m_data;

	//custom properties
	std::vector<std::vector<Alembic::AbcGeom::OFloatGeomParam>> m_floatParams;
	std::vector<std::vector<Alembic::AbcGeom::OV3fGeomParam>> m_vectorParams;
	std::vector<std::vector<Alembic::AbcGeom::OC3fGeomParam>> m_colourParams;
	std::vector<std::vector<Alembic::AbcGeom::GeometryScope>> m_arbScopes;
	std::vector<std::vector<std::string>> m_arbNames;
};

