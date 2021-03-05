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

//  Description : 3dengine interface

#ifndef CRYINCLUDE_CRYCOMMON_I3DENGINE_H
#define CRYINCLUDE_CRYCOMMON_I3DENGINE_H
#pragma once

// The maximum number of unique surface types that can be used per node
#define MMRM_MAX_SURFACE_TYPES 16
#define COMPILED_OCTREE_FILE_NAME "terrain/terrain.dat"

// !!! Do not add any headers here !!!
#include "CryEngineDecalInfo.h"
#include <IConsole.h>
#include <IStatObj.h> // <> required for Interfuscator
#include "IRenderer.h"
#include <IProcess.h> // <> required for Interfuscator
#include <IMaterial.h> // <> required for Interfuscator
#include <ISurfaceType.h> // <> required for Interfuscator
#include <IEntityRenderState.h> // <> required for Interfuscator
#include "CryArray.h"
#include <IMemory.h> // <> required for Interfuscator
#include <AzCore/Casting/numeric_cast.h>
// !!! Do not add any headers here !!!

class IOpticsManager;
class CContentCGF;
class ICrySizer;
class CRenderView;
struct ISystem;
struct CVars;
struct pe_params_particle;
struct IMaterial;
struct SpawnParams;
struct ForceObject;
struct IRenderNode;
struct IObjManager;
struct IDeferredPhysicsEventManager;
struct IBSPTree3D;
struct ITextureLoadHandler;
struct ITimeOfDay;
struct MacroTextureConfiguration;

namespace LegacyProceduralVegetation
{
    class IVegetationPoolManager;
}

namespace LegacyTerrain
{
    struct MacroTextureConfiguration;
}

namespace ChunkFile
{
    struct IChunkFileWriter;
}

namespace AZ
{
    class Aabb;
}

enum E3DEngineParameter
{
    E3DPARAM_SUN_COLOR,

    E3DPARAM_SUN_SPECULAR_MULTIPLIER,

    E3DPARAM_AMBIENT_GROUND_COLOR,
    E3DPARAM_AMBIENT_MIN_HEIGHT,
    E3DPARAM_AMBIENT_MAX_HEIGHT,

    E3DPARAM_FOG_COLOR,
    E3DPARAM_FOG_COLOR2,
    E3DPARAM_FOG_RADIAL_COLOR,

    E3DPARAM_VOLFOG_HEIGHT_DENSITY,
    E3DPARAM_VOLFOG_HEIGHT_DENSITY2,

    E3DPARAM_VOLFOG_GRADIENT_CTRL,

    E3DPARAM_VOLFOG_GLOBAL_DENSITY,
    E3DPARAM_VOLFOG_RAMP,

    E3DPARAM_VOLFOG_SHADOW_RANGE,
    E3DPARAM_VOLFOG_SHADOW_DARKENING,
    E3DPARAM_VOLFOG_SHADOW_ENABLE,

    E3DPARAM_VOLFOG2_CTRL_PARAMS,
    E3DPARAM_VOLFOG2_SCATTERING_PARAMS,
    E3DPARAM_VOLFOG2_RAMP,
    E3DPARAM_VOLFOG2_COLOR,
    E3DPARAM_VOLFOG2_GLOBAL_DENSITY,
    E3DPARAM_VOLFOG2_HEIGHT_DENSITY,
    E3DPARAM_VOLFOG2_HEIGHT_DENSITY2,
    E3DPARAM_VOLFOG2_COLOR1,
    E3DPARAM_VOLFOG2_COLOR2,

    E3DPARAM_SKYLIGHT_SUN_INTENSITY,

    E3DPARAM_SKYLIGHT_KM,
    E3DPARAM_SKYLIGHT_KR,
    E3DPARAM_SKYLIGHT_G,

    E3DPARAM_SKYLIGHT_WAVELENGTH_R,
    E3DPARAM_SKYLIGHT_WAVELENGTH_G,
    E3DPARAM_SKYLIGHT_WAVELENGTH_B,

    E3DPARAM_NIGHSKY_HORIZON_COLOR,
    E3DPARAM_NIGHSKY_ZENITH_COLOR,
    E3DPARAM_NIGHSKY_ZENITH_SHIFT,

    E3DPARAM_NIGHSKY_STAR_INTENSITY,

    E3DPARAM_NIGHSKY_MOON_DIRECTION,
    E3DPARAM_NIGHSKY_MOON_COLOR,
    E3DPARAM_NIGHSKY_MOON_SIZE,
    E3DPARAM_NIGHSKY_MOON_INNERCORONA_COLOR,
    E3DPARAM_NIGHSKY_MOON_INNERCORONA_SCALE,
    E3DPARAM_NIGHSKY_MOON_OUTERCORONA_COLOR,
    E3DPARAM_NIGHSKY_MOON_OUTERCORONA_SCALE,

    E3DPARAM_CLOUDSHADING_MULTIPLIERS,
    E3DPARAM_CLOUDSHADING_SUNCOLOR,
    E3DPARAM_CLOUDSHADING_SKYCOLOR,

    E3DPARAM_CORONA_SIZE,

    E3DPARAM_OCEANFOG_COLOR,
    E3DPARAM_OCEANFOG_DENSITY,

    // Sky highlight (ex. From Lightning)
    E3DPARAM_SKY_HIGHLIGHT_COLOR,
    E3DPARAM_SKY_HIGHLIGHT_SIZE,
    E3DPARAM_SKY_HIGHLIGHT_POS,

    E3DPARAM_SKY_MOONROTATION,

    E3DPARAM_SKY_SKYBOX_ANGLE,
    E3DPARAM_SKY_SKYBOX_STRETCHING,

    EPARAM_SUN_SHAFTS_VISIBILITY,

    E3DPARAM_SKYBOX_MULTIPLIER,

    E3DPARAM_DAY_NIGHT_INDICATOR,

    // Tone mapping tweakables
    E3DPARAM_HDR_FILMCURVE_SHOULDER_SCALE,
    E3DPARAM_HDR_FILMCURVE_LINEAR_SCALE,
    E3DPARAM_HDR_FILMCURVE_TOE_SCALE,
    E3DPARAM_HDR_FILMCURVE_WHITEPOINT,

    E3DPARAM_HDR_EYEADAPTATION_PARAMS,
    E3DPARAM_HDR_EYEADAPTATION_PARAMS_LEGACY,
    E3DPARAM_HDR_BLOOM_AMOUNT,

    E3DPARAM_HDR_COLORGRADING_COLOR_SATURATION,
    E3DPARAM_HDR_COLORGRADING_COLOR_BALANCE,

    E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_COLOR,
    E3DPARAM_COLORGRADING_FILTERS_PHOTOFILTER_DENSITY,
    E3DPARAM_COLORGRADING_FILTERS_GRAIN
};

enum EShadowMode
{
    ESM_NORMAL = 0,
    ESM_HIGHQUALITY
};

struct IBSPTree3D
{
    typedef DynArray<Vec3> CFace;
    typedef DynArray<CFace> FaceList;

    virtual ~IBSPTree3D() {}
    virtual bool IsInside(const Vec3& vPos) const = 0;
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual size_t WriteToBuffer(void* pBuffer) const = 0;
    virtual void ReadFromBuffer(const void* pBuffer) = 0;
};

//////////////////////////////////////////////////////////////////////////
// Description:
//     This structure is filled and passed by the caller to the DebugDraw functions of the stat object or entity.
struct SGeometryDebugDrawInfo
{
    Matrix34 tm;        // Transformation Matrix
    ColorB   color;     // Optional color of the lines.
    ColorB   lineColor; // Optional color of the lines.
    
    // Optional flags controlling how to render debug draw information.
    uint32   bNoCull  : 1;
    uint32   bNoLines : 1;
    uint32   bExtrude : 1; // Extrude debug draw geometry alittle bit so it is over real geometry.

    SGeometryDebugDrawInfo()
        : color(255, 0, 255, 255)
        , lineColor(255, 255, 0, 255)
        , bNoLines(0)
        , bNoCull(0) { tm.SetIdentity(); }
};

struct SFrameLodInfo
{
    uint32 nID;
    float fLodRatio;
    float fTargetSize;
    uint nMinLod;
    uint nMaxLod;

    SFrameLodInfo()
    {
        nID = 0;
        fLodRatio = 0.f;
        fTargetSize = 0.f;
        nMinLod = 0;
        nMaxLod = 6;
    }
};
struct SMeshLodInfo
{
    static const int s_nMaxLodCount = 5;
    float fGeometricMean;
    uint nFaceCount;
    uint32 nFrameLodID;
    SMeshLodInfo()
    {
        Clear();
    }
    void Clear()
    {
        fGeometricMean = 0.f;
        nFaceCount = 0;
        nFrameLodID = 0;
    }
    void Merge(const SMeshLodInfo& lodInfo)
    {
        uint nTotalCount = nFaceCount + lodInfo.nFaceCount;
        if (nTotalCount > 0)
        {
            float fGeometricMeanTotal = 0.f;
            if (fGeometricMean > 0.f)
            {
                fGeometricMeanTotal += logf(fGeometricMean) * nFaceCount;
            }
            if (lodInfo.fGeometricMean > 0.f)
            {
                fGeometricMeanTotal += logf(lodInfo.fGeometricMean) * lodInfo.nFaceCount;
            }
            fGeometricMean = expf(fGeometricMeanTotal / (float)nTotalCount);
            nFaceCount = nTotalCount;
        }
    }
};
// Summary:
//     Physics material enumerator, allows for 3dengine to get material id from game code.
struct IPhysMaterialEnumerator
{
    // <interfuscator:shuffle>
    virtual ~IPhysMaterialEnumerator(){}
    virtual int EnumPhysMaterial(const char* szPhysMatName) = 0;
    virtual bool IsCollidable(int nMatId) = 0;
    virtual int GetMaterialCount() = 0;
    virtual const char* GetMaterialNameByIndex(int index) = 0;
    // </interfuscator:shuffle>
};


// Summary:
//     Physics foreign data flags.
enum EPhysForeignFlags
{
    PFF_HIDABLE                = 0x1,
    PFF_HIDABLE_SECONDARY      = 0x2,
    PFF_EXCLUDE_FROM_STATIC    = 0x4,
    PFF_UNIMPORTANT            = 0x20,
    PFF_OUTDOOR_AREA           = 0x40,
    PFF_MOVING_PLATFORM        = 0x80,
};

// Summary:
//     Ocean data flags.
enum EOceanRenderFlags
{
    OCR_NO_DRAW     =   1 << 0,
    OCR_OCEANVOLUME_VISIBLE  =   1 << 1,
};

// Summary:
//      Structure to pass vegetation group properties.
struct IStatInstGroup
{
    enum EPlayerHideable
    {
        ePlayerHideable_None = 0,
        ePlayerHideable_High,
        ePlayerHideable_Mid,
        ePlayerHideable_Low,

        ePlayerHideable_COUNT,
    };

    IStatInstGroup()
    {
        pStatObj = 0;
        szFileName[0] = 0;
        bHideability = 0;
        bHideabilitySecondary = 0;
        fBending = 0;
        nCastShadowMinSpec = 0;
        bRecvShadow = 0;
        bDynamicDistanceShadows = false;
        bUseAlphaBlending = 0;
        fSpriteDistRatio = 1.f;
        fShadowDistRatio = 1.f;
        fMaxViewDistRatio = 1.f;
        fLodDistRatio = 1.f;
        fBrightness = 1.f;
        pMaterial = 0;

        fDensity = 1;
        fElevationMax = 4096;
        fElevationMin = 8;
        fSize = 1;
        fSizeVar = 0;
        fSlopeMax = 255;
        fSlopeMin = 0;
        fStiffness = 0.5f;
        fDamping = 2.5f;
        fVariance = 0.6f;
        fAirResistance = 1.f;
        bRandomRotation = false;
        nRotationRangeToTerrainNormal = 0;
        nMaterialLayers = 0;
        bAllowIndoor = false;
        fAlignToTerrainCoefficient = 0.f;
        bAutoMerged = false;
        minConfigSpec = (ESystemConfigSpec)0;
        nTexturesAreStreamedIn = 0;
        nPlayerHideable = ePlayerHideable_None;
        nID = -1;
    }

    IStatObj* GetStatObj()
    {
        IStatObj* p = pStatObj;
        return (IStatObj*)p;
    }
    const IStatObj* GetStatObj() const
    {
        const IStatObj* p = pStatObj;
        return (const IStatObj*)p;
    }

    _smart_ptr<IStatObj> pStatObj;
    char  szFileName[256];
    bool  bHideability;
    bool  bHideabilitySecondary;
    float fBending;
    uint8 nCastShadowMinSpec;
    bool  bRecvShadow;
    bool  bDynamicDistanceShadows;
    bool  bUseAlphaBlending;
    float fSpriteDistRatio;
    float fLodDistRatio;
    float fShadowDistRatio;
    float fMaxViewDistRatio;
    float fBrightness;
    bool  bRandomRotation;
    int32 nRotationRangeToTerrainNormal;
    float fAlignToTerrainCoefficient;
    bool  bAllowIndoor;
    bool  bAutoMerged;

    float fDensity;
    float fElevationMax;
    float fElevationMin;
    float fSize;
    float fSizeVar;
    float fSlopeMax;
    float fSlopeMin;
    float fStiffness;
    float fDamping;
    float fVariance;
    float fAirResistance;

    float fVegRadius;
    float fVegRadiusVert;
    float fVegRadiusHor;

    int nPlayerHideable;
    int nID;

    // Minimal configuration spec for this vegetation group.
    ESystemConfigSpec minConfigSpec;

    // Override material for this instance group.
    _smart_ptr<IMaterial> pMaterial;

    // Material layers bitmask -> which layers are active.
    uint8 nMaterialLayers;

    // Textures Are Streamed In.
    uint8 nTexturesAreStreamedIn;

    // Flags similar to entity render flags.
    int m_dwRndFlags;
};

