/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        virtual int GetFaceCount() const { return m_faces.size(); }
        virtual const Face* GetFaceBuffer() const { return m_faces.size() ? &m_faces[0] : 0; }

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

        virtual int GetVertexCount() const { return m_vertices.size(); }
        virtual const Vector3D* GetVertexBuffer() const{ return m_vertices.size() ? &m_vertices[0] : 0; }

        virtual int GetNormalCount() const { return m_normals.size(); }
        virtual const Vector3D* GetNormalBuffer() const { return m_normals.size() ? &m_normals[0] : 0; }

        virtual int GetTexCoordCount() const { return m_texCoords.size(); }
        virtual const UV* GetTexCoordBuffer() const { return m_texCoords.size() ? &m_texCoords[0] : 0; }

        virtual int GetMeshCount() const { return m_meshes.size(); }
        virtual Mesh* GetMesh(int index) const { return m_meshes[index]; }

        virtual size_t  MeshHash() const{return m_MeshHash; }

        void SetMaterialName(const char* pName);
        virtual int GetEntityAnimationDataCount() const {return m_entityAnimData.size(); }
        virtual const EntityAnimData* GetEntityAnimationData(int index) const {return &m_entityAnimData[index]; }
        virtual void SetEntityAnimationData(EntityAnimData entityData){ m_entityAnimData.push_back(entityData); };
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
        virtual int GetObjectCount() const { return m_objects.size(); }
        virtual Object* GetObject(int index) const { return m_objects[index]; }
        virtual Object* AddObject(const char* objectName);
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
    virtual bool RegisterExporter(IExporter* pExporter);

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
    virtual bool ExportSingleStatObj(IStatObj* pStatObj, const char* filename);

    void SetBakedKeysSequenceExport(bool bBaked){m_bBakedKeysSequenceExport = bBaked; };

    void SaveNodeKeysTimeToXML();

private:
    void AddMesh(Export::CObject* pObj, const IIndexedMesh* pIndMesh, Matrix34A* pTm = 0);
    bool AddStatObj(Export::CObject* pObj, IStatObj* pStatObj, Matrix34A* pTm = 0);
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
