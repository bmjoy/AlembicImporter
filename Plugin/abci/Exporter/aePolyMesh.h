#pragma once

class aeFaceSet
{
public:
    aeFaceSet(aeObject *parent, const char *name, uint32_t tsi);
    void writeSample(const aeFaceSetData &data);

private:
    std::unique_ptr<abcObject> m_abc;
    AbcGeom::OFaceSetSchema m_schema;
    RawVector<int> m_buf_faces;
};
using aeFaceSetPtr = std::shared_ptr<aeFaceSet>;


class aePolyMesh : public aeObject
{
using super = aeObject;
public:
    aePolyMesh(aeObject *parent, const char *name, uint32_t tsi);
    abcPolyMesh& getAbcObject() override;
    abcProperties getAbcProperties() override;

    size_t  getNumSamples() override;
    void    setFromPrevious() override;
    void    writeSample(const aePolyMeshData &data);

    int addFaceSet(const char *name);
    void writeFaceSetSample(int faceset_index, const aeFaceSetData &data);

private:
    void    writeSampleBody();

    AbcGeom::OPolyMeshSchema m_schema;
    std::unique_ptr<AbcGeom::OV2fGeomParam> m_uv1_param;
    std::unique_ptr<AbcGeom::OC4fGeomParam> m_colors_param;
    std::vector<aeFaceSetPtr> m_facesets;

    RawVector<int>   m_buf_faces;

    RawVector<abcV3> m_buf_points;
    RawVector<abcV3> m_buf_velocities;
    RawVector<int>   m_buf_indices;

    RawVector<abcV3> m_buf_normals;
    RawVector<int>   m_buf_normal_indices;

    RawVector<abcV2> m_buf_uv0;
    RawVector<int>   m_buf_uv0_indices;

    RawVector<abcV2> m_buf_uv1;
    RawVector<int>   m_buf_uv1_indices;

    RawVector<abcV4> m_buf_colors;
    RawVector<int>   m_buf_colors_indices;
};