// Description:
//     Water volumes should usually be created by I3DEngine::CreateWaterVolume.
// Summary:
//     Interface to water volumes.
struct IWaterVolume
{
    // <interfuscator:shuffle>
    //DOC-IGNORE-BEGIN
    virtual ~IWaterVolume(){}
    virtual void UpdatePoints(const Vec3* pPoints, int nCount, float fHeight) = 0;
    virtual void SetFlowSpeed(float fSpeed) = 0;
    virtual void SetAffectToVolFog(bool bAffectToVolFog) = 0;
    virtual void SetTriSizeLimits(float fTriMinSize, float fTriMaxSize) = 0;
    //  virtual void SetMaterial(const char * szShaderName) = 0;
    virtual void SetMaterial(_smart_ptr<IMaterial> pMaterial) = 0;
    virtual _smart_ptr<IMaterial> GetMaterial() = 0;
    virtual const char* GetName() const = 0;
    virtual void SetName(const char* szName) = 0;
    //DOC-IGNORE-END

    // Description:
    //     Used to change the water level. Will assign a new Z value to all 
    //     vertices of the water geometry.
    // Arguments:
    //     vNewOffset - Position of the new water level
    // Summary:
    //     Sets a new water level.
    virtual void SetPositionOffset(const Vec3& vNewOffset) = 0;
    // </interfuscator:shuffle>
};

struct SClipVolumeBlendInfo
{
    static const int BlendPlaneCount = 2;

    Plane blendPlanes[BlendPlaneCount];
    struct IClipVolume* blendVolumes[BlendPlaneCount];
};

struct IClipVolume
{
    enum EClipVolumeFlags
    {
        eClipVolumeConnectedToOutdoor = BIT(0),
        eClipVolumeIgnoreGI           = BIT(1),
        eClipVolumeAffectedBySun      = BIT(2),
        eClipVolumeBlend              = BIT(3),
        eClipVolumeIsVisArea          = BIT(4),
        eClipVolumeIgnoreOutdoorAO    = BIT(5),
    };

    virtual ~IClipVolume() {};
    virtual void GetClipVolumeMesh(_smart_ptr<IRenderMesh>& renderMesh, Matrix34& worldTM) const = 0;
    virtual AABB GetClipVolumeBBox() const = 0;
    virtual bool IsPointInsideClipVolume(const Vec3& point) const = 0;

    virtual uint8 GetStencilRef() const = 0;
    virtual uint    GetClipVolumeFlags() const = 0;
};

// Summary:
//     Provides information about the different VisArea volumes.
struct IVisArea
    : public IClipVolume
{
    // <interfuscator:shuffle>
    virtual ~IVisArea(){}
    // Summary:
    //     Gets the last rendered frame id.
    // Return Value:
    //     An int which contains the frame id.
    virtual int GetVisFrameId() = 0;

    // Description:
    //     Gets a list of all the VisAreas which are connected to the current one. 
    // Arguments:
    //     pAreas               - Pointer to an array of IVisArea*
    //     nMaxConnNum          - The maximum of IVisArea to write in pAreas
    //     bSkipDisabledPortals - Ignore portals which are disabled
    // Return Value:
    //     An integer which hold the amount of VisArea found to be connected. If 
    //     the return is equal to nMaxConnNum, it's possible that not all 
    //     connected VisAreas were returned due to the restriction imposed by the 
    //     argument.
    // Summary:
    //     Gets all the areas which are connected to the current one.
    virtual int GetVisAreaConnections(IVisArea** pAreas, int nMaxConnNum, bool bSkipDisabledPortals = false) = 0;

    // Summary:
    //     Determines if it's connected to an outdoor area.
    // Return Value:
    //     True if the VisArea is connected to an outdoor area.
    virtual bool IsConnectedToOutdoor() const = 0;

    // Summary:
    //     Determines if the visarea ignores Global Illumination inside.
    // Return Value:
    //     True if the VisArea ignores Global Illumination inside.
    virtual bool IsIgnoringGI() const = 0;

    // Summary:
    //     Determines if the visarea ignores Outdoor Ambient occlusion inside.
    // Return Value:
    //     True if the VisArea ignores Outdoor Ambient Occlusion inside.
    virtual bool IsIgnoringOutdoorAO() const = 0;

    // Summary:
    //     Gets the name.
    // Notes:
    //     The name is always returned in lower case.
    // Return Value:
    //     A null terminated char array containing the name of the VisArea.
    virtual const char* GetName() = 0;

    // Summary:
    //     Determines if this VisArea is a portal.
    // Return Value:
    //     True if the VisArea is a portal, or false in the opposite case.
    virtual bool IsPortal() const = 0;

    // Description: 
    //     Searches for a specified VisArea to see if it's connected to the current 
    //     VisArea.
    // Arguments:
    //     pAnotherArea         - A specified VisArea to find
    //     nMaxRecursion        - The maximum number of recursion to do while searching
    //     bSkipDisabledPortals - Will avoid searching disabled VisAreas
    //         pVisitedAreas                - if not NULL - will get list of all visited areas
    // Return Value:
    //     True if the VisArea was found.
    // Summary:
    //     Searches for a specified VisArea.
    virtual bool FindVisArea(IVisArea* pAnotherArea, int nMaxRecursion, bool bSkipDisabledPortals) = 0;

    // Description: 
    //     Searches for the surrounding VisAreas which connected to the current 
    //     VisArea.
    // Arguments:
    //     nMaxRecursion        - The maximum number of recursion to do while searching
    //     bSkipDisabledPortals - Will avoid searching disabled VisAreas
    //       pVisitedAreas        - if not NULL - will get list of all visited areas
    // Return Value:
    //     None.
    // Summary:
    //     Searches for the surrounding VisAreas.
    virtual void FindSurroundingVisArea(int nMaxRecursion, bool bSkipDisabledPortals, PodArray<IVisArea*>* pVisitedAreas = NULL, int nMaxVisitedAreas = 0, int nDeepness = 0) = 0;

    // Summary:
    //     Determines if it's affected by outdoor lighting.
    // Return Value:
    //     Returns true if the VisArea if it's affected by outdoor lighting, else
    //     false will be returned.
    virtual bool IsAffectedByOutLights() const = 0;

    // Summary:
    //     Determines if the spere can be affect the VisArea.
    // Return Value:
    //     Returns true if the VisArea can be affected by the sphere, else
    //     false will be returned.
    virtual bool IsSphereInsideVisArea(const Vec3& vPos, const f32 fRadius) = 0;

    // Summary:
    //     Clips geometry inside or outside a vis area.
    // Return Value:
    //     Whether geom was clipped.
    virtual bool ClipToVisArea(bool bInside, Sphere& sphere, Vec3 const& vNormal) = 0;
    
    // Summary:
    //     Gives back the axis aligned bounding box of VisArea.
    // Return Value:
    //     Returns the pointer of a AABB.
    virtual const AABB* GetAABBox() const = 0;

    // Summary:
    //     Gives back the axis aligned bounding box of all static objects in the VisArea.
    //     This AABB can be huger than the ViaArea AABB as some objects might not be completely inside the VisArea.
    // Return Value:
    //     Returns the pointer to the AABB.
    virtual const AABB* GetStaticObjectAABBox() const = 0;

    // Summary:
    //     Determines if the point can be affect the VisArea.
    // Return Value:
    //     Returns true if the VisArea can be affected by the point, else
    //     false will be returned.
    virtual bool IsPointInsideVisArea(const Vec3& vPos) const = 0;

    virtual void GetShapePoints(const Vec3*& pPoints, size_t& nPoints) = 0;
    virtual float GetHeight() = 0;
    // </interfuscator:shuffle>
};

#include "OceanConstants.h"

// float m_SortId        : offseted by +WATER_LEVEL_SORTID_OFFSET if the camera object line is crossing the water surface
// : otherwise offseted by -WATER_LEVEL_SORTID_OFFSET
#define WATER_LEVEL_SORTID_OFFSET            10000000

#define DEFAULT_SID 0

// Summary:
//     indirect lighting quadtree definition.
namespace NQT
{
    // Forward declaration
    template <class TLeafContent, uint32 TMaxCellElems, class TPosType, class TIndexType, bool TUseRadius>
    class CQuadTree;
}

#define FILEVERSION_TERRAIN_SHLIGHTING_FILE 5

enum EVoxelBrushShape
{
    evbsSphere = 1,
    evbsBox,
};

enum EVoxelEditTarget
{
    evetVoxelObjects = 1,
};

enum EVoxelEditOperation
{
    eveoNone = 0,
  eveoPaintHeightPos,
  eveoPaintHeightNeg,
    eveoCreate,
    eveoSubstract,
    eveoMaterial,
  eveoBaseColor,
    eveoBlurPos,
  eveoBlurNeg,
    eveoCopyTerrainPos,
  eveoCopyTerrainNeg,
  eveoPickHeight,
  eveoIntegrateMeshPos,
  eveoIntegrateMeshNeg,
  eveoForceDepth,
  eveoLimitLod,
  eveoLast,
};

#define COMPILED_VISAREA_MAP_FILE_NAME         "terrain\\indoor.dat"
#define COMPILED_MERGED_MESHES_BASE_NAME       "terrain\\merged_meshes_sectors\\"
#define COMPILED_MERGED_MESHES_LIST            "mmrm_used_meshes.lst"
#define LEVEL_INFO_FILE_NAME                   "levelinfo.xml"

//////////////////////////////////////////////////////////////////////////

#pragma pack(push,4)

#define VISAREAMANAGER_CHUNK_VERSION 6

#define SERIALIZATION_FLAG_BIG_ENDIAN 1
#define SERIALIZATION_FLAG_SECTOR_PALETTES 2

#define TCH_FLAG2_AREA_ACTIVATION_IN_USE 1

struct SVisAreaManChunkHeader
{
  int8 nVersion;
  int8 nDummy;
  int8 nFlags;
  int8 nFlags2;
  int nChunkSize;
    int nVisAreasNum;
    int nPortalsNum;
    int nOcclAreasNum;

    AUTO_STRUCT_INFO
};

struct SOcTreeNodeChunk
{
  int16    nChunkVersion;
  int16 ucChildsMask;
  AABB    nodeBox;
  int32 nObjectsBlockSize;

  AUTO_STRUCT_INFO
};

struct SHotUpdateInfo
{
  SHotUpdateInfo()
  {
    nHeigtmap = 1;
    nObjTypeMask = ~0;
    pVisibleLayerMask = NULL;
    pLayerIdTranslation = NULL;
    areaBox.Reset();
  }

  uint32 nHeigtmap;
  uint32 nObjTypeMask;
  const uint8* pVisibleLayerMask;
  const uint16* pLayerIdTranslation;
  AABB areaBox;

  AUTO_STRUCT_INFO
};

#define OCTREE_CHUNK_VERSION 29

//==============================================================================================

// Summary:
//   Common header for binary files used by 3dengine
struct SCommonFileHeader
{
    char    signature[4];   // File signature, should be "CRY "
    uint8   file_type;      // File type
    uint8   flags;          // File common flags
    uint16  version;        // File version

    AUTO_STRUCT_INFO
};

struct IVisAreaCallback
{
    // <interfuscator:shuffle>
    virtual ~IVisAreaCallback(){}
    virtual void OnVisAreaDeleted(IVisArea* pVisArea) = 0;
    // </interfuscator:shuffle>
};

struct IVisAreaManager
{
    // <interfuscator:shuffle>
    virtual ~IVisAreaManager(){}
    // Summary:
    //   Loads data into VisAreaManager engine from memory block.
    virtual bool SetCompiledData(uint8* pData, int nDataSize, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable, bool bHotUpdate, SHotUpdateInfo* pExportInfo) = 0;

    // Summary:
    //   Saves data from VisAreaManager engine into memory block.
    virtual bool GetCompiledData(uint8* pData, int nDataSize, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial> >** ppMatTable, std::vector<struct IStatInstGroup*>** ppStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo = NULL) = 0;

    // Summary:
    //   Returns VisAreaManager data memory block size.
    virtual int GetCompiledDataSize(SHotUpdateInfo* pExportInfo = NULL) = 0;

    // Summary:
    //     Returns the accumulated number of visareas and portals.
    virtual int GetNumberOfVisArea() const = 0;

    // Summary:
    //     Returns the visarea interface based on the id (0..GetNumberOfVisArea()) it can be a visarea or a portal.
    virtual IVisArea* GetVisAreaById(int nID) const = 0;

    virtual void AddListener(IVisAreaCallback* pListener) = 0;
    virtual void RemoveListener(IVisAreaCallback* pListener) = 0;

    virtual void PrepareSegmentData(const AABB& box) = 0;
    virtual void ReleaseInactiveSegments() = 0;
    virtual bool CreateSegment(int nSID) = 0;
    virtual bool DeleteSegment(int nSID, bool bDeleteNow) = 0;
    virtual bool StreamCompiledData(uint8* pData, int nDataSize, int nSID, std::vector<struct IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, std::vector<struct IStatInstGroup*>* pStatInstGroupTable, const Vec2& vIndexOffset) = 0;
    virtual void OffsetPosition(const Vec3& delta) = 0;
    virtual void UpdateConnections() = 0;
    // Summary:
    //   Clones all vis areas in a region of the level, offsetting and rotating them based
    //   on the values passed in.
    // Arguments:
    //  offset - Offset amount, relative to the center of the region passed in.
    //  zRotation - Rotation around the z axis, in radians.
    virtual void CloneRegion(const AABB& region, const Vec3& offset, float zRotation) = 0;

    // Summary:
    //   Removes all vis areas in a region of the level.
    virtual void ClearRegion(const AABB& region) = 0;

    virtual void GetObjectsByType(PodArray<IRenderNode*>& lstObjects, EERType objType, const AABB* pBBox, ObjectTreeQueryFilterCallback filterCallback = nullptr) = 0;
    virtual void GetObjectsByFlags(uint dwFlags, PodArray<IRenderNode*>& lstObjects) = 0;

    virtual void GetObjects(PodArray<IRenderNode*>& lstObjects, const AABB* pBBox) = 0;

    virtual bool IsOutdoorAreasVisible() = 0;
};

