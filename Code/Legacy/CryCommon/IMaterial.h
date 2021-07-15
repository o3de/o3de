/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : IMaterial interface declaration.


#ifndef CRYINCLUDE_CRYCOMMON_IMATERIAL_H
#define CRYINCLUDE_CRYCOMMON_IMATERIAL_H
#pragma once

struct ISurfaceType;
struct ISurfaceTypeManager;
class ICrySizer;

enum EEfResTextures : int; // Need to specify a fixed size for the forward declare to work on clang
struct IRenderShaderResources;
struct SEfTexModificator;
struct SInputShaderResources;

struct SShaderItem;
struct SShaderParam;
struct IShader;
struct IShaderPublicParams;
struct IMaterial;
struct IMaterialManager;
struct CMaterialCGF;
struct CRenderChunk;
struct IRenderMesh;

#include <Tarray.h>
#include <IXml.h>
#include <smartptr.h>
#include <AzCore/EBus/EBus.h>

#ifdef MAX_SUB_MATERIALS
// This checks that the values are in sync in the different files.
COMPILE_TIME_ASSERT(MAX_SUB_MATERIALS == 128);
#else
#define MAX_SUB_MATERIALS 128
#endif

// Special names for materials.
#define MTL_SPECIAL_NAME_COLLISION_PROXY "collision_proxy"
#define MTL_SPECIAL_NAME_COLLISION_PROXY_VEHICLE "nomaterial_vehicle"
#define MTL_SPECIAL_NAME_RAYCAST_PROXY "raycast_proxy"

namespace AZ
{
    class MaterialNotificationEvents : public AZ::EBusTraits
    {
    public:
        virtual ~MaterialNotificationEvents() {}

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const bool EnableEventQueue = true;
        using EventQueueMutexType = AZStd::mutex;


        virtual void OnShaderLoaded([[maybe_unused]] IShader* shader) {}
    };
    using MaterialNotificationEventBus = AZ::EBus<MaterialNotificationEvents>;
}

enum
{
    MAX_STREAM_PREDICTION_ZONES = 2
};

//////////////////////////////////////////////////////////////////////////
// Description:
//    IMaterial is an interface to the material object, SShaderItem host which is a combination of IShader and SShaderInputResources.
//    Material bind together rendering algorithm (Shader) and resources needed to render this shader, textures, colors, etc...
//    All materials except for pure sub material childs have a unique name which directly represent .mtl file on disk.
//    Ex: "Materials/Fire/Burn"
//    Materials can be created by Sandbox MaterialEditor.
//////////////////////////////////////////////////////////////////////////
enum EMaterialFlags
{
    MTL_FLAG_WIRE                       = 0x0001,   // Use wire frame rendering for this material.
    MTL_FLAG_2SIDED                     = 0x0002,   // Use 2 Sided rendering for this material.
    MTL_FLAG_ADDITIVE                   = 0x0004,   // Use Additive blending for this material.
    //MTL_FLAG_DETAIL_DECAL               = 0x0008,   // UNUSED RESERVED FOR LEGACY REASONS
    MTL_FLAG_LIGHTING                   = 0x0010,   // Should lighting be applied on this material.
    MTL_FLAG_NOSHADOW                   = 0x0020,   // Material do not cast shadows.
    MTL_FLAG_ALWAYS_USED                = 0x0040,   // When set forces material to be export even if not explicitly used.
    MTL_FLAG_PURE_CHILD                 = 0x0080,   // Not shared sub material, sub material unique to his parent multi material.
    MTL_FLAG_MULTI_SUBMTL               = 0x0100,   // This material is a multi sub material.
    MTL_FLAG_NOPHYSICALIZE              = 0x0200,   // Should not physicalize this material.
    MTL_FLAG_NODRAW                     = 0x0400,   // Do not render this material.
    MTL_FLAG_NOPREVIEW                  = 0x0800,   // Cannot preview the material.
    MTL_FLAG_NOTINSTANCED               = 0x1000,   // Do not instantiate this material.
    MTL_FLAG_COLLISION_PROXY            = 0x2000,   // This material is the collision proxy.
    MTL_FLAG_SCATTER                    = 0x4000,   // Use scattering for this material
    MTL_FLAG_REQUIRE_FORWARD_RENDERING  = 0x8000,   // This material has to be rendered in forward rendering passes (alpha/additive blended)
    MTL_FLAG_NON_REMOVABLE              = 0x10000,  // Material with this flag once created are never removed from material manager (Used for decal materials, this flag should not be saved).
    MTL_FLAG_HIDEONBREAK                = 0x20000,  // Non-physicalized subsets with such materials will be removed after the object breaks
    MTL_FLAG_UIMATERIAL                 = 0x40000,  // Used for UI in Editor. Don't need show it DB.
    MTL_64BIT_SHADERGENMASK             = 0x80000,  // ShaderGen mask is remapped
    MTL_FLAG_RAYCAST_PROXY              = 0x100000,
    MTL_FLAG_REQUIRE_NEAREST_CUBEMAP    = 0x200000, // materials with alpha blending requires special processing for shadows
    MTL_FLAG_CONSOLE_MAT                = 0x400000,
    MTL_FLAG_DELETE_PENDING             = 0x800000, // Internal use only
    MTL_FLAG_BLEND_TERRAIN              = 0x1000000,
    MTL_FLAG_IS_TERRAIN                 = 0x2000000,// indication to the loader - Terrain type
    MTL_FLAG_IS_SKY                     = 0x4000000,// indication to the loader - Sky type
    MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH= 0x8000000 // high vertex shading quality behaves more accurately with fog volumes.
};

