/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Manager for export geometry


#ifndef CRYINCLUDE_EDITOR_EXPORT_EXPORTMANAGER_H
#define CRYINCLUDE_EDITOR_EXPORT_EXPORTMANAGER_H
#pragma once


#include <AzCore/Component/EntityId.h>
#include <IExportManager.h>

class CExportManager;
class CTrackViewTrack;
class CTrackViewSequence;
class CTrackViewAnimNode;
class CEntityObject;

typedef std::vector<IExporter*> TExporters;


namespace Export
{
    class CMesh
        : public Mesh
        , public CRefCountBase
    {
    public:
        CMesh();

        int GetFaceCount() const override { return static_cast<int>(m_faces.size()); }
        const Face* GetFaceBuffer() const override { return !m_faces.empty() ? &m_faces[0] : nullptr; }

    private:
        std::vector<Face> m_faces;

        friend CExportManager;
    };


    class CObject
        : public Object
        , public CRefCountBase
    {
    public:
        CObject(const char* pName);

        int GetVertexCount() const override { return static_cast<int>(m_vertices.size()); }
        const Vector3D* GetVertexBuffer() const override { return !m_vertices.empty() ? &m_vertices[0] : nullptr; }

        int GetNormalCount() const override { return static_cast<int>(m_normals.size()); }
        const Vector3D* GetNormalBuffer() const override { return !m_normals.empty() ? &m_normals[0] : nullptr; }

        int GetTexCoordCount() const override { return static_cast<int>(m_texCoords.size()); }
        const UV* GetTexCoordBuffer() const override { return !m_texCoords.empty() ? &m_texCoords[0] : nullptr; }

        int GetMeshCount() const override { return static_cast<int>(m_meshes.size()); }
        Mesh* GetMesh(int index) const override { return m_meshes[index]; }

        size_t  MeshHash() const override{return m_MeshHash; }

        void SetMaterialName(const char* pName);
        int GetEntityAnimationDataCount() const override {return static_cast<int>(m_entityAnimData.size()); }
        const EntityAnimData* GetEntityAnimationData(int index) const override {return &m_entityAnimData[index]; }
        void SetEntityAnimationData(EntityAnimData entityData) override{ m_entityAnimData.push_back(entityData); };
        void SetLastPtr(CBaseObject* pObject){m_pLastObject = pObject; };
        CBaseObject* GetLastObjectPtr(){return m_pLastObject; };

    private:
        CBaseObject* m_pLastObject;
        std::vector<Vector3D> m_vertices;
        std::vector<Vector3D> m_normals;
        std::vector<UV> m_texCoords;
        std::vector< _smart_ptr<CMesh> > m_meshes;
        std::vector<EntityAnimData> m_entityAnimData;

        size_t  m_MeshHash;

        friend CExportManager;
    };


    class CData
        : public IData
    {
    public:
        virtual ~CData() = default;

        int GetObjectCount() const override { return static_cast<int>(m_objects.size()); }
        Object* GetObject(int index) const override { return m_objects[index]; }
        Object* AddObject(const char* objectName) override;
        void Clear();

    private:
        std::vector< _smart_ptr<Export::CObject> > m_objects;

        friend CExportManager;
    };
}


typedef std::map<CBaseObject*, int> TObjectMap;

class CExportManager
    : public IExportManager
{
public:

    CExportManager();
    virtual ~CExportManager();

    //! Register exporter
    //! return true if succeed, otherwise false
    bool RegisterExporter(IExporter* pExporter) override;

    //! Export specified geometry
    //! return true if succeed, otherwise false
    bool Export(const char* defaultName, const char* defaultExt = "", const char* defaultPath = "", bool isSelectedObjects = true,
        bool isSelectedRegionObjects = false, bool isOccluder = false, bool bAnimationExport = false);

    bool AddSelectedEntityObjects();

    //! Add to Export Data geometry from objects inside selected region volume
    //! return true if succeed, otherwise false
    bool AddSelectedRegionObjects();

    //! Export to file collected data, using specified exporter throughout the file extension
    //! return true if succeed, otherwise false
    bool ExportToFile(const char* filename, bool bClearDataAfterExport = true);

    bool ImportFromFile(const char* filename);
    const Export::CData& GetData() const {return m_data; };

    //! Exports the stat obj to the obj file specified
    //! returns true if succeeded, otherwise false
    bool ExportSingleStatObj(IStatObj* pStatObj, const char* filename) override;

    void SetBakedKeysSequenceExport(bool bBaked){m_bBakedKeysSequenceExport = bBaked; };

    void SaveNodeKeysTimeToXML();

private:
    void AddMesh(Export::CObject* pObj, const IIndexedMesh* pIndMesh, Matrix34A* pTm = nullptr);
    bool AddStatObj(Export::CObject* pObj, IStatObj* pStatObj, Matrix34A* pTm = nullptr);
    bool AddMeshes(Export::CObject* pObj);
    bool AddObject(CBaseObject* pBaseObj);
    void SolveHierarchy();

    void AddEntityAnimationData(AZ::EntityId entityId);
    void ProcessEntityAnimationTrack(AZ::EntityId entityId, Export::CObject* pObj, AnimParamType entityTrackParamType);
    void AddEntityAnimationData(const CTrackViewTrack* pTrack, Export::CObject* pObj, AnimParamType entityTrackParamType);

    void AddPosRotScale(Export::CObject* pObj, const CBaseObject* pBaseObj);
    void AddEntityData(Export::CObject* pObj, Export::AnimParamType dataType, const float fValue, const float fTime);

    bool AddObjectsFromSequence(CTrackViewSequence* pSequence, XmlNodeRef seqNode = 0);
    bool IsDuplicateObjectBeingAdded(const QString& newObject);
    void SetFBXExportSettings(bool bLocalCoordsToSelectedObject, bool bExportOnlyPrimaryCamera, const float fps);
    bool ProcessObjectsForExport();

    bool ShowFBXExportDialog();

    void FillAnimTimeNode(XmlNodeRef writeNode, CTrackViewAnimNode* pObjectNode, CTrackViewSequence* currentSequence);
    QString CleanXMLText(const QString& text);

private:

    TExporters m_exporters;
    Export::CData m_data;
    bool m_isPrecaching;
    bool m_isOccluder;
    float m_fScale;
    TObjectMap m_objectMap;
    bool m_bAnimationExport;

    CBaseObject* m_pBaseObj;

    float m_FBXBakedExportFPS;
    bool m_bExportLocalCoords;
    bool m_bExportOnlyPrimaryCamera;
    int m_numberOfExportFrames;
    CEntityObject* m_pivotEntityObject;
    bool m_bBakedKeysSequenceExport;

    QString m_animTimeExportPrimarySequenceName;
    float m_animTimeExportPrimarySequenceCurrentTime;
    XmlNodeRef m_animTimeNode;

    bool m_animKeyTimeExport;
    bool m_soundKeyTimeExport;
};


#endif // CRYINCLUDE_EDITOR_EXPORT_EXPORTMANAGER_H