struct SSkyLightRenderParams
{
    static const int skyDomeTextureWidth = 64;
    static const int skyDomeTextureHeight = 32;
    static const int skyDomeTextureSize = 64 * 32;

    static const int skyDomeTextureWidthBy8 = 8;
    static const int skyDomeTextureWidthBy4Log = 4; // = log2(64/4)
    static const int skyDomeTextureHeightBy2Log = 4; // = log2(32/2)

    SSkyLightRenderParams()
        : m_pSkyDomeMesh(0)
        , m_pSkyDomeTextureDataMie(0)
        , m_pSkyDomeTextureDataRayleigh(0)
        , m_skyDomeTexturePitch(0)
        , m_skyDomeTextureTimeStamp(-1)
        , m_partialMieInScatteringConst(0.0f, 0.0f, 0.0f, 0.0f)
        , m_partialRayleighInScatteringConst(0.0f, 0.0f, 0.0f, 0.0f)
        , m_sunDirection(0.0f, 0.0f, 0.0f, 0.0f)
        , m_phaseFunctionConsts(0.0f, 0.0f, 0.0f, 0.0f)
        , m_hazeColor(0.0f, 0.0f, 0.0f, 0.0f)
        , m_hazeColorMieNoPremul(0.0f, 0.0f, 0.0f, 0.0f)
        , m_hazeColorRayleighNoPremul(0.0f, 0.0f, 0.0f, 0.0f)
        , m_skyColorTop(0.0f, 0.0f, 0.0f)
        , m_skyColorNorth(0.0f, 0.0f, 0.0f)
        , m_skyColorEast(0.0f, 0.0f, 0.0f)
        , m_skyColorSouth(0.0f, 0.0f, 0.0f)
        , m_skyColorWest(0.0f, 0.0f, 0.0f)
    {
    }

    // Sky dome mesh
    _smart_ptr<IRenderMesh> m_pSkyDomeMesh;

    // temporarily add padding bytes to prevent fetching Vec4 constants below from wrong offset
    uint32 dummy0;
    uint32 dummy1;

    // Sky dome texture data
    const void* m_pSkyDomeTextureDataMie;
    const void* m_pSkyDomeTextureDataRayleigh;
    size_t m_skyDomeTexturePitch;
    int m_skyDomeTextureTimeStamp;

    int pad;//Enable 16 byte alignment for Vec4s

    // Sky dome shader constants
    Vec4 m_partialMieInScatteringConst;
    Vec4 m_partialRayleighInScatteringConst;
    Vec4 m_sunDirection;
    Vec4 m_phaseFunctionConsts;
    Vec4 m_hazeColor;
    Vec4 m_hazeColorMieNoPremul;
    Vec4 m_hazeColorRayleighNoPremul;

    // Sky hemisphere colors
    Vec3 m_skyColorTop;
    Vec3 m_skyColorNorth;
    Vec3 m_skyColorEast;
    Vec3 m_skyColorSouth;
    Vec3 m_skyColorWest;
};

struct SVisAreaInfo
{
  float fHeight;
  Vec3 vAmbientColor;
  bool bAffectedByOutLights;
  bool bIgnoreSkyColor;
  bool bSkyOnly;
  float fViewDistRatio;
  bool bDoubleSide;
  bool bUseDeepness;
  bool bUseInIndoors;
    bool bOceanIsVisible;
    bool bIgnoreGI;
    bool bIgnoreOutdoorAO;
    float fPortalBlending;
};

struct SDebugFPSInfo
{
    SDebugFPSInfo()
        : fAverageFPS(0.0f)
        , fMaxFPS(0.0f)
        , fMinFPS(0.0f)
    {
    }
    float fAverageFPS;
    float fMinFPS;
    float fMaxFPS;
};

struct SRainParams
{
    SRainParams()
        : fAmount(0.f)
        , fCurrentAmount(0.f)
        , fRadius(0.f)
        , nUpdateFrameID(-1)
        , bIgnoreVisareas(false)
        , bDisableOcclusion(false)
        , matOccTrans(IDENTITY)
        , matOccTransRender(IDENTITY)
        , qRainRotation(IDENTITY)
        , areaAABB(AABB::RESET)
        , bApplySkyColor(false)
        , fSkyColorWeight(0.5f)
    {
    }

    Matrix44        matOccTrans;          // Transformation matrix for rendering into a new occ map    
    Matrix44        matOccTransRender;    // Transformation matrix for rendering occluded rain using current occ map
    Quat            qRainRotation;        // Quaternion for the scene's rain entity rotation
    AABB            areaAABB;

    Vec3            vWorldPos;
    Vec3            vColor;

    float           fAmount;
    float           fCurrentAmount;
    float           fRadius;

    // Deferred rain params
    float           fFakeGlossiness; // unused
    float           fFakeReflectionAmount; // unused
    float           fDiffuseDarkening;
    float           fRainDropsAmount;
    float           fRainDropsSpeed;
    float           fRainDropsLighting;
    float           fMistAmount;
    float           fMistHeight;
    float           fPuddlesAmount;
    float           fPuddlesMaskAmount;
    float           fPuddlesRippleAmount;
    float            fSplashesAmount;

    int             nUpdateFrameID;

    bool            bApplyOcclusion;
    bool            bIgnoreVisareas;
    bool            bDisableOcclusion;

// Summary:
//     Common scene rain parameters shared across engine and editor
    bool            bApplySkyColor;
    float           fSkyColorWeight;

    // Bus ID to listen to
};

struct SSnowParams 
{
    SSnowParams() 
        : m_vWorldPos(0, 0, 0)
        , m_fRadius(0.0)
        , m_fSnowAmount(0.0)
        , m_fFrostAmount(0.0)
        , m_fSurfaceFreezing(0.0)
        , m_nSnowFlakeCount(0)
        , m_fSnowFlakeSize(0.0)
        , m_fSnowFallBrightness(0.0)
        , m_fSnowFallGravityScale(0.0)
        , m_fSnowFallWindScale(0.0)
        , m_fSnowFallTurbulence(0.0)
        , m_fSnowFallTurbulenceFreq(0.0)
    {
    }

    Vec3 m_vWorldPos;
    float m_fRadius;

    // Surface params.
    float m_fSnowAmount;
    float m_fFrostAmount;
    float m_fSurfaceFreezing;

    
    // Snowfall params.
    int m_nSnowFlakeCount;
    float m_fSnowFlakeSize;
    float m_fSnowFallBrightness;
    float m_fSnowFallGravityScale;
    float m_fSnowFallWindScale;
    float m_fSnowFallTurbulence;
    float m_fSnowFallTurbulenceFreq; 
};


struct IScreenshotCallback
{
    // <interfuscator:shuffle>
    virtual ~IScreenshotCallback(){}
    virtual void SendParameters(void* data, uint32 width, uint32 height, f32 minx, f32 miny, f32 maxx, f32 maxy) = 0;
    // </interfuscator:shuffle>
};

class IStreamedObjectListener
{
public:
    // <interfuscator:shuffle>
    virtual void OnCreatedStreamedObject(const char* filename, void* pHandle) = 0;
    virtual void OnRequestedStreamedObject(void* pHandle) = 0;
    virtual void OnReceivedStreamedObject(void* pHandle) = 0;
    virtual void OnUnloadedStreamedObject(void* pHandle) = 0;
    virtual void OnBegunUsingStreamedObjects(void** pHandles, size_t numHandles) = 0;
    virtual void OnEndedUsingStreamedObjects(void** pHandles, size_t numHandles) = 0;
    virtual void OnDestroyedStreamedObject(void* pHandle) = 0;
    // </interfuscator:shuffle>
protected:
    virtual ~IStreamedObjectListener() {}
};

#pragma pack(push, 16)

struct SFogVolumeData
{
    AABB avgAABBox;
    ColorF fogColor;
    int m_volumeType;
    Vec3 m_heightFallOffBasePoint;
    float m_densityOffset;
    Vec3 m_heightFallOffDirScaled;
    float m_globalDensity;
   
    SFogVolumeData() : 
        avgAABBox(AABB::RESET),
        m_globalDensity(0.0f),
        m_densityOffset(0.0f),
        m_volumeType(0),
        m_heightFallOffBasePoint(Vec3(0, 0, 0)),
        m_heightFallOffDirScaled(Vec3(0, 0, 0)),
        fogColor(ColorF(1.0f, 1.0f, 1.0f, 1.0f))
    {
    }

};

// Summary:
//     Light volumes data

#define LIGHTVOLUME_MAXLIGHTS 16

struct SLightVolume
{
    struct SLightData
    {
        SLightData()
            : vPos(0, 0, 0, 0)
            , vColor(0, 0, 0, 0)
            , vParams(0, 0, 0, 0) {}
        SLightData(const Vec4& vInPos, const Vec4& vInColor, const Vec4& vInParams)
            : vPos(vInPos)
            , vColor(vInColor)
            , vParams(vInParams) {}

        Vec4 vPos;
        Vec4 vColor;
        Vec4 vParams;
    };

    SLightVolume()
    {
        pData.reserve(LIGHTVOLUME_MAXLIGHTS);
    }

    typedef DynArray<SLightData> LightDataVector;

    DEFINE_ALIGNED_DATA(LightDataVector, pData, 16);
};

#pragma pack(pop)

struct CRNTmpData
{
    struct SRNUserData
    {
        int m_narrDrawFrames[MAX_RECURSION_LEVELS];
        SLodDistDissolveTransitionState lodDistDissolveTransitionState;
        Matrix34 objMat;
        OcclusionTestClient m_OcclState;
        struct IClipVolume* m_pClipVolume;
        SBending m_Bending;
        SBending m_BendingPrev;
        Vec3 vCurrentWind;
        uint32 nBendingLastFrame : 29;
        uint32 bWindCurrent : 1;
        uint32 bBendingSet : 1;
        uint16 nCubeMapId;
        uint16 nCubeMapIdCacheClearCounter;
        uint8 nWantedLod;
        CRenderObject* m_pRenderObject[MAX_STATOBJ_LODS_NUM];
        CRenderObject* m_arrPermanentRenderObjects[MAX_STATOBJ_LODS_NUM];
    } userData;

    CRNTmpData() { memset(this, 0, sizeof(*this)); assert((void*)this == (void*)&this->userData); nPhysAreaChangedProxyId = ~0; }
    CRNTmpData* pNext, * pPrev;
    CRNTmpData** pOwnerRef;
    uint32 nFrameInfoId;
    uint32 nPhysAreaChangedProxyId;

    void Unlink()
    {
        if (!pNext || !pPrev)
        {
            return;
        }
        pNext->pPrev = pPrev;
        pPrev->pNext = pNext;
        pNext = pPrev = NULL;
    }

    void Link(CRNTmpData* Before)
    {
        if (pNext || pPrev)
        {
            return;
        }
        pNext = Before->pNext;
        Before->pNext->pPrev = this;
        Before->pNext = this;
        pPrev = Before;
    }

    int Count()
    {
        int nCounter = 0;
        for (CRNTmpData* pElem = pNext; pElem != this; pElem = pElem->pNext)
        {
            nCounter++;
        }
        return nCounter;
    }

    void OffsetPosition(const Vec3& delta)
    {
        userData.objMat.SetTranslation(userData.objMat.GetTranslation() + delta);
    }
};