#define MTL_FLAGS_SAVE_MASK (MTL_FLAG_WIRE | MTL_FLAG_2SIDED | MTL_FLAG_ADDITIVE |  MTL_FLAG_LIGHTING | \
                             MTL_FLAG_NOSHADOW | MTL_FLAG_MULTI_SUBMTL | MTL_FLAG_SCATTER | MTL_FLAG_REQUIRE_FORWARD_RENDERING | MTL_FLAG_FOG_VOLUME_SHADING_QUALITY_HIGH | MTL_FLAG_HIDEONBREAK | MTL_FLAG_UIMATERIAL | MTL_64BIT_SHADERGENMASK | MTL_FLAG_REQUIRE_NEAREST_CUBEMAP | MTL_FLAG_CONSOLE_MAT | MTL_FLAG_BLEND_TERRAIN)

// Post effects flags
enum EPostEffectFlags
{
    POST_EFFECT_GHOST     = 0x1,
    POST_EFFECT_HOLOGRAM  = 0x2,

    POST_EFFECT_MASK = POST_EFFECT_GHOST | POST_EFFECT_HOLOGRAM
};

// Bit offsets for shader layer flags
enum EMaterialLayerFlags
{
    // Active layers flags
    MTL_LAYER_FROZEN = 0x0001,
    MTL_LAYER_WET    = 0x0002,
    MTL_LAYER_DYNAMICFROZEN = 0x0008,

    // Usage flags
    MTL_LAYER_USAGE_NODRAW = 0x0001,     // Layer is disabled
    MTL_LAYER_USAGE_REPLACEBASE = 0x0002, // Replace base pass rendering with layer - optimization
    MTL_LAYER_USAGE_FADEOUT = 0x0004,            // Layer doesn't render but still causes parent to fade out

    // Blend offsets
    MTL_LAYER_BLEND_FROZEN = 0xff000000,
    MTL_LAYER_BLEND_WET    = 0x00fe0000,
    MTL_LAYER_BLEND_DYNAMICFROZEN = 0x000000ff,

