/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Editor structure that wraps access to IStatObj


#ifndef CRYINCLUDE_EDITOR_GEOMETRY_EDMESH_H
#define CRYINCLUDE_EDITOR_GEOMETRY_EDMESH_H
#pragma once

#include "EdGeometry.h"
#include "Objects/SubObjSelection.h"
#include "TriMesh.h"

// Flags that can be set on CEdMesh.
enum CEdMeshFlags
{
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    CEdMesh is a Geometry kind representing simple mesh.
//    Holds IStatObj interface from the 3D Engine.
//////////////////////////////////////////////////////////////////////////
class CRYEDIT_API CEdMesh
    : public CEdGeometry
{
public:
    //////////////////////////////////////////////////////////////////////////
    // CEdGeometry implementation.
    //////////////////////////////////////////////////////////////////////////
    virtual EEdGeometryType GetType() const { return GEOM_TYPE_MESH; };
    virtual void Serialize(CObjectArchive& ar);

    virtual void GetBounds(AABB& box);
    virtual CEdGeometry* Clone();
    virtual IIndexedMesh* GetIndexedMesh(size_t idx = 0);
    virtual void GetTM(Matrix34* pTM, size_t idx = 0);
    virtual void SetModified(bool bModified = true);
    virtual bool IsModified() const { return m_bModified; };
    virtual bool StartSubObjSelection(const Matrix34& nodeWorldTM, int elemType, int nFlags);
    virtual void EndSubObjSelection();
    virtual void Display(DisplayContext& dc);
    virtual bool HitTest(HitContext& hit);
    bool GetSelectionReferenceFrame(Matrix34& refFrame);
    virtual void ModifySelection(SSubObjSelectionModifyContext& modCtx, bool isUndo = true);
    virtual void AcceptModifySelection();
    //////////////////////////////////////////////////////////////////////////

    ~CEdMesh();

    // Return filename of mesh.
    const QString& GetFilename() const { return m_filename; };
    void SetFilename(const QString& filename);

    //! Reload geometry of mesh.
    void ReloadGeometry();
    void AddUser();
    void RemoveUser();
    int GetUserCount() const { return m_nUserCount; };

    //////////////////////////////////////////////////////////////////////////
    void SetFlags(int nFlags) { m_nFlags = nFlags; };
    int  GetFlags() { return m_nFlags; }
    //////////////////////////////////////////////////////////////////////////

    //! Access stored IStatObj.
    IStatObj* GetIStatObj() const { return m_pStatObj; }

    //! Returns true if filename and geomname refer to the same object as this one.
    bool IsSameObject(const char* filename);

    //! RenderMesh.
    void Render(SRendParams& rp, const SRenderingPassInfo& passInfo);

    //! Make new CEdMesh, if same IStatObj loaded, and CEdMesh for this IStatObj is allocated.
    //! This instance of CEdMesh will be returned.
    static CEdMesh* LoadMesh(const char* filename);

    // Creates a new mesh not from a file.
    // Create a new StatObj and IndexedMesh.
    static CEdMesh* CreateMesh(const char* name);

    //! Reload all geometries.
    static void ReloadAllGeometries();
    static void ReleaseAll();

    //! Check if default object was loaded.
    bool IsDefaultObject();

    //////////////////////////////////////////////////////////////////////////
    // Copy EdMesh data to the specified mesh.
    void CopyToMesh(CTriMesh& toMesh, int nCopyFlags);
    // Copy EdMesh data from the specified mesh.
    void CopyFromMesh(CTriMesh& fromMesh, int nCopyFlags, bool bUndo);

    // Retrieve mesh class.
    CTriMesh* GetMesh();

    //////////////////////////////////////////////////////////////////////////
    void InvalidateMesh();
    void SetWorldTM(const Matrix34& worldTM);

    // Save mesh into the file.
    // Optionally can provide pointer to the pak file where to save files into.
    void SaveToCGF(const char* sFilename, CPakFile* pPakFile = NULL, _smart_ptr<IMaterial> pMaterial = NULL);

    // Draw debug representation of this mesh.
    void DebugDraw(const SGeometryDebugDrawInfo& info, float fExtrdueScale = 0.01f);

private:
    //////////////////////////////////////////////////////////////////////////
    CEdMesh(IStatObj* pGeom);
    CEdMesh();
    void UpdateSubObjCache();
    void UpdateIndexedMeshFromCache(bool bFast);
    void OnSelectionChange();

    //////////////////////////////////////////////////////////////////////////
    struct SSubObjHitTestEnvironment
    {
        Vec3 vWSCameraPos;
        Vec3 vWSCameraVector;
        Vec3 vOSCameraVector;

        bool bHitTestNearest;
        bool bHitTestSelected;
        bool bSelectOnHit;
        bool bAdd;
        bool bRemove;
        bool bSelectValue;
        bool bHighlightOnly;
        bool bIgnoreBackfacing;
    };
    struct SSubObjHitTestResult
    {
        CTriMesh::EStream stream;  // To What stream of the TriMesh this result apply.
        MeshElementsArray elems; // List of hit elements.
        float minDistance;      // Minimal distance to the hit.
        SSubObjHitTestResult() { minDistance = FLT_MAX; }
    };
    bool HitTestVertex(HitContext& hit, SSubObjHitTestEnvironment& env, SSubObjHitTestResult& result);
    bool HitTestEdge(HitContext& hit, SSubObjHitTestEnvironment& env, SSubObjHitTestResult& result);
    bool HitTestFace(HitContext& hit, SSubObjHitTestEnvironment& env, SSubObjHitTestResult& result);

    // Return`s true if selection changed.
    bool SelectSubObjElements(SSubObjHitTestEnvironment& env, SSubObjHitTestResult& result);
    bool IsHitTestResultSelected(SSubObjHitTestResult& result);

    //////////////////////////////////////////////////////////////////////////
    //! CGF filename.
    QString m_filename;
    IStatObj* m_pStatObj;
    int m_nUserCount;
    int m_nFlags;

    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    typedef std::map<QString, CEdMesh*, stl::less_stricmp<QString> > MeshMap;
    static MeshMap m_meshMap;

    // This cache is created when sub object selection is needed.
    struct SubObjCache
    {
        // Cache of data in geometry.
        // World space mesh.
        CTriMesh* pTriMesh;
        Matrix34 worldTM;
        Matrix34 invWorldTM;
        CBitArray m_tempBitArray;
        bool bNoDisplay;

        SubObjCache()
            : pTriMesh(0)
            , bNoDisplay(false) {};
    };
    SubObjCache* m_pSubObjCache;
    bool m_bModified;

    std::vector<IIndexedMesh*> m_tempIndexedMeshes;
    std::vector<Matrix34> m_tempMatrices;
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
};

#endif // CRYINCLUDE_EDITOR_GEOMETRY_EDMESH_H