// Summary:
//     Interface to the 3d Engine.
struct I3DEngine
    : public IProcess
{    
    struct SObjectsStreamingStatus
    {
        int nReady;
        int nInProgress;
        int nTotal;
        int nActive;
        int nAllocatedBytes;
        int nMemRequired;
        int nMeshPoolSize; // in MB

    };

    struct OceanAnimationData
    {
        float fWindDirection;
        float fWindSpeed;
        float fWavesSpeed;
        float fWavesAmount;
        float fWavesSize;
        float fWindDirectionU;
        float fWindDirectionV;
    };

    struct SStremaingBandwidthData
    {
        SStremaingBandwidthData()
        {
            memset(this, 0, sizeof(SStremaingBandwidthData));
        }
        float fBandwidthActual;
        float fBandwidthRequested;
    };

    enum eStreamingSubsystem
    {
        eStreamingSubsystem_Textures,
        eStreamingSubsystem_Objects,
        eStreamingSubsystem_Audio,
    };

    using LoadStaticObjectAsyncResult = AZStd::function<void(_smart_ptr<IStatObj>)>;

    struct StaticObjectAsyncLoadRequest
    {
        StaticObjectAsyncLoadRequest() = default;

        LoadStaticObjectAsyncResult m_callback;
        string m_filename;
        string m_geomName;
        bool m_useStreaming;
        unsigned long m_loadingFlags;
    };

    struct CausticsParams
    {
        float tiling = 0.0f;
        float distanceAttenuation = 0.0f;
        float height = 0.0f;
        float depth = 0.0f;
        float intensity = 0.0f;
    };


    // <interfuscator:shuffle>
    // Summary:
    //     Initializes the 3D Engine.
    // See Also:
    //     ShutDown
    // Notes:
    //     Only call once, after creating the instance.
    virtual bool Init() = 0;


    // Summary:
    //     Sets the path used to load levels.
    // See Also:
    //     LoadLevel
    // Arguments:
    //     szFolderName - Should contains the folder to be used
    virtual void SetLevelPath(const char* szFolderName) = 0;

    virtual bool CheckMinSpec(uint32 nMinSpec) = 0;

    virtual void PrepareOcclusion(const CCamera& rCamera) = 0;
    virtual void EndOcclusion() = 0;
    // Description:
    //     Will load a level from the folder specified with SetLevelPath. If a 
    //     level is already loaded, the resources will be deleted before.
    // See Also:
    //     SetLevelPath
    // Arguments:
    //     szFolderName - Name of the subfolder to load
    //     szMissionName - Name of the mission
    // Return Value:
    //     A boolean which indicate the result of the function; true is 
    //     succeed, or false if failed.
    // Summary:
    //     Load a level.
    virtual bool LoadLevel(const char* szFolderName, const char* szMissionName) = 0;
    virtual bool InitLevelForEditor(const char* szFolderName, const char* szMissionName) = 0;
    virtual bool LevelLoadingInProgress() = 0;
    // Summary:
    //     Handles any work needed at start of new frame.
    // Notes:
    //     Should be called for every frame.
    virtual void OnFrameStart() = 0;

    // Description:
    //    Must be called after the game completely finishes loading the level.
    //    3D engine uses it to pre-cache some resources needed for rendering.
    // See Also:
    //    LoadLevel
    // Summary:
    //      Pre-caches some resources need for rendering.
    virtual void PostLoadLevel() = 0;

    // Description:
    //    Loads the required assets for a null level.
    virtual void LoadEmptyLevel() = 0;

    // Summary:
    //   Clears all rendering resources, all objects, characters and materials, voxels and terrain.
    // Notes:
    //   Should always be called before LoadLevel, and also before loading textures from a script.
    virtual void UnloadLevel() = 0;


    // Summary:
    //     Updates the 3D Engine.
    // Notes:
    //     Should be called for every frame.
    virtual void Update() = 0;

    // Summary:
    //    Returns the Camera used for Rendering on 3DEngine Side, normaly equal to the view camera, except if frozen with e_camerafreeze
    // Notes:
    //      Only valid during RenderWorld(else the camera of the last frame is used)
    //      This is the camera which should be used for all Engine side culling (since e_camerafreeze allows easy debugging then)
    virtual const CCamera& GetRenderingCamera() const = 0;
    virtual float GetZoomFactor() const = 0;

    // Summary:
    // clear all per frame temp data used in SRenderingPass
    virtual void Tick() = 0;

    // Summary:
    //   Update all ShaderItems flags, only required after shaders were reloaded at runtime
    virtual void UpdateShaderItems() = 0;

    // Summary:
    //     Deletes the 3D Engine instance.
    virtual void Release() = 0;

    // Summary:
    //     Draws the world.
    // See Also:
    //     SetCamera
    // Arguments:
    //   szDebugName - name that can be visualized for debugging purpose, must not be 0
    virtual void RenderWorld(const int nRenderFlags, const SRenderingPassInfo& passInfo, const char* szDebugName) = 0;

    virtual void RenderSceneReflection(const int nRenderFlags, const SRenderingPassInfo& passInfo) = 0;

    // Summary:
    //  Prepares for the world stream update, should be called before rendering
    virtual void PreWorldStreamUpdate(const CCamera& cam) = 0;

    // Summary:
    //  Performs the actual world streaming update. PreWorldStreamUpdate must be called before
    virtual void WorldStreamUpdate() = 0;

    // Summary:
    //     Shuts down the 3D Engine.
    virtual void ShutDown() = 0;

    // Summary:
    //     Loads a static object from a CGF file.  Does not increment the static object's reference counter.  The reference returned is not guaranteed to be valid when run outside the main thread.  Best used for priming the cache
    // See Also:
    //     IStatObj
    // Arguments:
    //     fileName - CGF Filename - should not be 0 or ""
    //     geomName - Optional name of geometry inside CGF.
    //     subObject - [Out]Optional Out parameter,Pointer to the
    //     loadingFlags - Zero or a bitwise combination of the flags from ELoadingFlags, defined in IMaterial.h, under the interface IMaterialManager.
    //     data - Raw buffer contain CGF data.
    //     dataSize - Size of the raw data buffer.
    // Return Value:
    //     A pointer to an object derived from IStatObj.
    virtual IStatObj* LoadStatObjUnsafeManualRef(const char* fileName, const char* geomName = nullptr, /*[Out]*/ IStatObj::SSubObject** subObject = nullptr, 
        bool useStreaming = true, unsigned long loadingFlags = 0, const void* data = nullptr, int dataSize = 0) = 0;

    // Summary:
    //     Loads a static object from a CGF file.  Increments the static object's reference counter.  This method is threadsafe.  Not suitable for preloading
    // See Also:
    //     IStatObj
    // Arguments:
    //     fileName - CGF Filename - should not be 0 or "", even if data is provided.
    //     geomName - Optional name of geometry inside CGF.
    //     subObject - [Out]Optional Out parameter,Pointer to the
    //     loadingFlags - Zero or a bitwise combination of the flags from ELoadingFlags, defined in IMaterial.h, under the interface IMaterialManager.
    //     data - Raw buffer contain CGF data.
    //     dataSize - Size of the raw data buffer.
    // Return Value:
    //     A smart pointer to an object derived from IStatObj.
    virtual _smart_ptr<IStatObj> LoadStatObjAutoRef(const char* fileName, const char* geomName = nullptr, /*[Out]*/ IStatObj::SSubObject** subObject = nullptr, 
        bool useStreaming = true, unsigned long loadingFlags = 0, const void* data = nullptr, int dataSize = 0) = 0;

    // Summary:
    //     Flushes queued async mesh loads.
    virtual void ProcessAsyncStaticObjectLoadRequests() = 0;

    // Summary:
    //     Loads a static object from a CGF file asynchronously and invokes a callback on completion.
    //     Actual load will occur on the main thread.
    // See Also:
    //     IStatObj
    // Arguments:
    //     callback - The user callback to invoke on completion.
    //     szFileName - CGF Filename - should not be 0 or ""
    //     szGeomName - Optional name of geometry inside CGF.
    //     ppSubObject - [Out]Optional Out parameter,Pointer to the
    //     nLoadingFlags - Zero or a bitwise combination of the flags from ELoadingFlags,
    //     defined in IMaterial.h, under the interface IMaterialManager.
    // Return Value:
    //     None
    virtual void LoadStatObjAsync(LoadStaticObjectAsyncResult resultCallback, const char* szFileName, const char* szGeomName = nullptr, bool bUseStreaming = true, unsigned long nLoadingFlags = 0) = 0;

    // Summary:
    //     Finds a static object created from the given filename
    // See Also:
    //     IStatObj
    // Arguments:
    //     szFileName - CGF Filename - should not be 0 or ""
    // Return Value:
    //     A pointer to an object derived from IStatObj.
    virtual IStatObj* FindStatObjectByFilename(const char* filename) = 0;

    // Summary:
    //     Gets gsm range
    // Return Value:
    //     An integer representing the gsm range.
    virtual const float GetGSMRange() = 0;

    // Summary:
    //     Gets gsm range step
    // Return Value:
    //     An integer representing the gsm range step
    virtual const float GetGSMRangeStep() = 0;

    // Summary:
    //     Gets the amount of loaded objects.
    // Return Value:
    //     An integer representing the amount of loaded objects.
    virtual int GetLoadedObjectCount() { return 0; }

    // Summary:
    //     Fills pObjectsArray with pointers to loaded static objects
    //     if pObjectsArray is NULL only fills nCount parameter with amount of loaded objects.
    virtual void GetLoadedStatObjArray(IStatObj** pObjectsArray, int& nCount) = 0;

    // Summary:
    //     Gets stats on streamed objects
    virtual void GetObjectsStreamingStatus(SObjectsStreamingStatus& outStatus) = 0;

    // Summary:
    //        Gets stats on the streaming bandwidth requests from subsystems
    // Arguments:
    //        subsystem - the streaming subsystem we want bandwidth data for
    //        outData - structure containing the bandwidth data for the subsystem requested
    virtual void GetStreamingSubsystemData(int subsystem, SStremaingBandwidthData& outData) = 0;

    // Summary:
    //     Registers an entity to be rendered.
    // Arguments:
    //     pEntity - The entity to render
    virtual void RegisterEntity(IRenderNode* pEntity, int nSID = -1, int nSIDConsideredSafe = -1) = 0;

    // Summary:
    //     Selects an entity for debugging.
    // Arguments:
    //     pEntity - The entity to render
    virtual void SelectEntity(IRenderNode* pEntity) = 0;

    //////////////////////////////////////////////
    // Start Confetti //////////////////////////
    // Summary:
    //     Return the setting for sun shadows. Return false for sub classes without sun shadows settings.
    virtual bool IsSunShadows(){ return false; };
    // End Confetti//////////////////////////////
    //////////////////////////////////////////////

    // Summary:
    //     Offers same functionality as Cry3DEngineBase::MakeSystemMaterialFromShader
    virtual _smart_ptr<IMaterial> MakeSystemMaterialFromShaderHelper(const char* sShaderName, SInputShaderResources* Res = NULL) = 0;

    virtual bool CheckMinSpecHelper(uint32 nMinSpec) = 0;

    virtual void OnCasterDeleted(IShadowCaster* pCaster) = 0;

    virtual void GetStatObjAndMatTables(DynArray<IStatObj*>* pStatObjTable, DynArray<_smart_ptr<IMaterial>>* pMatTable, DynArray<IStatInstGroup*>* pStatInstGroupTable, uint32 nObjTypeMask) = 0;

#ifndef _RELEASE
    enum EDebugDrawListAssetTypes
    {
        DLOT_ALL = 0,
        DLOT_CHARACTER = BIT(2),
        DLOT_STATOBJ = BIT(3)
    };

    struct SObjectInfoToAddToDebugDrawList
    {
        const char* pName;
        const char* pClassName;
        const char* pFileName;
        IRenderNode* pRenderNode;
        uint32    numTris;
        uint32    numVerts;
        uint32    texMemory;
        uint32    meshMemory;
        EDebugDrawListAssetTypes type;
        const AABB*            pBox;
        const Matrix34* pMat;
    };

    virtual void AddObjToDebugDrawList(SObjectInfoToAddToDebugDrawList& objInfo) = 0;
    virtual bool IsDebugDrawListEnabled() const = 0;
#endif


    // Summary:
    //     Notices the 3D Engine to stop rendering a specified entity.
    // Arguments:
    //     pEntity - The entity to stop render
    virtual void UnRegisterEntityDirect(IRenderNode* pEntity) = 0;
    virtual void UnRegisterEntityAsJob(IRenderNode* pEnt) = 0;
    // Summary:
    //       Returns whether a world pos is under water.
    virtual bool IsUnderWater(const Vec3& vPos) const = 0;
  
    // Summary:
    //       Returns whether ocean volume is visible or not.
    virtual void SetOceanRenderFlags(uint8 nFlags) = 0;
    virtual uint8 GetOceanRenderFlags() const = 0;
    virtual uint32 GetOceanVisiblePixelsCount() const = 0;

    // Summary:
    //     Gets the closest walkable bottom z straight beneath the given reference position.
    // Notes:
    //     This function will take into account both the global terrain elevation and local voxel (or other solid walkable object).
    // Arguments:
    //     referencePos - Position from where to start searching downwards.
    //     maxRelevantDepth - Max depth caller is interested in relative to referencePos (for ray casting performance reasons).
    //     objtypes - expects physics entity flags.  Use this to specify what object types make a valid bottom for you.
    // Return Value:
    //     A float value which indicate the global world z of the bottom level beneath the referencePos.
    //     If the referencePos is below terrain but not inside any voxel area BOTTOM_LEVEL_UNKNOWN is returned.
    virtual float GetBottomLevel(const Vec3& referencePos, float maxRelevantDepth, int objtypes) = 0;
    // A set of overloads for enabling users to use different sets of input params.  Basically, only
    // referencePos is mandatory.  The overloads as such don't need to be virtual but this seems to be
    // a purely virtual interface.
    virtual float GetBottomLevel(const Vec3& referencePos, float maxRelevantDepth = 10.0f) = 0;
    virtual float GetBottomLevel(const Vec3& referencePos, int objflags) = 0;

    // Summary:
    //     Gets the ocean water level. Fastest option, always prefer is only ocean height required.
    // Notes:
    //     This function will take into account just the global water level.
    // Return Value:
    //     A float value which indicate the water level. In case no water was 
    //     found at the specified location, the value WATER_LEVEL_UNKNOWN will 
    //     be returned.
    virtual float GetWaterLevel() = 0;

    // Summary:
    //     Gets the closest walkable bottom z straight beneath the given reference position.
    //                - Use with caution the accurate query - SLOW
    // Notes:    
    //     This function will take into account both the global water level and any water volume present.
    //     Function is provided twice for performance with diff. arguments.
    // Arguments:
    //     pvPos - Desired position to inspect the water level
    //     pent - Pointer to return the physical entity to test against (optional)
    // Return Value:
    //     A float value which indicate the water level. In case no water was 
    //     found at the specified location, the value WATER_LEVEL_UNKNOWN will 
    //     be returned.
    virtual float GetWaterLevel(const Vec3* pvPos, IPhysicalEntity* pent = NULL, bool bAccurate = false) = 0;

    // Summary:
    //     Gets the ocean water level for a specified position.
    //                - Use with caution the accurate query - SLOW
    // Notes:
    //     This function only takes into account ocean water.
    // Arguments:
    //     pCurrPos - Position to check water level
    // Return Value:
    //     A float value which indicate the water level.
    virtual float GetAccurateOceanHeight(const Vec3& pCurrPos) const = 0;

    // Summary:
    //     Gets caustics parameters.
    virtual CausticsParams GetCausticsParams() const = 0;
    
    // Summary:
    //     Gets ocean animation parameters.
    virtual OceanAnimationData GetOceanAnimationParams() const = 0;

    // Summary:
    //     Gets HDR setup parameters.
    // Return Value:
    virtual void GetHDRSetupParams    (Vec4 pParams[5]) const = 0; 

    // Summary:
    //     Removes all particles and decals from the world.
    virtual void ResetParticlesAndDecals() = 0;

    // Summary:
    //     Creates new decals on the walls, static objects, terrain and entities.
    // Arguments:
    //     Decal - Structure describing the decal effect to be applied
    virtual void CreateDecal(const CryEngineDecalInfo& Decal) = 0;

    // Summary:
    //     Removes decals in a specified range.
    // Arguments:
    //     vAreaBox - Specify the area in which the decals will be removed
    //     pEntity  - if not NULL will only delete decals attached to this entity
    virtual void DeleteDecalsInRange(AABB* pAreaBox, IRenderNode* pEntity) = 0;

    // Summary:
    //     Sets the current sun base color.
    virtual void SetSunColor(Vec3 vColor) = 0;

    // Summary:
    //     Gets the current sun animated color.
    virtual Vec3 GetSunAnimColor() = 0;

    // Summary:
    //     Sets the current sky brightening multiplier.
    virtual void SetSunAnimColor(const Vec3& color) = 0;

    // Summary:
    //     Sets the current sun animation speed.
    virtual float GetSunAnimSpeed() = 0;

    // Summary:
    virtual void SetSunAnimSpeed(float sunAnimSpeed) = 0;

    // Summary:
    //     Gets the current sun animation phase.
    virtual AZ::u8 GetSunAnimPhase() = 0;
    // Summary:
    //     Sets the current sun animation phase.
    virtual void SetSunAnimPhase(AZ::u8 sunAnimPhase) = 0;

    // Summary:
    //     Gets the current sun animation index.
    virtual AZ::u8 GetSunAnimIndex() = 0;

    // Summary:
    //     Sets the current sun animation index.
    virtual void SetSunAnimIndex(AZ::u8 sunAnimIndex) = 0;

    // Summary:
    //     Sets current rain parameters.
    virtual void SetRainParams(const SRainParams& rainParams) = 0;

    // Summary:
    //     Gets the validity and fills current rain parameters.
    virtual bool GetRainParams(SRainParams& rainParams) = 0;

    // Summary:
    //        Sets current snow surface parameters.
    virtual void SetSnowSurfaceParams(const Vec3& vCenter, float fRadius, float fSnowAmount, float fFrostAmount, float fSurfaceFreezing) = 0;

    // Summary:
    //        Gets current snow surface parameters.
    virtual bool GetSnowSurfaceParams(Vec3& vCenter, float& fRadius, float& fSnowAmount, float& fFrostAmount, float& fSurfaceFreezing) = 0;

    // Summary:
    //        Sets current snow parameters.
    virtual void SetSnowFallParams(int nSnowFlakeCount, float fSnowFlakeSize, float fSnowFallBrightness, float fSnowFallGravityScale, float fSnowFallWindScale, float fSnowFallTurbulence, float fSnowFallTurbulenceFreq) = 0;

    // Summary:
    //        Gets current snow parameters.
    virtual bool GetSnowFallParams(int& nSnowFlakeCount, float& fSnowFlakeSize, float& fSnowFallBrightness, float& fSnowFallGravityScale, float& fSnowFallWindScale, float& fSnowFallTurbulence, float& fSnowFallTurbulenceFreq) = 0;

    // Summary:
    //     Sets the view distance scale.
    // Arguments:
    //     fScale - may be between 0 and 1, 1.f = Unmodified view distance set by level designer, value of 0.5 will reduce it twice
  // Notes:
  //     This value will be reset automatically to 1 on next level loading.
  virtual void SetMaxViewDistanceScale(float fScale) = 0;

    // Summary:
    //     Gets the view distance.
    // Return Value:
    //     A float value representing the maximum view distance.
    virtual float GetMaxViewDistance(bool bScaled = true) = 0;

    virtual const SFrameLodInfo& GetFrameLodInfo() const = 0;
    virtual void SetFrameLodInfo(const SFrameLodInfo& frameLodInfo) = 0;

    // Summary:
    //     Sets the fog color.
    virtual void SetFogColor(const Vec3& vFogColor) = 0;

    // Summary:
    //     Gets the fog color.
    virtual Vec3 GetFogColor() = 0;

    // Summary:
    //   Gets various sky light parameters.
    virtual void GetSkyLightParameters(Vec3& sunDir, Vec3& sunIntensity, float& Km, float& Kr, float& g, Vec3& rgbWaveLengths) = 0;

    // Summary:
    //   Sets various sky light parameters.
    virtual void SetSkyLightParameters(const Vec3& sunDir, const Vec3& sunIntensity, float Km, float Kr, float g, const Vec3& rgbWaveLengths, bool forceImmediateUpdate = false) = 0;

    // Return Value:
    //   In logarithmic scale -4.0 .. 4.0
    virtual float GetLightsHDRDynamicPowerFactor() const = 0;

    // Return true if tessellation is allowed for given render object
    virtual bool IsTessellationAllowed(const CRenderObject* pObj, const SRenderingPassInfo& passInfo, bool bIgnoreShadowPass = false) const = 0;

    // allows to modify material on render nodes at run-time (make sure it is properly restored back)
    virtual void SetRenderNodeMaterialAtPosition(EERType eNodeType, const Vec3& vPos, _smart_ptr<IMaterial> pMat) = 0;

    // override the camera precache point with the requested position for the current round
    virtual void OverrideCameraPrecachePoint(const Vec3& vPos) = 0;

    // begin streaming of meshes and textures for specified position, pre-cache stops after fTimeOut seconds
    virtual int AddPrecachePoint(const Vec3& vPos, const Vec3& vDir, float fTimeOut = 3.f, float fImportanceFactor = 1.0f) = 0;
    virtual void ClearPrecachePoint(int id) = 0;
    virtual void ClearAllPrecachePoints() = 0;

    virtual void GetPrecacheRoundIds(int pRoundIds[MAX_STREAM_PREDICTION_ZONES]) = 0;

    virtual void TraceFogVolumes(const Vec3& vPos, const AABB& objBBox, SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo, bool fogVolumeShadingQuality) = 0;

    //DOC-IGNORE-BEGIN

    // Internal functions, mostly used by the editor, which won't be documented for now

    // Summary:
    //        Removes all static objects on the map (for editor)
    virtual void RemoveAllStaticObjects(int nSID = DEFAULT_SID) = 0;

    // Summary:
    //        Sets group parameters
    virtual bool SetStatInstGroup(int nGroupId, const IStatInstGroup& siGroup, int nSID = 0) = 0;

    // Summary:
    //        Gets group parameters
    virtual bool GetStatInstGroup(int nGroupId, IStatInstGroup& siGroup, int nSID = 0) = 0;

    //DOC-IGNORE-END

    // Summary:
    //   Notifies of an explosion, and maybe creates an hole in the terrain
    // Description:
    //   This function should usually make sure that no static objects are near before making the hole.
    // Arguments:
    //   vPos - Position of the explosion
    //   fRadius - Radius of the explosion
    //   bDeformTerrain - Allow to deform the terrain
    virtual void OnExplosion(Vec3 vPos, float fRadius, bool bDeformTerrain = true) = 0;

    // Summary:
    //   Sets the physics material enumerator
    // Arguments:
    //   pPhysMaterialEnumerator - The physics material enumarator to set
    virtual void SetPhysMaterialEnumerator(IPhysMaterialEnumerator* pPhysMaterialEnumerator) = 0;

    // Summary:
    //   Gets the physics material enumerator
    // Return Value:
    //   A pointer to an IPhysMaterialEnumerator derived object.
    virtual IPhysMaterialEnumerator* GetPhysMaterialEnumerator() = 0;

    //DOC-IGNORE-BEGIN
    //Internal functions
    
    // Summary:
    //     Allows to enable fog in editor
    virtual void SetupDistanceFog() = 0;

    // Summary:
    //     Loads environment settings for specified mission
    virtual void LoadMissionDataFromXMLNode(const char* szMissionName) = 0;

    virtual void LoadEnvironmentSettingsFromXML(XmlNodeRef pInputNode, int nSID = DEFAULT_SID) = 0;

    // Summary:
    //      This one is called by the editor when the terrain editing tools are not being built.
    //      This is the case when the game developers are using external tools to author terrain.
    //      The Octree data and the terrain heightmap data are stored in the same file.
    //      This method makes sure only the non-terrain data of the Octree is loaded.
    virtual bool LoadCompiledOctreeForEditor() = 0;

    //DOC-IGNORE-END

    //DOC-IGNORE-BEGIN
    // Set direction to the sun
    //  virtual void SetSunDir( const Vec3& vNewSunDir ) = 0;

    // Summary:
    //        Return non-normalized direction to the sun
    virtual Vec3 GetSunDir()  const = 0;

    // Summary:
    //        Return normalized direction to the sun
    virtual Vec3 GetSunDirNormalized()  const = 0;

    // Summary:
    //        Return realtime (updated every frame with real sun position) normalized direction to the scene
    virtual Vec3 GetRealtimeSunDirNormalized()  const = 0;

    //Internal function used by 3d engine and renderer
    // Summary:
    //      Gets distance to the sector containig ocean water
    virtual float GetDistanceToSectorWithWater() = 0;
    //DOC-IGNORE-END

    // Summary:
    //   Gets the sun color
    // Notes:
    //   Should have been specified in the editor.
    // Return Value:
    //   An rgb value contained in a Vec3 object.
    virtual Vec3 GetSunColor() const = 0;

    // Summary:
    //   Retrieves the current SSAO multiplier
    // Notes:
    // Return Value:
    //   scalar value
    virtual float GetSSAOAmount() const = 0;

    // Summary:
    //   Retrieves the current SSAO contrast multiplier
    // Notes:
    // Return Value:
    //   scalar value
    virtual float GetSSAOContrast() const = 0;
    // Summary:
    //     Frees entity render info.
    virtual void FreeRenderNodeState(IRenderNode* pEntity) = 0;

    // Summary:
    //   Adds the level's path to a specified filename.
    // Arguments:
    //   szFileName - The filename for which we need to add the path
    // Return Value:
    //   Full path for the filename; including the level path and the filename appended after.
    virtual const char* GetLevelFilePath(const char* szFileName) = 0;

    // Summary:
    //   Displays statistic on the 3d Engine.
    // Arguments:
    //   fTextPosX - X position for the text
    //   fTextPosY - Y position for the text
    //   fTextStepY - Amount of pixels to distance each line
    //   bEnhanced - false=normal, true=more interesting information
    virtual void DisplayInfo(float& fTextPosX, float& fTextPosY, float& fTextStepY, const bool bEnhanced) = 0;

    // Summary:
    //   Displays CPU and GPU memory usage statistics on screen
    virtual void DisplayMemoryStatistics() = 0;

    // Summary:
    //   Draws text right aligned at the y pixel precision.
    virtual void DrawTextRightAligned(const float x, const float y, const char* format, ...) PRINTF_PARAMS(4, 5) = 0;
    virtual void DrawTextRightAligned(const float x, const float y, const float scale, const ColorF& color, const char* format, ...) PRINTF_PARAMS(6, 7) = 0;

    virtual void DrawBBoxHelper(const Vec3& vMin, const Vec3& vMax, ColorB col = Col_White) = 0;
    virtual void DrawBBoxHelper(const AABB& box, ColorB col = Col_White) = 0;

    // Summary:
    //   Enables or disables portal at a specified position.
    // Arguments:
    //   vPos - Position to place the portal
    //   bActivate - Set to true in order to enable the portal, or to false to disable
    //   szEntityName -
    virtual void ActivatePortal(const Vec3& vPos, bool bActivate, const char* szEntityName) = 0;

    //DOC-IGNORE-BEGIN
    // Summary:
    //     Counts memory usage
    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    // Summary:
    //     Counts resource memory usage
    // Arguments:
    //        cstAABB - Use the whole level AABB if you want to grab the resources
    //              from the whole level.  For height level, use something BIG 
    //                            (ie: +-FLT_MAX)
    virtual void GetResourceMemoryUsage(ICrySizer* pSizer, const AABB& cstAABB) = 0;
    //DOC-IGNORE-END

    // Summary:
    //   Creates a new VisArea.
    // Return Value:
    //   A pointer to a newly created VisArea object
    virtual IVisArea* CreateVisArea(uint64 visGUID) = 0;

    // Summary:
    //   Deletes a VisArea.
    // Arguments:
    //   pVisArea - A pointer to the VisArea to delete
    virtual void DeleteVisArea(IVisArea* pVisArea) = 0;

    //mat: todo

    // Summary:
    //   Updates the VisArea
    // Arguments:
    //   pArea -
    //   pPoints - 
    //   nCount -
    //   szName -
    //     info    -
    //     bReregisterObjects -
    virtual void UpdateVisArea(IVisArea* pArea, const Vec3* pPoints, int nCount, const char* szName, const SVisAreaInfo& info, bool bReregisterObjects) = 0;

    // Summary:
    //   Determines if two VisAreas are connected.
    // Description:
    //   Used to determine if a sound is potentially hearable between two VisAreas.
    // Arguments:
    //   pArea1 - A pointer to a VisArea
    //   pArea2 - A pointer to a VisArea
    //   nMaxRecursion - Maximum number of recursions to be done
    //   bSkipDisabledPortals - Indicate if disabled portals should be skipped
    // Return Value:
    //   A boolean value set to true if the two VisAreas are connected, else false will be returned.
    virtual bool IsVisAreasConnected(IVisArea* pArea1, IVisArea* pArea2, int nMaxRecursion = 1, bool bSkipDisabledPortals = true) = 0;

    // Summary:
    //   Creates a ClipVolume.
    // Return Value:
    //   A pointer to a newly created ClipVolume object
    virtual IClipVolume* CreateClipVolume() = 0;
        
    // Summary:
    //   Deletes a ClipVolume.
    // Arguments:
    //   pClipVolume - A pointer to the ClipVolume to delete
    virtual void DeleteClipVolume(IClipVolume* pClipVolume) = 0;

    // Summary:
    //   Updates a ClipVolume
    // Arguments:
    //   pClipVolume - Pointer to volume that needs updating
    //   pRenderMesh - Pointer to new render mesh
    //   worldTM - Updated world transform
    //   szName - Updated ClipVolume name
    virtual void UpdateClipVolume(IClipVolume* pClipVolume, _smart_ptr<IRenderMesh> pRenderMesh, IBSPTree3D* pBspTree, const Matrix34& worldTM, bool bActive, uint32 flags, const char* szName) = 0;

    //mat: todo

    // Summary:
    //   Creates instance of IRenderNode object with specified type.
    virtual IRenderNode* CreateRenderNode(EERType type) = 0;

    // Summary:
    //     Delete RenderNode object.
    virtual void DeleteRenderNode(IRenderNode* pRenderNode) = 0;


    // Summary:
    //     Gets wind direction and force, averaged within a box.
    virtual Vec3 GetWind(const AABB& box, bool bIndoors) const = 0;

    // Summary:
    //     Gets the global wind vector.
    virtual Vec3 GetGlobalWind(bool bIndoors) const = 0;

    // Summary:
    //     Gets wind direction and forace at the sample points provided. 
    // Note: the positions defining the samples will be overwritten 
    // with the accumulated wind influences.
    virtual bool SampleWind(Vec3* pSamples, int nSamples, const AABB& volume, bool bIndoors) const = 0;

    // Description:
    //   Gets the VisArea which is present at a specified point.
    // Arguments:
    //     vPos: 
    // Return Value:
    //     VisArea containing point, if any. 0 otherwise.
    virtual IVisArea* GetVisAreaFromPos(const Vec3& vPos) = 0;

    // Description:
    //   Tests for intersection against Vis Areas.
    // Arguments:
    //   box: Volume to test for intersection.
    //   pNodeCache (out, optional): Set to a cached pointer, for quicker calls to ClipToVisAreas.
    // Return Value:
    //     Whether box intersects any vis areas.
    virtual    bool IntersectsVisAreas(const AABB& box, void** pNodeCache = 0) = 0;    

    // Description:
    //   Clips geometry against the boundaries of VisAreas.
    // Arguments:
    //   pInside: Vis Area to clip inside of. If 0, clip outside all Vis Areas.
    //     sphere -
    //     vNormal -
    //     pNodeChache -
    // Return Value:
    //     Whether it was clipped
    virtual    bool ClipToVisAreas(IVisArea* pInside, Sphere& sphere, Vec3 const& vNormal, void* pNodeCache = 0) = 0;    

    // Summary:
    //   Enables or disables ocean rendering.
    // Arguments:
    //   bOcean - Will enable or disable the rendering of ocean
    virtual void EnableOceanRendering(bool bOcean) = 0;

    // Description:
    //      Register a texture load handler
    virtual void AddTextureLoadHandler(ITextureLoadHandler* pHandler) = 0;

    // Description
    //      Unregister a texture load handler
    virtual void RemoveTextureLoadHandler(ITextureLoadHandler* pHandler) = 0;

    // Description:
    //      Get a texture load handler for a given extension
    virtual ITextureLoadHandler* GetTextureLoadHandlerForImage(const char* ext) = 0;
    // Summary:
    //   Creates a new light source.
    // Return Value:
    //   Pointer to newly created light or -1 if it fails.
    virtual struct ILightSource* CreateLightSource() = 0;

    // Summary:
    //   Deletes a light.
    // Arguments:
    //   Pointer to the light
    virtual void DeleteLightSource(ILightSource* pLightSource) = 0;

    // Summary:
    //   Gives access to the list holding all static light sources
    // Return Value:
    //   An array holding all the CDLight pointers.
    virtual const PodArray<ILightSource*>* GetLightEntities() = 0;

    // Summary:
    //   Gives access to list holding all lighting volumes
    // Return Value:
    //   An array holding all the SLightVolume pointers.
    virtual void GetLightVolumes(threadID nThreadID, SLightVolume*& pLightVols, uint32& nNumVols) = 0;

    // Summary:
    //   Registers a volume for lighting
    // Return Value:
    //   The index of the registered volume.
    virtual uint16 RegisterVolumeForLighting(const Vec3& vPos, f32 fRadius, uint8 nClipVolumeRef, const SRenderingPassInfo& passInfo) = 0;

    // Summary:
    //   Reload the heightmap.
    // Description:
    //   Reloading the heightmap will resets all decals and particles.
    // Notes:
    //     In future will restore deleted vegetations
    // Returns:
    //   success
    virtual bool RestoreTerrainFromDisk(int nSID = 0) = 0;

    //DOC-IGNORE-BEGIN
  // tmp
    virtual const char* GetFilePath(const char* szFileName) { return GetLevelFilePath(szFileName); }
    //DOC-IGNORE-END

    // Summary:
    //     Post processing effects interfaces.
    virtual class IPostEffectGroupManager* GetPostEffectGroups() const = 0;
    virtual class IPostEffectGroup* GetPostEffectBaseGroup() const = 0;

    // Most code should use either GetPostEffectGroups() or GetPostEffectBaseGroup() instead of these
    virtual void SetPostEffectParam(const char* pParam, float fValue, bool bForceValue = false) const = 0;
    virtual void SetPostEffectParamVec4(const char* pParam, const Vec4& pValue, bool bForceValue = false) const = 0;
    virtual void SetPostEffectParamString(const char* pParam, const char* pszArg) const = 0;

    virtual void GetPostEffectParam(const char* pParam, float& fValue) const = 0;
    virtual void GetPostEffectParamVec4(const char* pParam, Vec4& pValue) const = 0;
    virtual void GetPostEffectParamString(const char* pParam, const char*& pszArg) const = 0;

    virtual int32 GetPostEffectID(const char* pPostEffectName) = 0;

    virtual void ResetPostEffects(bool bOnSpecChange = false) = 0;
    //Disable post effect groups other than default and base.
    virtual void DisablePostEffects() = 0;

    virtual void SetShadowsGSMCache(bool bCache) = 0;
    virtual void SetCachedShadowBounds(const AABB& shadowBounds, float fAdditionalCascadesScale) = 0;
    virtual void SetRecomputeCachedShadows(uint nUpdateStrategy = 0) = 0;

    // Summary:
    //     In debug mode check memory heap and makes assert, do nothing in release
    virtual void CheckMemoryHeap() = 0;
    
    // Summary:
    //     Removes all decals attached to specified entity.
    virtual void DeleteEntityDecals(IRenderNode* pEntity) = 0;

    // Summary:
    //     Disables CGFs unloading.
    virtual void LockCGFResources() = 0;

    // Summary:
    //     Enables CGFs unloading (this is default state), this function will also release all not used CGF's.
    virtual void UnlockCGFResources() = 0;

    // Summary:
    //     Release all unused CGFs.
    virtual void FreeUnusedCGFResources() = 0;


    // Summary:
    //     Creates static object containing empty IndexedMesh.
    virtual IStatObj* CreateStatObj() = 0;
    virtual IStatObj* CreateStatObjOptionalIndexedMesh(bool createIndexedMesh) = 0;

    // Summary:
    //   Creates the instance of the indexed mesh.
    virtual IIndexedMesh* CreateIndexedMesh() = 0;

    // Summary:
    //     Saves/loads state of engine objects.
    virtual void SerializeState(TSerialize ser) = 0;
    
    // Summary:
    //     Cleanups after save/load.    
    virtual void PostSerialize(bool bReading) = 0;

    // Description:
    //    Retrieve pointer to the material i/o interface.
    virtual IMaterialHelpers& GetMaterialHelpers() = 0;

    // Description:
    //    Retrieve pointer to the material manager interface.
    virtual IMaterialManager* GetMaterialManager() = 0;

    // Description:
    //    Retrieve pointer to the object manager interface.
    virtual IObjManager* GetObjManager() = 0;

    //////////////////////////////////////////////////////////////////////////
    // CGF Loader.
    //////////////////////////////////////////////////////////////////////////
  // Description:
    // Creates a chunkfile content instance
  // Returns 'NULL' if the memory for the instance could not be allocated
    virtual CContentCGF* CreateChunkfileContent(const char* filename) = 0;

  // Description:
    // Deletes the chunkfile content instance
    virtual void ReleaseChunkfileContent(CContentCGF*) = 0;

  // Description:
    // Loads the contents of a chunkfile into the given CContentCGF. 
  // Returns 'false' on error.
    virtual bool LoadChunkFileContent(CContentCGF* pCGF, const char* filename, bool bNoWarningMode = false, bool bCopyChunkFile = true) = 0;

  // Description:
    // Loads the contents of a chunkfile into the given CContentCGF. 
  // Returns 'false' on error.
    virtual bool LoadChunkFileContentFromMem(CContentCGF* pCGF, const void* pData, size_t nDataLen, uint32 nLoadingFlags, bool bNoWarningMode = false, bool bCopyChunkFile = true) = 0;

    // Description:
    //    Creates ChunkFile.
    virtual IChunkFile* CreateChunkFile(bool bReadOnly = false) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Chunk file writer.
    //////////////////////////////////////////////////////////////////////////
    enum EChunkFileFormat
    {
        eChunkFileFormat_0x745,
        eChunkFileFormat_0x746,
    };    
    virtual ChunkFile::IChunkFileWriter* CreateChunkFileWriter(EChunkFileFormat eFormat, AZ::IO::IArchive* pPak, const char* filename) const = 0;
    virtual void ReleaseChunkFileWriter(ChunkFile::IChunkFileWriter* p) const = 0;

    // Description:
    //      Returns true if the ocean was created successfully.
    virtual bool CreateOcean(_smart_ptr<IMaterial> pTerrainWaterMat, float waterLevel) = 0;

    // Description:
    //      Deletes the ocean if it exists, otherwise does nothing.
    virtual void DeleteOcean() = 0;

    // Description:
    //      Changes ocean material if the ocean exists.
    virtual void ChangeOceanMaterial(_smart_ptr<IMaterial> pMat) = 0;

    // Description:
    //      Changes ocean water level if the ocean exists.
    virtual void ChangeOceanWaterLevel(float fWaterLevel) = 0;

    virtual void InitMaterialDefautMappingAxis(_smart_ptr<IMaterial> pMat) = 0;

    // Description:
    //      Returns interface to visarea manager.
    virtual IVisAreaManager* GetIVisAreaManager() = 0;

    // Description:
    //      Places camera into every visarea or every manually set pre-cache points and render the scenes.
    virtual void PrecacheLevel(bool bPrecacheAllVisAreas, Vec3* pPrecachePoints, int nPrecachePointsNum) = 0;

    // Description:
    //      Proposes 3dengine to load on next frame all shaders and textures synchronously.
    virtual void ProposeContentPrecache() = 0;
    


    // Description:
    //      Returns TOD interface.
    virtual ITimeOfDay* GetTimeOfDay() = 0;

    // Description:
    //    Updates the sky material paths.  LoadSkyMaterial will handle loading these materials.
    virtual void SetSkyMaterialPath(const string& skyMaterialPath) = 0;
    virtual void SetSkyLowSpecMaterialPath(const string& skyMaterialPath) = 0;
    
    // Description:
    //    Loads the sky material for the level.  e_SkyType will determine if we load the low spec sky material or the regular sky material.
    virtual void LoadSkyMaterial() = 0;

    // Description:
    //    Returns SkyBox material.
    virtual _smart_ptr<IMaterial> GetSkyMaterial() = 0;

    // Description:
    //    Sets SkyBox Material.
    virtual void SetSkyMaterial(_smart_ptr<IMaterial> pSkyMat) = 0;

    // Description:
    //    Sets global 3d engine parameter.
    virtual void SetGlobalParameter(E3DEngineParameter param, const Vec3& v) = 0;
    void SetGlobalParameter(E3DEngineParameter param, float val) { SetGlobalParameter(param, Vec3(val, 0, 0)); };

    // Description:
    //    Retrieves global 3d engine parameter.
    virtual void GetGlobalParameter(E3DEngineParameter param, Vec3& v) = 0;
    float GetGlobalParameter(E3DEngineParameter param) { Vec3 v(0, 0, 0); GetGlobalParameter(param, v); return v.x; };

    virtual void SetShadowMode(EShadowMode shadowMode) = 0;
    virtual EShadowMode GetShadowMode() const = 0;
    virtual void AddPerObjectShadow(IShadowCaster* pCaster, float fConstBias, float fSlopeBias, float fJitter, const Vec3& vBBoxScale, uint nTexSize) = 0;
    virtual void RemovePerObjectShadow(IShadowCaster* pCaster) = 0;
    virtual struct SPerObjectShadow* GetPerObjectShadow(IShadowCaster* pCaster) = 0;
    virtual void GetCustomShadowMapFrustums(struct ShadowMapFrustum*& arrFrustums, int& nFrustumCount) = 0;

    // Description:
    //        Saves pStatObj to a stream. 
    // Notes:
    //        Full mesh for generated ones, path/geom otherwise
    virtual int SaveStatObj(IStatObj* pStatObj, TSerialize ser) = 0;
    // Description:
    //        Loads statobj from a stream
    virtual IStatObj* LoadStatObj(TSerialize ser) = 0;

    // Description:
    //        Returns true if input line segment intersect clouds sprites.
    virtual bool CheckIntersectClouds(const Vec3& p1, const Vec3& p2) = 0;

    // Description:
    //        Removes references to RenderMesh
    virtual void OnRenderMeshDeleted(IRenderMesh* pRenderMesh) = 0;

    //     Used to highlight an object under the reticule
    virtual void DebugDraw_UpdateDebugNode() = 0;

    // Description:
    //     Used by editor during AO computations
    virtual bool RayObjectsIntersection2D(Vec3 vStart, Vec3 vEnd, Vec3& vHitPoint, EERType eERType) = 0;


    // Description:
    //    Used by editor during object alignment
    virtual bool RenderMeshRayIntersection(IRenderMesh* pRenderMesh, SRayHitInfo& hitInfo, _smart_ptr<IMaterial> pCustomMtl = 0) = 0;

    virtual void CheckCreateRNTmpData(CRNTmpData** ppInfo, IRenderNode* pRNode, const SRenderingPassInfo& passInfo) = 0;

    // Description:
    //     Frees lod transition state
    virtual void FreeRNTmpData(CRNTmpData** ppInfo) = 0;

    // Description:
    //     Returns true if the Octree is ready.
    virtual bool IsObjectTreeReady() = 0;

    virtual IOctreeNode* GetIObjectTree() = 0;

    // Description:
    //     Call function 2 times (first to get the size then to fill in the data)
    // Arguments:
    //   pObjects - 0 if only the count is required
    // Return Value:
    //   Count returned
    virtual uint32 GetObjectsByType(EERType objType, IRenderNode** pObjects = 0) = 0;
    virtual uint32 GetObjectsByTypeInBox(EERType objType, const AABB& bbox, IRenderNode** pObjects = 0, ObjectTreeQueryFilterCallback filterCallback = nullptr) = 0;
    virtual uint32 GetObjectsInBox(const AABB& bbox, IRenderNode** pObjects = 0) = 0;
    virtual uint32 GetObjectsByFlags(uint dwFlag, IRenderNode** pObjects = 0) = 0;

    // variant which takes a POD array which is resized in the function itself
    virtual void GetObjectsByTypeInBox(EERType objType, const AABB& bbox, PodArray<IRenderNode*>* pLstObjects, ObjectTreeQueryFilterCallback filterCallback = nullptr) = 0;

    // Called from editor whenever an object is modified by the user
    virtual void OnObjectModified(IRenderNode* pRenderNode, uint dwFlags) = 0;

    virtual void FillDebugFPSInfo(SDebugFPSInfo&) = 0;

    virtual const char* GetLevelFolder() = 0;

    virtual bool IsAreaActivationInUse() = 0;

    virtual void RenderRenderNode_ShadowPass(IShadowCaster* pRNode, const SRenderingPassInfo& passInfo, AZ::LegacyJobExecutor* pJobExecutor) = 0;

    virtual IOpticsManager* GetOpticsManager() = 0;

    // Description:
    //        Syncs and performs outstanding operations for the Asyncrhon ProcessStreaming Update
    virtual void SyncProcessStreamingUpdate() = 0;

    // Set Callback for Editor to store additional information in Minimap tool
    virtual void SetScreenshotCallback(IScreenshotCallback* pCallback) = 0;

    // Show/Hide objects by layer (useful for streaming and performance)
    virtual void ActivateObjectsLayer(uint16 nLayerId, bool bActivate, bool bPhys, bool bObjects, bool bStaticLights, const char* pLayerName, IGeneralMemoryHeap* pHeap = NULL, bool bCheckLayerActivation = true) = 0;

    // Get object layer memory usage
    virtual void GetLayerMemoryUsage(uint16 nLayerId, ICrySizer* pSizer, int* pNumBrushes, int* pNumDecals) const = 0;

    // Collect layer ID's to skip loading objects from these layers, e.g. to skip console specific layers
    virtual void SkipLayerLoading(uint16 nLayerId, bool bClearList) = 0;

    // Activate streaming of render node and all sub-components
    virtual void PrecacheRenderNode(IRenderNode* pObj, float fEntDistanceReal) = 0;

    virtual IDeferredPhysicsEventManager* GetDeferredPhysicsEventManager() = 0;

    virtual void SetStreamableListener(IStreamedObjectListener* pListener) = 0;

    // following functions are used by SRenderingPassInfo
    virtual CCamera* GetRenderingPassCamera(const CCamera& rCamera) = 0;

    struct SSvoStaticTexInfo
    {
        SSvoStaticTexInfo()
        {
            ZeroStruct(*this);
        }

        // SVO data pools
        ITexture* pTexTree;
        ITexture* pTexOpac;
        ITexture* pTexRgb0;
        ITexture* pTexRgb1;
        ITexture* pTexDynl;
        ITexture* pTexRgb2;
        ITexture* pTexRgb3;
        ITexture* pTexNorm;
        ITexture* pTexAldi;

        ITexture* pGlobalSpecCM;

        float fGlobalSpecCM_Mult;
        int nTexDimXY;
        int nTexDimZ;
        int nBrickSize;
        bool bSvoReady;
        bool bSvoFreeze;
    };

    struct SLightTI
    {
        Vec4 vPosR;
        Vec4 vDirF;
        Vec4 vCol;
        float fSortVal;
        class ITexture* pCM;
    };

    virtual void GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D) = 0;

    struct SSvoNodeInfo
    {
        AABB wsBox;
        AABB tcBox;
        int nAtlasOffset;
    };

    virtual void GetSvoBricksForUpdate(PodArray<SSvoNodeInfo>& arrNodeInfo, bool getDynamic) = 0;