    MTL_LAYER_FROZEN_MASK                   = 0xff,
    MTL_LAYER_WET_MASK                      = 0xfe, // bit stolen
    MTL_LAYER_DYNAMICFROZEN_MASK    = 0xff,

    MTL_LAYER_BLEND_MASK = (MTL_LAYER_BLEND_FROZEN | MTL_LAYER_BLEND_WET | MTL_LAYER_BLEND_DYNAMICFROZEN),

    // Slot count
    MTL_LAYER_MAX_SLOTS = 3
};

// copy flags
enum EMaterialCopyFlags
{
    // copy flags
    MTL_COPY_DEFAULT  = 0,
    MTL_COPY_NAME     = BIT(0),
    MTL_COPY_TEXTURES = BIT(1),
};

struct IMaterialHelpers
{
    virtual ~IMaterialHelpers() {}

    //////////////////////////////////////////////////////////////////////////
    virtual EEfResTextures FindTexSlot(const char* texName) const = 0;
    virtual const char* FindTexName(EEfResTextures texSlot) const = 0;
    virtual const char* LookupTexName(EEfResTextures texSlot) const = 0;
    virtual const char* LookupTexDesc(EEfResTextures texSlot) const = 0;
    virtual const char* LookupTexEnum(EEfResTextures texSlot) const = 0;
    virtual const char* LookupTexSuffix(EEfResTextures texSlot) const = 0;
    virtual bool IsAdjustableTexSlot(EEfResTextures texSlot) const = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual bool SetGetMaterialParamFloat(IRenderShaderResources& pShaderResources, const char* sParamName, float& v, bool bGet) const = 0;
    virtual bool SetGetMaterialParamVec3(IRenderShaderResources& pShaderResources, const char* sParamName, Vec3& v, bool bGet) const = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetTexModFromXml(SEfTexModificator& pShaderResources, const XmlNodeRef& node) const = 0;
    virtual void SetXmlFromTexMod(const SEfTexModificator& pShaderResources, XmlNodeRef& node) const = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetTexturesFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const = 0;
    virtual void SetXmlFromTextures( SInputShaderResources& pShaderResources, XmlNodeRef& node) const = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetVertexDeformFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const = 0;
    virtual void SetXmlFromVertexDeform(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetLightingFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const = 0;
    virtual void SetXmlFromLighting(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void SetShaderParamsFromXml(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const = 0;
    virtual void SetXmlFromShaderParams(const SInputShaderResources& pShaderResources, XmlNodeRef& node) const = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual void MigrateXmlLegacyData(SInputShaderResources& pShaderResources, const XmlNodeRef& node) const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////
// Description:
//    IMaterialLayer is group of material layer properties.
//    Each layer is composed of shader item, specific layer textures, lod info, etc
struct IMaterialLayer
{
    // <interfuscator:shuffle>
    virtual ~IMaterialLayer(){}
    // Reference counting
    virtual void AddRef() = 0;
    virtual void Release() = 0;

    // Description:
    //    - Enable/disable layer usage
    virtual void Enable(bool bEnable = true) = 0;
    // Description:
    //    - Check if layer enabled
    virtual bool IsEnabled() const = 0;
    // Description:
    //    - Enable/disable fade out
    virtual void FadeOut(bool bFadeOut = true) = 0;
    // Description:
    //    - Check if layer fades out
    virtual bool DoesFadeOut() const = 0;
    // Description:
    //    - Set shader item
    virtual void SetShaderItem(const _smart_ptr<IMaterial> pParentMtl, const SShaderItem& pShaderItem) = 0;
    // Description:
    //    - Return shader item
    virtual const SShaderItem& GetShaderItem() const = 0;
    virtual SShaderItem& GetShaderItem() = 0;
    // Description:
    //    - Set layer usage flags
    virtual void SetFlags(uint8 nFlags) = 0;
    // Description:
    //    - Get layer usage flags
    virtual uint8 GetFlags() const = 0;

    // todo: layer specific textures support
    //
    // </interfuscator:shuffle>
};

struct IMaterial
{
    // TODO: Remove it!
    //! default texture mapping
    uint8 m_ucDefautMappingAxis;
    float m_fDefautMappingScale;

    // <interfuscator:shuffle>
    virtual ~IMaterial() {};

    //////////////////////////////////////////////////////////////////////////
    // Reference counting.
    //////////////////////////////////////////////////////////////////////////
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual int GetNumRefs() = 0;

    virtual IMaterialHelpers& GetMaterialHelpers() = 0;
    virtual IMaterialManager* GetMaterialManager() = 0;

    //////////////////////////////////////////////////////////////////////////
    // material name
    //////////////////////////////////////////////////////////////////////////
    //! Set material name, (Do not use this directly
    virtual void SetName(const char* pName) = 0;
    //! Returns material name.
    virtual const char* GetName() const = 0;

    //! Set/get shader name. The shader name may include technique name so it could be deferent than GetShaderItem()->m_pShader->GetName().
    virtual void SetShaderName(const char* pName) = 0;
    virtual const char* GetShaderName() const = 0;

    //! Material flags.
    //! @see EMaterialFlags
    virtual void SetFlags(int flags) = 0;
    virtual int GetFlags() const = 0;
    virtual void UpdateFlags() = 0;

    // Returns true if this is the default material.
    virtual bool IsDefault() = 0;

    virtual int GetSurfaceTypeId() = 0;

    // Assign a different surface type to this material.
    virtual void SetSurfaceType(const char* sSurfaceTypeName) = 0;

    virtual ISurfaceType* GetSurfaceType() = 0;

    // shader item
    virtual void ReleaseCurrentShaderItem() = 0;
    virtual void SetShaderItem(const SShaderItem& _ShaderItem) = 0;
    // [Alexey] EF_LoadShaderItem return value with RefCount = 1, so if you'll use SetShaderItem after EF_LoadShaderItem use Assign function
    virtual void AssignShaderItem(const SShaderItem& _ShaderItem) = 0;
    virtual SShaderItem& GetShaderItem() = 0;
    virtual const SShaderItem& GetShaderItem() const = 0;

    // Returns shader item for correct sub material or for single material.
    // Even if this is not sub material or nSubMtlSlot is invalid it will return valid renderable shader item.
    virtual SShaderItem& GetShaderItem(int nSubMtlSlot) = 0;
    virtual const SShaderItem& GetShaderItem(int nSubMtlSlot) const = 0;

    // Returns true if streamed in
    virtual bool IsStreamedIn(const int nMinPrecacheRoundIds[MAX_STREAM_PREDICTION_ZONES], IRenderMesh* pRenderMesh) const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Sub materials access.
    //////////////////////////////////////////////////////////////////////////
    //! Returns number of child sub materials holded by this material.
    virtual void SetSubMtlCount(int numSubMtl) = 0;
    //! Returns number of child sub materials holded by this material.
    virtual int GetSubMtlCount() = 0;
    //! Return sub material at specified index.
    virtual _smart_ptr<IMaterial> GetSubMtl(int nSlot) = 0;
    // Assign material to the sub mtl slot.
    // Must first allocate slots using SetSubMtlCount.
    virtual void SetSubMtl(int nSlot, _smart_ptr<IMaterial> pMtl) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Layers access.
    //////////////////////////////////////////////////////////////////////////
    //! Returns number of layers in this material.
    virtual void SetLayerCount(uint32 nCount) = 0;
    //! Returns number of layers in this material.
    virtual uint32 GetLayerCount() const = 0;
    //! Set layer at slot id (### MUST ALOCATE SLOTS FIRST ### USING SetLayerCount)
    virtual void SetLayer(uint32 nSlot, IMaterialLayer* pLayer) = 0;
    //! Return active layer
    virtual const IMaterialLayer* GetLayer(uint8 nLayersMask, uint8 nLayersUsageMask) const = 0;
    //! Return layer at slot id
    virtual const IMaterialLayer* GetLayer(uint32 nSlot) const = 0;
    //! Create a new layer
    virtual IMaterialLayer* CreateLayer() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Always get a valid material.
    // If not multi material return this material.
    // If Multi material return Default material if wrong id.
    virtual _smart_ptr<IMaterial> GetSafeSubMtl(int nSlot) = 0;

    // Description:
    //    Fill an array of integeres representing surface ids of the sub materials or the material itself.
    // Arguments:
    //    pSurfaceIdsTable is a pointer to the array of int with size enough to hold MAX_SUB_MATERIALS surface type ids.
    // Return:
    //    number of filled items.
    virtual int FillSurfaceTypeIds(int pSurfaceIdsTable[]) = 0;

    //////////////////////////////////////////////////////////////////////////
    // UserData used to link with the Editor.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetUserData(void* pUserData) = 0;
    virtual void* GetUserData() const = 0;

    //////////////////////////////////////////////////////////////////////////
    //! Set or get a material parameter value.
    //! \param sParamName        - Name of the parameter
    //! \param v                 - Input or output value depending on bGet
    //! \param bGet              - If true, v is an output; if false, v is an input
    //! \param allowShaderParam  - If true, and sParamName is not a built-in parameter of the Material, then custom shader parameters will be searched as well. Defaults to false to preserve legacy behavior.
    //! \param materialIndex     - Index of the sub-material if this is a material group
    //! return                   - True if sParamName was found
    virtual bool SetGetMaterialParamFloat(const char* sParamName, float& v, bool bGet, bool allowShaderParam = false, int materialIndex = 0) = 0;
    //! Set or get a material parameter value.
    //! \param sParamName        - Name of the parameter
    //! \param v                 - Input or output value depending on bGet
    //! \param bGet              - If true, v is an output; if false, v is an input
    //! \param allowShaderParam  - If true, and sParamName is not a built-in parameter of the Material, then custom shader parameters will be searched as well. Defaults to false to preserve legacy behavior.
    //! \param materialIndex     - Index of the sub-material if this is a material group
    //! return                   - True if sParamName was found
    virtual bool SetGetMaterialParamVec3(const char* sParamName, Vec3& v, bool bGet, bool allowShaderParam = false, int materialIndex = 0) = 0;
    //! Set or get a material parameter value.
    //! \param sParamName        - Name of the parameter
    //! \param v                 - Input or output value depending on bGet
    //! \param bGet              - If true, v is an output; if false, v is an input
    //! \param allowShaderParam  - If true, and sParamName is not a built-in parameter of the Material, then custom shader parameters will be searched as well. Defaults to false to preserve legacy behavior.
    //! \param materialIndex     - Index of the sub-material if this is a material group
    //! return                   - True if sParamName was found
    virtual bool SetGetMaterialParamVec4(const char* sParamName, Vec4& v, bool bGet, bool allowShaderParam = false, int materialIndex = 0) = 0;

    virtual void SetDirty(bool dirty = true) = 0;
    virtual bool IsDirty() const = 0;

    //! Returns true if the material is the parent of a group of materials
    virtual bool IsMaterialGroup() const = 0;

    //! Returns true if the material is a single material belongs to a material group
    virtual bool IsSubMaterial() const = 0;

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    virtual size_t GetResourceMemoryUsage(ICrySizer* pSizer) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Makes this specific material enter sketch mode.
    // Current supported sketch modes:
    //  - 0, no sketch.
    //  - 1, normal sketch mode.
    //  - 2, fast sketch mode.
    virtual void SetSketchMode(int mode) = 0;

    // Sets FT_DONT_STREAM flag for all textures used by the material
    // If a stream is already in process, this will stop the stream and flush the device texture
    virtual void DisableTextureStreaming() = 0;
    //////////////////////////////////////////////////////////////////////////
    // Tells to texture streamer to start loading textures asynchronously
    //////////////////////////////////////////////////////////////////////////
    virtual void RequestTexturesLoading(const float fMipFactor) = 0;

    virtual void PrecacheMaterial(const float fEntDistance, struct IRenderMesh* pRenderMesh, bool bFullUpdate, bool bDrawNear = false) = 0;

    // Estimates texture memory usage for this material
    // When nMatID is not negative only caluclate for one sub-material
    virtual int GetTextureMemoryUsage(ICrySizer* pSizer, int nMatID = -1) = 0;

    // Set & retrieve a material link name
    // This value by itself is not used by the material system per-se and hence
    // has no real effect, however it is used on a higher level to tie related materials
    // together, for example by procedural breakable glass to determine which material to
    // switch to.
    virtual void SetMaterialLinkName(const char* name) = 0;
    virtual const char* GetMaterialLinkName() const = 0;
    virtual void SetKeepLowResSysCopyForDiffTex() = 0;

    virtual uint32 GetDccMaterialHash() const = 0;
    virtual void SetDccMaterialHash(uint32 hash) = 0;

    virtual CryCriticalSection& GetSubMaterialResizeLock() = 0;

    virtual void UpdateShaderItems() = 0;

    // </interfuscator:shuffle>
};



//////////////////////////////////////////////////////////////////////////
// Description:
//     IMaterialManagerListener is a callback interface to listenen
//     for special events of material manager, (used by Editor).
struct IMaterialManagerListener
{
    // <interfuscator:shuffle>
    virtual ~IMaterialManagerListener(){}
    // Called when material manager tries to load a material.
    //  nLoadingFlags - Zero or a bitwise combination of the flagas defined in ELoadingFlags.
    virtual void OnCreateMaterial(_smart_ptr<IMaterial> pMaterial) = 0;
    virtual void OnDeleteMaterial(_smart_ptr<IMaterial> pMaterial) = 0;
    virtual bool IsCurrentMaterial(_smart_ptr<IMaterial> pMaterial) const = 0;
    // </interfuscator:shuffle>
};

using IMaterialRef = _smart_ptr<IMaterial>;
//////////////////////////////////////////////////////////////////////////
// Description:
//     IMaterialManager interface provide access to the material manager
//     implemented in 3d engine.
struct IMaterialManager
{
    //! Loading flags
    enum ELoadingFlags
    {
        ELoadingFlagsPreviewMode        = BIT(0),
    };

    // <interfuscator:shuffle>
    virtual ~IMaterialManager(){}

    virtual void GetMemoryUsage(ICrySizer* pSizer) const = 0;

    // Summary:
    //   Creates a new material object and register it with the material manager
    // Return Value:
    //   A newly created object derived from IMaterial.
    virtual _smart_ptr<IMaterial> CreateMaterial(const char* sMtlName, int nMtlFlags = 0) = 0;

    // Summary:
    //   Renames a material object
    // Note:
    //   Do not use IMaterial::SetName directly.
    // Arguments:
    //   pMtl - Pointer to a material object
    //   sNewName - New name to assign to the material
    virtual void RenameMaterial(_smart_ptr<IMaterial> pMtl, const char* sNewName) = 0;

    // Description:
    //    Finds named material.
    virtual _smart_ptr<IMaterial> FindMaterial(const char* sMtlName) const = 0;

    // Description:
    //    Loads material.
    //  nLoadingFlags - Zero or a bitwise combination of the values defined in ELoadingFlags.
    virtual _smart_ptr<IMaterial> LoadMaterial(const char* sMtlName, bool bMakeIfNotFound = true, bool bNonremovable = false, unsigned long nLoadingFlags = 0) = 0;

    // Description:
    //    Loads material from xml.
    virtual _smart_ptr<IMaterial> LoadMaterialFromXml(const char* sMtlName, XmlNodeRef mtlNode) = 0;

    // Description:
    //    Reloads the material from disk.
    virtual void ReloadMaterial(_smart_ptr<IMaterial> pMtl) = 0;

    // Description:
    //    Saves material.
    virtual bool SaveMaterial(XmlNodeRef mtlNode, _smart_ptr<IMaterial> pMtl) = 0;

    // Description:
    //    Clone single material or multi sub material.
    // Arguments:
    //    nSubMtl - when negative all sub materials of MultiSubMtl are cloned, if positive only specified slot is cloned.
    virtual _smart_ptr<IMaterial> CloneMaterial(_smart_ptr<IMaterial> pMtl, int nSubMtl = -1) = 0;

    // Description:
    //    Copy single material.
    virtual void CopyMaterial(_smart_ptr<IMaterial> pMtlSrc, _smart_ptr<IMaterial> pMtlDest, EMaterialCopyFlags flags) = 0;

    // Description:
    //    Clone MultiSubMtl material.
    // Arguments:
    //    sSubMtlName - name of the sub-material to clone, if NULL all submaterial are cloned.
    virtual _smart_ptr<IMaterial> CloneMultiMaterial(_smart_ptr<IMaterial> pMtl, const char* sSubMtlName = 0) = 0;

    // Description:
    //    Associate a special listener callback with material manager inside 3d engine.
    //    This listener callback is used primerly by the editor.
    virtual void SetListener(IMaterialManagerListener* pListener) = 0;

    // Description:
    //    Retrieve a default engine material.
    virtual _smart_ptr<IMaterial> GetDefaultMaterial() = 0;

    // Description:
    //    Retrieve a default engine material for terrain layer
    virtual _smart_ptr<IMaterial> GetDefaultTerrainLayerMaterial() = 0;

    // Description:
    //    Retrieve a default engine material with material layers presets.
    virtual _smart_ptr<IMaterial> GetDefaultLayersMaterial() = 0;

    // Description:
    //    Retrieve a default engine material for drawing helpers.
    virtual _smart_ptr<IMaterial> GetDefaultHelperMaterial() = 0;

    // Description:
    //    Retrieve surface type by name.
    virtual ISurfaceType* GetSurfaceTypeByName(const char* sSurfaceTypeName, const char* sWhy = NULL) = 0;
    virtual int GetSurfaceTypeIdByName(const char* sSurfaceTypeName, const char* sWhy = NULL) = 0;
    // Description:
    //    Retrieve surface type by unique surface type id.
    virtual ISurfaceType* GetSurfaceType(int nSurfaceTypeId, const char* sWhy = NULL) = 0;
    // Description:
    //    Retrieve interface to surface type manager.
    virtual ISurfaceTypeManager* GetSurfaceTypeManager() = 0;

    // Get IMaterial pointer from the CGF material structure.
    //  nLoadingFlags - Zero, or a bitwise combination of the enum items from ELoadingFlags.
    virtual _smart_ptr<IMaterial> LoadCGFMaterial(CMaterialCGF* pMaterialCGF, const char* sCgfFilename, unsigned long nLoadingFlags = 0) = 0;

    // for statistics - call once to get the count (pData==0), again to get the data(pData!=0)
    virtual void GetLoadedMaterials(AZStd::vector<_smart_ptr<IMaterial>>* pData, uint32& nObjCount) const = 0;

    // Updates material data in the renderer
    virtual void RefreshMaterialRuntime() = 0;

    //// Forcing to create ISurfaceTypeManager
    //virtual void CreateSurfaceTypeManager() = 0;
    //// Forcing to destroy ISurfaceTypeManager
    //virtual void ReleaseSurfaceTypeManager() = 0;

    // </interfuscator:shuffle>
};

#endif // CRYINCLUDE_CRYCOMMON_IMATERIAL_H
