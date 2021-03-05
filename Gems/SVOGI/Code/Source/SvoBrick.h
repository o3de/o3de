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

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/std/parallel/shared_mutex.h>

#include <LmbrCentral/Rendering/MeshAsset.h>

#include "Cry_Math.h"//has to go before cry_color.h
#include "Cry_Color.h"
#include <cstring>//for memset
#include "smartptr.h"

#include "I3DEngine.h"
#include "ITexture.h"
#include "IMaterial.h"
#include "IStatObj.h"
#include <SVOGI_Traits_Platform.h>


namespace AzFramework
{
    namespace Terrain
    {
        class TerrainDataRequests;
    }
}

/*
 This structure represents the brick in the parlance of brickmaps.  With brick maps each voxel
 represents a brick of data. The brick subdivides the voxel into a denser sampling region while
 allowing for a lower tree height and data structure overhead.
*/
namespace SVOGI
{
    struct MeshData
    {
        MeshData(AZ::EntityId entityId, AZ::Transform transform, AZ::Aabb worldAabb,
            AZ::Data::Asset<LmbrCentral::MeshAsset> meshAsset, _smart_ptr<IMaterial> material)
            : m_entityId(entityId)
            , m_transform(transform)
            , m_worldAabb(worldAabb)
            , m_meshAsset(meshAsset)
            , m_material(material)
        {}

        AZ::Transform   m_transform;
        AZ::EntityId    m_entityId;
        AZ::Aabb        m_worldAabb;
        AZ::Data::Asset<LmbrCentral::MeshAsset> m_meshAsset;
        _smart_ptr<IMaterial> m_material;
    };

    using EntityMeshDataMap = AZStd::unordered_map<AZ::EntityId, AZStd::shared_ptr<MeshData>>;

    const AZ::s32 brickDimension = (16);
    const AZ::s32 nVoxBloMaxDim = (16);
    const AZ::s32 nVoxNodMaxDim = 2;

    template <typename T>
    struct DataBrick
    {
        AZ_CLASS_ALLOCATOR(DataBrick, AZ::SystemAllocator, 0);
        T m_data[brickDimension * brickDimension * brickDimension];
        DataBrick()
        {
            Reset();
        }
        T& operator[](size_t idx) { return m_data[idx]; }
        const T& operator[](size_t idx) const { return m_data[idx]; }
        void Reset()
        {
            memset(m_data, 0, sizeof(m_data));
        }
    };

    struct ObjectInfo
    {
        ObjectInfo() { memset(this, 0, sizeof(*this)); }
        Matrix34                matObjInv;
        Matrix34                matObj;
        _smart_ptr<IMaterial>   m_material;
        IStatObj*               pStatObj;
        float                   fObjScale;
    };

    // SuperMesh index type
    typedef AZ_TRAIT_SVOGI_SUPERMESH_INDEX_TYPE SMINDEX;

    struct SuperTriangle
    {
        SuperTriangle()
        {
            memset(this, 0, sizeof(*this));
            arrVertId[0] = arrVertId[1] = arrVertId[2] = (SMINDEX)~0;
        }
        Vec3    vFaceNorm;
        AZ::u8  nTriArea;
        AZ::u8  nOpacity;
        AZ::u8  nHitObjType;
        SMINDEX arrVertId[3];
        AZ::u16 nMatID;
    };

    struct SRayHitVertex
    {
        Vec3    v;
        Vec2    t;
        ColorB  c;
    };

    struct SvoMaterialInfo
    {
        SvoMaterialInfo() { memset(this, 0, sizeof(*this)); }
        _smart_ptr<IMaterial>   m_material;
        ITexture*               m_texture;
        ColorB*                 m_textureColor;
        AZ::u16                 m_textureWidth;
        AZ::u16                 m_textureHeight;
        bool operator == (const SvoMaterialInfo& other) const { return m_material == other.m_material; }
    };

    struct SuperMesh
    {
        SuperMesh();
        ~SuperMesh();

        void AddSuperTriangle(SRayHitTriangle& htIn);
        void AddSuperMesh(SuperMesh& smIn, float fVertexOffset);
        void Clear();
        void ReleaseTextures();

        AZStd::vector<SuperTriangle>   m_triangles;
        AZStd::vector<AZ::Vector3>     m_faceNormals;
        AZStd::vector<SRayHitVertex>   m_vertices;
        AZStd::vector<SvoMaterialInfo> m_materials;

    protected:
        AZ::s32 AddVertex(const SRayHitVertex& rVert, AZStd::vector<SRayHitVertex>& vertsInArea);
    };
    

    struct GISubVoxels
    {
        AZ::Color m_colors[4][4][4];
        AZ::Vector3 m_normals[4][4][4];
        float m_emittances[4][4][4];
        float m_opacities[4][4][4];
    };

    class Brick
        : public SuperMesh
    {
    public:
        AZ_CLASS_ALLOCATOR(Brick, AZ::SystemAllocator, 0);
        Brick();
        ~Brick();

        void    ExtractTriangles(AZStd::vector<ObjectInfo>& objects);
        void    ExtractTerrainTriangles();
        void    ExtractVisAreaTriangles();
        float   GetBrickSize() { return (m_brickAabb.GetZExtent()); }
        //Collect Brushes and Vegetation cryEntities.
        bool    CollectLegacyObjects(const AZ::Aabb& nodeBox, AZStd::vector<ObjectInfo>* parrObjects);
        void    ProcessMeshes(
            const EntityMeshDataMap& insertions,
            const EntityMeshDataMap& removals,
            DataBrick<GISubVoxels>& scratch
        );
        bool    ProcessTriangles( 
            DataBrick<GISubVoxels>& data
            , DataBrick<AZ::Vector3>& centers
        );

        bool    HasBrickData() const;
        void    UpdateBrickData(
            DataBrick<GISubVoxels>& data,
            bool increment
        );

        //The Aabb is defined in local coordinates to brick origin.
        AZ::Aabb            m_brickAabb;
        AZ::Vector3         m_brickOrigin;
        
        //Guard against uploading data while writing data at the same time. 
        mutable AZStd::shared_mutex m_brickDataMutex;

        DataBrick<ColorB>*  m_opacities;
        DataBrick<ColorB>*  m_colors;
        DataBrick<ColorB>*  m_normals;
        //Count the number of samples in each subvoxel calculation to be able to do average updates.
        DataBrick<AZ::s32>* m_counts;
        AZStd::atomic_uint  m_lastUploaded;
        AZStd::atomic_uint  m_lastUpdated;
         
        bool m_collectedLegacyObjects;
        bool m_terrainOnly;
        AZ::s32 m_numLegacyObjects;
        AZStd::unordered_set<AZ::EntityId> m_entityIDs;

    private:
        void    FreeBrickData();
        void    FreeTriangleData();
        void    ClearTriangleData();

        //! Does the real work on behalf of ExtractTerrainTriangles() but @terrain is a thread safe
        //! EBus interface.
        void    ExtractTerrainTrianglesLocked(AzFramework::Terrain::TerrainDataRequests* terrain);
    };

}