#if defined(USE_GEOM_CACHES)
    // Summary:
    //     Loads a geometry cache from a CAX file.
    // See Also:
    //     IGeomCache
    // Arguments:
    //     szFileName - CAX Filename - should not be 0 or ""
    // Return Value:
    //     A pointer to an object derived from IGeomCache.
    virtual IGeomCache* LoadGeomCache(const char* szFileName) = 0;

    //     Finds a geom cache created from the given filename
    // See Also:
    //     IGeomCache
    // Arguments:
    //     szFileName - CAX Filename - should not be 0 or ""
    // Return Value:
    //     A pointer to an object derived from IGeomCache.
    virtual IGeomCache* FindGeomCacheByFilename(const char* szFileName) = 0;
#endif

    // Summary:
    //     Loads a designer object from a stream of _decoded_ binary <mesh> node. (Base64Decode)
    // Arguments:
    //     szBinaryStream - decoded stream + size
    virtual IStatObj* LoadDesignerObject(int nVersion, const char* szBinaryStream, int size) = 0;

    // Summary:
    //     Makes sure all queued culling jobs are completely finished.
    // This is useful when adding or removing Components that trigger AZ::Job creation
    // like the terrain system. It is important to complete those jobs before removing
    // such type of components.
    virtual void WaitForCullingJobsCompletion() = 0;

    // </interfuscator:shuffle>
};

