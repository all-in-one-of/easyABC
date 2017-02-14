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
	~AbcWriter();

	bool addSample(std::vector<Alembic::Abc::V3f>& vertices,
		std::vector<int>& faceIndices, std::vector<int>& faceCounts);

	bool addSample(const std::vector<Alembic::Abc::V3f>& vertices,
		const std::vector<int>& faceIndices, const std::vector<int>& faceCounts,
		const std::vector<Alembic::Abc::V3f>& normals, PROP_SCOPE normalsScope,
		const std::vector<std::vector<float>> floatProps,
		const std::vector<std::vector<Alembic::Abc::V3f>> vectorProps);
	


private:
	std::string m_archiveName;
	std::string m_objectName;

	bool m_fileIsOpen;

	std::shared_ptr<AbcWriterImp> m_data;

	//custom properties
	std::vector<Alembic::AbcGeom::OFloatGeomParam> m_floatParams;
	std::vector<Alembic::AbcGeom::OV3fGeomParam> m_vectorParams;
	std::vector<Alembic::AbcGeom::OC3fGeomParam> m_colourParams;
	std::vector<Alembic::AbcGeom::GeometryScope> m_arbScopes;
	std::vector<std::string> m_arbNames;
};