#pragma pack(pop)

#include <Cry_Camera.h>

// state of 3dengine during rendering
// used to prevent global state
struct SRenderingPassInfo
{
    enum EShadowMapType
    {
        SHADOW_MAP_NONE = 0,
        SHADOW_MAP_GSM,
        SHADOW_MAP_LOCAL,
        SHADOW_MAP_CACHED,
        SHADOW_MAP_CACHED_MGPU_COPY
    };

    // enum flags to identify which objects to skip for this pass
    enum ESkipRenderingFlags
    {
        SHADOWS = BIT(0),
        ENTITIES = BIT(3),
        WATEROCEAN = BIT(5),
        DECALS = BIT(7),
        MERGED_MESHES = BIT(10),
        ROADS = BIT(13),
        WATER_VOLUMES = BIT(14),
        CLOUDS = BIT(15),
        CUBEMAP_GEN = BIT(16),
        GEOM_CACHES = BIT(17),
        DISABLE_RENDER_CHUNK_MERGE = BIT(18),
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        RENDER_SCENE_TO_TEXTURE = BIT(20),
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

        // below are precombined flags
        STATIC_OBJECTS = ENTITIES,
        DEFAULT_FLAGS = SHADOWS | ENTITIES | WATEROCEAN | DECALS | MERGED_MESHES | ROADS | WATER_VOLUMES | CLOUDS | GEOM_CACHES,
        DEFAULT_RECURSIVE_FLAGS = ENTITIES | WATEROCEAN | DECALS | MERGED_MESHES | ROADS | WATER_VOLUMES | CLOUDS | GEOM_CACHES
    };
    
    // creating function for RenderingPassInfo, the create functions will fetch all other necessary
    // information like thread id/frame id, etc
    static SRenderingPassInfo CreateGeneralPassRenderingInfo(const CCamera& rCamera, uint32 nRenderingFlags = DEFAULT_FLAGS, bool bAuxWindow = false);
    static SRenderingPassInfo CreateRecursivePassRenderingInfo(const CCamera& rCamera, uint32 nRenderingFlags = DEFAULT_RECURSIVE_FLAGS);
    static SRenderingPassInfo CreateShadowPassRenderingInfo(const CCamera& rCamera, int nLightFlags, int nShadowMapLod, bool bExtendedLod, bool bIsMGPUCopy, uint32* pShadowGenMask, uint32 nSide, uint32 nShadowFrustumID, uint32 nRenderingFlags = DEFAULT_FLAGS);
    static SRenderingPassInfo CreateTempRenderingInfo(const CCamera& rCamera, const SRenderingPassInfo& rPassInfo);
    static SRenderingPassInfo CreateTempRenderingInfo(uint32 nRenderingFlags, const SRenderingPassInfo& rPassInfo);
    
    // state getter    
    bool IsGeneralPass() const;

    bool IsRecursivePass() const;
    uint32 GetRecursiveLevel() const;

    bool IsShadowPass() const;
    bool IsCachedShadowPass() const;
    EShadowMapType GetShadowMapType() const;
    bool IsDisableRenderChunkMerge() const;
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    bool IsRenderSceneToTexturePass() const;
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    bool IsAuxWindow() const;

    threadID ThreadID() const;
    void SetThreadID(threadID id) { m_nThreadID = static_cast<uint8>(id); }

    int GetFrameID() const;
    uint32 GetMainFrameID() const;

    const CCamera& GetCamera() const;
    bool IsCameraUnderWater() const;

    float GetZoomFactor() const;
    float GetInverseZoomFactor() const;
    bool RenderShadows() const;
    bool RenderEntities() const;
    bool RenderWaterOcean() const;
    bool RenderDecals() const;
    bool RenderWaterVolumes() const;
    bool RenderClouds() const;
    bool RenderGeomCaches() const;

    bool IsRenderingCubemap() const;

    uint32* ShadowGenMaskAddress() const;
    uint32 ShadowFrustumID() const;
    uint8 ShadowFrustumSide() const;
    uint8 ShadowFrustumLod() const;

    CRenderView* GetRenderView() const;

    SRenderingPassInfo(threadID id)
    {
        SRenderingPassInfo();
        SetThreadID(id);
        m_pRenderView = gEnv->pRenderer->GetRenderViewForThread(id);
    }

private:

    // private constructor, creation is only allowed with create functions
    SRenderingPassInfo()
        : pShadowGenMask(NULL)
        , nShadowSide(0)
        , nShadowLod(0)
        , nShadowFrustumId(0)
        , m_bAuxWindow(0)
        , m_nRenderStackLevel(0)
        , m_eShadowMapRendering(static_cast<uint8>(SHADOW_MAP_NONE))
        , m_bCameraUnderWater(0)
        , m_nRenderingFlags(0)
        , m_fZoomFactor(0.0f)
        , m_pCamera(NULL)
    {
        threadID nThreadID = 0;
        gEnv->pRenderer->EF_Query(EFQ_MainThreadList, nThreadID);
        m_nThreadID =  aznumeric_caster(nThreadID);
        m_nRenderFrameID = gEnv->pRenderer->GetFrameID();
        m_nRenderMainFrameID = gEnv->pRenderer->GetFrameID(false);
        m_pRenderView = gEnv->pRenderer->GetRenderViewForThread(m_nThreadID);
    }

    void InitRenderingFlags(uint32 nRenderingFlags);
    void SetCamera(const CCamera& cam);

    uint8 m_nThreadID;
    uint8 m_nRenderStackLevel;
    uint8 m_eShadowMapRendering;        // State flag denoting what type of shadow map is being currently rendered into
    uint8 m_bCameraUnderWater;

    uint32 m_nRenderingFlags;

    float m_fZoomFactor;

    int m_nRenderFrameID;
    uint32 m_nRenderMainFrameID;

    const CCamera* m_pCamera;

    // Render view used for this rendering pass
    CRenderView* m_pRenderView;

    // members used only in shadow pass
    uint32* pShadowGenMask;
    uint32 nShadowFrustumId;
    uint8 nShadowSide : 4;
    uint8 nShadowLod  : 4;
    uint8 m_bAuxWindow;
};

///////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsGeneralPass() const
{
    return m_nRenderStackLevel == 0 && m_bAuxWindow == 0 && static_cast<EShadowMapType>(m_eShadowMapRendering) == SHADOW_MAP_NONE;
}

///////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsRecursivePass() const
{ 
    return m_nRenderStackLevel > 0; 
}

///////////////////////////////////////////////////////////////////////////////
inline uint32 SRenderingPassInfo::GetRecursiveLevel() const 
{ 
    return m_nRenderStackLevel; 
}

///////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsShadowPass() const 
{ 
    return static_cast<EShadowMapType>(m_eShadowMapRendering) != SHADOW_MAP_NONE; 
}

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
///////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsRenderSceneToTexturePass() const
{
    return (m_nRenderingFlags & RENDER_SCENE_TO_TEXTURE) != 0;
}
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

///////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsCachedShadowPass() const
{
    return IsShadowPass() && 
        (GetShadowMapType() == SRenderingPassInfo::SHADOW_MAP_CACHED || 
         GetShadowMapType() == SRenderingPassInfo::SHADOW_MAP_CACHED_MGPU_COPY);
}
///////////////////////////////////////////////////////////////////////////////
inline SRenderingPassInfo::EShadowMapType SRenderingPassInfo::GetShadowMapType() const 
{
    assert(IsShadowPass());
    return static_cast<EShadowMapType>(m_eShadowMapRendering); 
}

///////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsAuxWindow() const 
{ 
    return m_bAuxWindow != 0;
}

///////////////////////////////////////////////////////////////////////////////
inline threadID SRenderingPassInfo::ThreadID() const 
{
    return m_nThreadID;
}

///////////////////////////////////////////////////////////////////////////////
inline int SRenderingPassInfo::GetFrameID() const
{
    return m_nRenderFrameID;
}

///////////////////////////////////////////////////////////////////////////////
inline uint32 SRenderingPassInfo::GetMainFrameID() const
{
    return m_nRenderMainFrameID;
}

///////////////////////////////////////////////////////////////////////////////
inline const CCamera& SRenderingPassInfo::GetCamera() const 
{ 
    assert(m_pCamera != NULL);
    return *m_pCamera; 
}
    
///////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsCameraUnderWater() const
{
    return m_bCameraUnderWater != 0;
}

///////////////////////////////////////////////////////////////////////////////
inline float SRenderingPassInfo::GetZoomFactor() const
{
    return m_fZoomFactor;
}

///////////////////////////////////////////////////////////////////////////////
inline float SRenderingPassInfo::GetInverseZoomFactor() const
{
    return 1.0f / m_fZoomFactor;
}


////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::RenderShadows() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::SHADOWS) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::RenderEntities() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::ENTITIES) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::RenderWaterOcean() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::WATEROCEAN) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::RenderDecals() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::DECALS) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::RenderWaterVolumes() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::WATER_VOLUMES) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::RenderClouds() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::CLOUDS) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::RenderGeomCaches() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::GEOM_CACHES) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsRenderingCubemap() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::CUBEMAP_GEN) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline bool SRenderingPassInfo::IsDisableRenderChunkMerge() const
{
    return (m_nRenderingFlags& SRenderingPassInfo::DISABLE_RENDER_CHUNK_MERGE) != 0;
}

////////////////////////////////////////////////////////////////////////////////
inline uint32* SRenderingPassInfo::ShadowGenMaskAddress() const
{
    assert(pShadowGenMask);
    return pShadowGenMask;
}

////////////////////////////////////////////////////////////////////////////////
inline uint32 SRenderingPassInfo::ShadowFrustumID() const
{
    return nShadowFrustumId;
}

////////////////////////////////////////////////////////////////////////////////
inline uint8 SRenderingPassInfo::ShadowFrustumSide() const
{
    return nShadowSide;
}

////////////////////////////////////////////////////////////////////////////////
inline uint8 SRenderingPassInfo::ShadowFrustumLod() const
{
    return nShadowLod;
}
////////////////////////////////////////////////////////////////////////////////
inline CRenderView* SRenderingPassInfo::GetRenderView() const
{
    return m_pRenderView;
}
////////////////////////////////////////////////////////////////////////////////
inline void SRenderingPassInfo::SetCamera(const CCamera& cam)
{
    m_pCamera = gEnv->p3DEngine->GetRenderingPassCamera(cam);
    m_bCameraUnderWater = gEnv->p3DEngine->IsUnderWater(cam.GetPosition());
    m_fZoomFactor = 0.2f + 0.8f * (RAD2DEG(cam.GetFov()) / 60.f);
}

////////////////////////////////////////////////////////////////////////////////
inline void SRenderingPassInfo::InitRenderingFlags(uint32 nRenderingFlags)
{
    m_nRenderingFlags = nRenderingFlags;
#if (ALLOW_CONST_CVAR_MODIFICATIONS)
    static ICVar* pDefaultMaterial = gEnv->pConsole->GetCVar("e_DefaultMaterial");
    static ICVar* pShadows = gEnv->pConsole->GetCVar("e_Shadows");
    static ICVar* pEntities = gEnv->pConsole->GetCVar("e_Entities");
    static ICVar* pWaterOcean = gEnv->pConsole->GetCVar("e_WaterOcean");
    static ICVar* pDecals = gEnv->pConsole->GetCVar("e_Decals");
    static ICVar* pWaterVolumes = gEnv->pConsole->GetCVar("e_WaterVolumes");
    static ICVar* pClouds = gEnv->pConsole->GetCVar("e_Clouds");
    static ICVar* pGeomCaches = gEnv->pConsole->GetCVar("e_GeomCaches");

    if (pShadows->GetIVal() == 0)
    {
        m_nRenderingFlags &= ~SRenderingPassInfo::SHADOWS;
    }
    if (pEntities->GetIVal() == 0)
    {
        m_nRenderingFlags &= ~SRenderingPassInfo::ENTITIES;
    }
    if (pWaterOcean->GetIVal() == 0)
    {
        m_nRenderingFlags &= ~SRenderingPassInfo::WATEROCEAN;
    }
    if (pDecals->GetIVal() == 0)
    {
        m_nRenderingFlags &= ~SRenderingPassInfo::DECALS;
    }
    if (pWaterVolumes->GetIVal() == 0)
    {
        m_nRenderingFlags &= ~SRenderingPassInfo::WATER_VOLUMES;
    }
    if (pClouds->GetIVal() == 0)
    {
        m_nRenderingFlags &= ~SRenderingPassInfo::CLOUDS;
    }
    if (pGeomCaches->GetIVal() == 0)
    {
        m_nRenderingFlags &= ~SRenderingPassInfo::GEOM_CACHES;
    }

    // on dedicated server, never render any object at all
    if (gEnv->IsDedicated())
    {
        m_nRenderingFlags = 0;
    }
#endif // (ALLOW_CONST_CVAR_MODIFICATIONS)
}

////////////////////////////////////////////////////////////////////////////////
inline SRenderingPassInfo SRenderingPassInfo::CreateGeneralPassRenderingInfo(const CCamera& rCamera, uint32 nRenderingFlags, bool bAuxWindow)
{
    static ICVar* pCameraFreeze = gEnv->pConsole->GetCVar("e_CameraFreeze");

    // Update Camera only if e_camerafreeze is not set
    const CCamera& rCameraToSet = (pCameraFreeze && pCameraFreeze->GetIVal() != 0) ? gEnv->p3DEngine->GetRenderingCamera() : rCamera;

    SRenderingPassInfo passInfo;
    passInfo.SetCamera(rCameraToSet);
    passInfo.InitRenderingFlags(nRenderingFlags);
    passInfo.m_bAuxWindow = bAuxWindow;

    return passInfo;
}

///////////////////////////////////////////////////////////////////////////////
inline SRenderingPassInfo SRenderingPassInfo::CreateRecursivePassRenderingInfo(const CCamera& rCamera, uint32 nRenderingFlags)
{
    static ICVar* pRecursionViewDistRatio = gEnv->pConsole->GetCVar("e_RecursionViewDistRatio");
    
    SRenderingPassInfo passInfo;
    passInfo.m_nRenderStackLevel = 1;
    passInfo.SetCamera(rCamera);

    // adjust view distance in recursive mode by adjusting the ZoomFactor
    passInfo.m_fZoomFactor /= pRecursionViewDistRatio->GetFVal();

    passInfo.InitRenderingFlags(nRenderingFlags);

    return passInfo;
}

///////////////////////////////////////////////////////////////////////////////
inline SRenderingPassInfo SRenderingPassInfo::CreateShadowPassRenderingInfo(const CCamera& rCamera, int nLightFlags, int nShadowMapLod, bool bExtendedLod, bool bIsMGPUCopy, uint32* pShadowGenMask, uint32 nSide, uint32 nShadowFrustumID, uint32 nRenderingFlags)
{
    SRenderingPassInfo passInfo;
    passInfo.SetCamera(rCamera);
    passInfo.InitRenderingFlags(nRenderingFlags);

    // set correct shadow map type
    if (nLightFlags & DLF_SUN)
    {
        assert(nShadowMapLod >= 0 && nShadowMapLod < 8);
        if (bExtendedLod)
        {
            passInfo.m_eShadowMapRendering = bIsMGPUCopy ? static_cast<uint8>(SHADOW_MAP_CACHED_MGPU_COPY) : static_cast<uint8>(SHADOW_MAP_CACHED);
        }
        else
        {
            passInfo.m_eShadowMapRendering = static_cast<uint8>(SHADOW_MAP_GSM);
        }
    }
    else if (nLightFlags & (DLF_POINT | DLF_PROJECT | DLF_AREA_LIGHT))
    {
        passInfo.m_eShadowMapRendering = static_cast<uint8>(SHADOW_MAP_LOCAL);
    }
    else
    {
        passInfo.m_eShadowMapRendering = static_cast<uint8>(SHADOW_MAP_NONE);
    }

    passInfo.pShadowGenMask = pShadowGenMask;
    passInfo.nShadowSide = nSide;
    passInfo.nShadowLod = nShadowMapLod;
    passInfo.nShadowFrustumId = nShadowFrustumID;

    return passInfo;
}

///////////////////////////////////////////////////////////////////////////////
inline SRenderingPassInfo SRenderingPassInfo::CreateTempRenderingInfo(const CCamera& rCamera, const SRenderingPassInfo& rPassInfo)
{
    SRenderingPassInfo passInfo = rPassInfo;
    passInfo.SetCamera(rCamera);

    passInfo.pShadowGenMask = NULL;
    passInfo.nShadowSide = 0;
    passInfo.nShadowFrustumId = 0;

    return passInfo;
}

///////////////////////////////////////////////////////////////////////////////
inline SRenderingPassInfo SRenderingPassInfo::CreateTempRenderingInfo(uint32 nRenderingFlags, const SRenderingPassInfo& rPassInfo)
{
    SRenderingPassInfo passInfo = rPassInfo;
    passInfo.m_nRenderingFlags = nRenderingFlags;
    return passInfo;
}

///////////////////////////////////////////////////////////////////////////////
// class to wrap a special counter used to presort SRendItems
// this is used to fix random ordering introduced by parallelization
// of parts of the 3DEngine
struct SRendItemSorter
{
    // Deferred PreProcess needs a special ordering, use these to prefix the values
    // to ensure the deferred shading pass is after all LPV objects
    enum EDeferredPreprocess
    {        
        eLPVPass = 0,
        eDeferredShadingPass = BIT(30)
    };

    static SRendItemSorter CreateRendItemSorter(const SRenderingPassInfo& passInfo);
    static SRendItemSorter CreateShadowPassRendItemSorter(const SRenderingPassInfo& passInfo);
    static SRendItemSorter CreateParticleRendItemSorter(const SRenderingPassInfo& passInfo);
    static SRendItemSorter CreateDeferredPreProcessRendItemSorter(const SRenderingPassInfo& passInfo, EDeferredPreprocess deferredPrerocessType);
    static SRendItemSorter CreateDefaultRendItemSorter();

    void IncreaseOctreeCounter()    { nValue += eOctreeNodeCounter; }
    void IncreaseObjectCounter()    { nValue += eObjectCounter; }
    void IncreaseGroupCounter()        { nValue += eGroupCounter; }
    
    void IncreaseParticleCounter()    { nValue += eParticleCounter; }    
    uint32 ParticleCounter() const    { return nValue & ~eRecursivePassMask; }

    uint32 ShadowFrustumID() const    { return nValue  & ~eRecursivePassMask; }

    uint32 GetValue() const { return nValue; }

    bool operator<(const SRendItemSorter& rOther) const
    {
        return nValue < rOther.nValue;
    }

    bool IsRecursivePass() const { return (nValue & eRecursivePassMask) != 0; }

    SRendItemSorter()
        : nValue(0) {}
    explicit SRendItemSorter(uint32 _nValue)
        : nValue(_nValue) {}

private:
    // encode various counter in a single value
    enum
    {
        eRecursivePassMask = BIT(31)
    };                                     // present in all combinations

    // flags used for regular SRendItems
    enum
    {
        eObjectCounter = BIT(0)
    };                                                  // bits 0-14 used
    enum
    {
        eOctreeNodeCounter = BIT(14)
    };                                          // bits 15-27 used
    enum
    {
        eGroupCounter = BIT(27)
    };                                                  // bits 28-31 used

    // flags used for Particles
    enum
    {
        eParticleCounter = BIT(0)
    };

    uint32 nValue;
};

///////////////////////////////////////////////////////////////////////////////
inline SRendItemSorter SRendItemSorter::CreateRendItemSorter(const SRenderingPassInfo& passInfo)
{
    SRendItemSorter rendItemSorter;
    rendItemSorter.nValue = 0;
    rendItemSorter.nValue |= passInfo.IsRecursivePass() ? eRecursivePassMask : 0;
    return rendItemSorter;
}

///////////////////////////////////////////////////////////////////////////////
inline SRendItemSorter SRendItemSorter::CreateShadowPassRendItemSorter(const SRenderingPassInfo& passInfo)
{
    SRendItemSorter rendItemSorter;    
    rendItemSorter.nValue = passInfo.ShadowFrustumID();
    rendItemSorter.nValue |= passInfo.IsRecursivePass() ? eRecursivePassMask : 0;
    return rendItemSorter;
}

///////////////////////////////////////////////////////////////////////////////
inline SRendItemSorter SRendItemSorter::CreateParticleRendItemSorter(const SRenderingPassInfo& passInfo)
{
    SRendItemSorter rendItemSorter;
    rendItemSorter.nValue = 0;
    rendItemSorter.nValue |= passInfo.IsRecursivePass() ? eRecursivePassMask : 0;    
    return rendItemSorter;
}

///////////////////////////////////////////////////////////////////////////////
inline SRendItemSorter SRendItemSorter::CreateDeferredPreProcessRendItemSorter(const SRenderingPassInfo& passInfo, EDeferredPreprocess deferredPrerocessType)
{
    SRendItemSorter rendItemSorter;
    rendItemSorter.nValue = 0;
    rendItemSorter.nValue |= passInfo.IsRecursivePass() ? eRecursivePassMask : 0;    
    rendItemSorter.nValue |= deferredPrerocessType;
    return rendItemSorter;
}

///////////////////////////////////////////////////////////////////////////////
inline SRendItemSorter SRendItemSorter::CreateDefaultRendItemSorter()
{
    SRendItemSorter rendItemSorter;
    rendItemSorter.nValue = 0;
    return rendItemSorter;
}

//Legacy Bus for communicating with SVOGI from the legacy engine code.
class SVOGILegacyRequests
    : public AZ::EBusTraits
{
public:
    //////////////////////////////////////////////////////////////////////////
    // EBusTraits overrides
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    using MutexType = AZStd::recursive_mutex;
//    static const bool LocklessDispatch = true;
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Triggers an update of voxel data. 
    virtual void UpdateVoxelData() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Triggers an update of voxel data to GPU.
    virtual void UpdateRenderData() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Called at framestart
    virtual void OnFrameStart(const SRenderingPassInfo& passInfo) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Gets the textures bound for GI plus lighting data.
    virtual void GetSvoStaticTextures(I3DEngine::SSvoStaticTexInfo& svoInfo, PodArray<I3DEngine::SLightTI>* pLightsTI_S, PodArray<I3DEngine::SLightTI>* pLightsTI_D) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Generates a list of bricks that need to be updated in compute shaders.
    virtual void GetSvoBricksForUpdate(PodArray<I3DEngine::SSvoNodeInfo>& arrNodeInfo, bool getDynamic) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Causes the GI system to free all voxel data. 
    virtual void ReleaseData() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Register and unregister a mutex to protect assets during rendering.
    virtual void RegisterMutex(AZStd::mutex* mutex) = 0;
    virtual void UnregisterMutex() = 0;
    
};
using SVOGILegacyRequestBus = AZ::EBus<SVOGILegacyRequests>;


#endif // CRYINCLUDE_CRYCOMMON_I3DENGINE_H

