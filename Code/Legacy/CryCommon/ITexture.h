/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_ITEXTURE_H
#define CRYINCLUDE_CRYCOMMON_ITEXTURE_H
#pragma once

#include <AzCore/PlatformDef.h>

#include "Cry_Math.h"
#include "Cry_Color.h"
#include "Tarray.h"
#include <smartptr.h>
class CTexture;

#ifndef COMPILER_SUPPORTS_ENUM_SPECIFICATION
# if defined(_MSC_VER)
#  define COMPILER_SUPPORTS_ENUM_SPECIFICATION 1
# else
#  define COMPILER_SUPPORTS_ENUM_SPECIFICATION 0
# endif
#endif

#if COMPILER_SUPPORTS_ENUM_SPECIFICATION
enum ETEX_Type : uint8
#else
typedef uint8 ETEX_Type;
enum eTEX_Type
#endif
{
    eTT_1D = 0,
    eTT_2D,
    eTT_3D,
    eTT_Cube,
    eTT_CubeArray,
    eTT_Dyn2D,
    eTT_User,
    eTT_NearestCube,

    eTT_2DArray,
    eTT_2DMS,

    eTT_Auto2D,

    eTT_MaxTexType,     // not used
};


// Texture formats
#if COMPILER_SUPPORTS_ENUM_SPECIFICATION
enum ETEX_Format : uint8
#else
typedef uint8 ETEX_Format;
enum eTEX_Format
#endif
{
    eTF_Unknown = 0,
    eTF_R8G8B8A8S,
    eTF_R8G8B8A8 = 2, // may be saved into file

    eTF_A8 = 4,
    eTF_R8,
    eTF_R8S,
    eTF_R16,
    eTF_R16F,
    eTF_R32F,
    eTF_R8G8,
    eTF_R8G8S,
    eTF_R16G16,
    eTF_R16G16S,
    eTF_R16G16F,
    eTF_R11G11B10F,
    eTF_R10G10B10A2,
    eTF_R16G16B16A16,
    eTF_R16G16B16A16S,
    eTF_R16G16B16A16F,
    eTF_R32G32B32A32F,

    eTF_CTX1,
    eTF_BC1 = 22, // may be saved into file
    eTF_BC2 = 23, // may be saved into file
    eTF_BC3 = 24, // may be saved into file
    eTF_BC4U,     // 3Dc+
    eTF_BC4S,
    eTF_BC5U,     // 3Dc
    eTF_BC5S,
    eTF_BC6UH,
    eTF_BC6SH,
    eTF_BC7,
    eTF_R9G9B9E5,

    // hardware depth buffers
    eTF_D16,
    eTF_D24S8,
    eTF_D32F,
    eTF_D32FS8,

    // only available as hardware format under DX11.1 with DXGI 1.2
    eTF_B5G6R5,
    eTF_B5G5R5,
    eTF_B4G4R4A4,

    // only available as hardware format under OpenGL
    eTF_EAC_R11,
    eTF_EAC_RG11,
    eTF_ETC2,
    eTF_ETC2A,

    // only available as hardware format under DX9
    eTF_A8L8,
    eTF_L8,
    eTF_L8V8U8,
    eTF_B8G8R8,
    eTF_L8V8U8X8,
    eTF_B8G8R8X8,
    eTF_B8G8R8A8,

    eTF_PVRTC2,
    eTF_PVRTC4,

    eTF_ASTC_4x4,
    eTF_ASTC_5x4,
    eTF_ASTC_5x5,
    eTF_ASTC_6x5,
    eTF_ASTC_6x6,
    eTF_ASTC_8x5,
    eTF_ASTC_8x6,
    eTF_ASTC_8x8,
    eTF_ASTC_10x5,
    eTF_ASTC_10x6,
    eTF_ASTC_10x8,
    eTF_ASTC_10x10,
    eTF_ASTC_12x10,
    eTF_ASTC_12x12,

    // add R16 unsigned int format for hardware that do not support float point rendering
    eTF_R16U,
    eTF_R16G16U,
    eTF_R10G10B10A2UI,

    eTF_MaxFormat               // unused, must be always the last in the list
};

#if COMPILER_SUPPORTS_ENUM_SPECIFICATION
enum ETEX_TileMode : uint8
#else
typedef uint8 ETEX_TileMode;
enum eTEX_TileMode
#endif
{
    eTM_None = 0,
    eTM_LinearPadded,
    eTM_Optimal,
};


enum ETextureFlags
{
    FT_NOMIPS                  = 0x00000001,
    FT_TEX_NORMAL_MAP          = 0x00000002,
    FT_TEX_WAS_NOT_PRE_TILED   = 0x00000004,
    FT_USAGE_DEPTHSTENCIL      = 0x00000008,
    FT_USAGE_ALLOWREADSRGB     = 0x00000010,
    FT_FILESINGLE              = 0x00000020, // suppress loading of additional files like _DDNDIF (faster, RC can tag the file for that)
    FT_TEX_FONT                = 0x00000040,
    FT_HAS_ATTACHED_ALPHA      = 0x00000080,
    FT_USAGE_UNORDERED_ACCESS  = 0x00000100,
    FT_USAGE_READBACK          = 0x00000200,
    FT_USAGE_MSAA              = 0x00000400,
    FT_FORCE_MIPS              = 0x00000800,
    FT_USAGE_RENDERTARGET      = 0x00001000,
    FT_USAGE_DYNAMIC           = 0x00002000,
    FT_STAGE_READBACK          = 0x00004000,
    FT_STAGE_UPLOAD            = 0x00008000,
    FT_DONT_RELEASE            = 0x00010000,
    FT_ASYNC_PREPARE           = 0x00020000,
    FT_DONT_STREAM             = 0x00040000,
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(ITexture_h)
#endif

#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
#if defined(AZ_PLATFORM_IOS)
    FT_USAGE_MEMORYLESS        = 0x00080000, //reusing an unused bit for ios
#else
    FT_USAGE_PREDICATED_TILING = 0x00080000, //unused
#endif
#endif
    FT_FAILED                  = 0x00100000,
    FT_FROMIMAGE               = 0x00200000,
    FT_STATE_CLAMP             = 0x00400000,
    FT_USAGE_ATLAS             = 0x00800000,
    FT_ALPHA                   = 0x01000000,
    FT_REPLICATE_TO_ALL_SIDES  = 0x02000000,
    FT_KEEP_LOWRES_SYSCOPY     = 0x04000000, // keep low res copy in system memory for voxelization on CPU
    FT_SPLITTED                = 0x08000000, // for split dds files
    FT_USE_HTILE               = 0x10000000,
    FT_IGNORE_PRECACHE         = 0x20000000,
    FT_COMPOSITE               = 0x40000000,
    FT_USAGE_UAV_RWTEXTURE     = 0x80000000,
};

struct SDepthTexture;

struct STextureStreamingStats
{
    STextureStreamingStats(bool bComputeTexturesPerFrame)
        : bComputeReuquiredTexturesPerFrame(bComputeTexturesPerFrame)
    {
        nMaxPoolSize = 0;
        nCurrentPoolSize = 0;
        nStreamedTexturesSize = 0;
        nStaticTexturesSize = 0;
        nThroughput = 0;
        nNumTexturesPerFrame = 0;
        nRequiredStreamedTexturesSize = 0;
        nRequiredStreamedTexturesCount = 0;
        bPoolOverflow = false;
        bPoolOverflowTotally = false;
        fPoolFragmentation = 0.0f;
    }
    size_t nMaxPoolSize;
    size_t nCurrentPoolSize;
    size_t nStreamedTexturesSize;
    size_t nStaticTexturesSize;
    uint32 nNumTexturesPerFrame;
    size_t nThroughput;
    size_t nRequiredStreamedTexturesSize;
    uint32 nRequiredStreamedTexturesCount;
    float fPoolFragmentation;
    uint32 bPoolOverflow : 1;
    uint32 bPoolOverflowTotally : 1;
    const bool bComputeReuquiredTexturesPerFrame;
};


//////////////////////////////////////////////////////////////////////
// Texture object interface
class CDeviceTexture;
class ITexture
{
protected:
    virtual ~ITexture() {}
public:

    // <interfuscator:shuffle>
    virtual int AddRef() = 0;
    virtual int Release() = 0;
    virtual int ReleaseForce() = 0;

    virtual const ColorF& GetClearColor() const = 0;
    virtual const ETEX_Format GetDstFormat() const = 0;
    virtual const ETEX_Format GetSrcFormat() const = 0;
    virtual const ETEX_Type GetTexType() const = 0;
    virtual void ApplyTexture(int nTUnit, int nState = -1) = 0;
    virtual const char* GetName() const = 0;
    virtual const int GetWidth() const = 0;
    virtual const int GetHeight() const = 0;
    virtual const int GetDepth() const = 0;
    virtual const int GetTextureID() const = 0;
    virtual const uint32 GetFlags() const = 0;
    virtual const int GetNumMips() const = 0;
    virtual const int GetRequiredMip() const = 0;
    virtual const int GetDeviceDataSize() const  = 0;
    virtual const int GetDataSize() const = 0;
    virtual const ETEX_Type GetTextureType() const = 0;
    // Sets the texture type of the texture to be used before the texture is loaded.
    // Once the texture is loaded the type from the file will overwrite whatever
    // value was set here.
    virtual void SetTextureType(ETEX_Type type) = 0;
    virtual const bool IsTextureLoaded() const = 0;
    virtual void PrecacheAsynchronously(float fMipFactor, int nFlags, int nUpdateId, int nCounter = 1) = 0;
    virtual uint8* GetData32(int nSide = 0, int nLevel = 0, uint8* pDst = NULL, ETEX_Format eDstFormat = eTF_R8G8B8A8) = 0;
    virtual bool SetFilter(int nFilter) = 0;   // FILTER_ flags
    virtual void SetClamp(bool bEnable) = 0; // Texture addressing set
    virtual float GetAvgBrightness() const = 0;

    virtual int StreamCalculateMipsSigned(float fMipFactor) const = 0;
    virtual int GetStreamableMipNumber() const = 0;
    virtual int GetStreamableMemoryUsage(int nStartMip) const = 0;
    virtual int GetMinLoadedMip() const = 0;

    using StagingHook = AZStd::function<bool(void*, uint32, uint32)>;
    virtual void Readback(AZ::u32 subresourceIndex, StagingHook callback) = 0;

    virtual bool Reload() = 0;
    // Used for debugging/profiling.
    virtual const char* GetFormatName() const = 0;
    virtual const char* GetTypeName() const = 0;
    virtual const bool IsStreamedVirtual() const = 0;
    virtual const bool IsShared() const = 0;
    virtual const bool IsStreamable() const = 0;
    virtual bool IsStreamedIn(const int nMinPrecacheRoundIds[2]) const = 0;
    virtual const int GetAccessFrameId() const = 0;

    virtual const ETEX_Format GetTextureDstFormat() const = 0;
    virtual const ETEX_Format GetTextureSrcFormat() const = 0;

    virtual bool IsPostponed() const = 0;
    virtual const bool IsParticularMipStreamed(float fMipFactor) const = 0;

    // get low res system memory (used for CPU voxelization)
    virtual const ColorB* GetLowResSystemCopy([[maybe_unused]] uint16& nWidth, [[maybe_unused]] uint16& nHeight, [[maybe_unused]] int** ppLowResSystemCopyAtlasId) { return 0; }

    // </interfuscator:shuffle>

    void GetMemoryUsage([[maybe_unused]] ICrySizer* pSizer) const
    {
        COMPILE_TIME_ASSERT(eTT_MaxTexType <= 255);
        COMPILE_TIME_ASSERT(eTF_MaxFormat <= 255);
        /*LATER*/
    }

    virtual void SetKeepSystemCopy(bool bKeepSystemCopy) = 0;
    virtual void UpdateTextureRegion(const uint8_t* data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc) = 0;
    virtual CDeviceTexture* GetDevTexture() const = 0;

};

struct STextureLoadData
{
    void*               m_pData;
    size_t              m_DataSize;
    int                 m_Width;
    int                 m_Height;
    ETEX_Format         m_Format;
    int                 m_NumMips;
    int                 m_nFlags;
    ITexture*           m_pTexture;
    STextureLoadData()
        : m_pData(nullptr)
        , m_Width(0)
        , m_Height(0)
        , m_Format(eTF_Unknown)
        , m_NumMips(0)
        , m_nFlags(0)
        , m_pTexture(nullptr)
    {
    }
    ~STextureLoadData()
    {
        if (m_pData)
        {
            CryModuleFree(m_pData);
        }
    }

    static void* AllocateData(size_t dataSize)
    {
        return CryModuleMalloc(dataSize);
    }
};
struct ITextureLoadHandler
{
    virtual ~ITextureLoadHandler() {}
    virtual bool LoadTextureData(const char* path, STextureLoadData& loadData) = 0;
    virtual bool SupportsExtension(const char* ext) const = 0;
    virtual void Update() = 0;
};

//=========================================================================================

class IDynTexture
{
public:
    enum
    {
        fNeedRegenerate = 1ul << 0,
    };
    // <interfuscator:shuffle>
    virtual ~IDynTexture(){}
    virtual void Release() = 0;
    virtual void GetSubImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight) = 0;
    virtual void GetImageRect(uint32& nX, uint32& nY, uint32& nWidth, uint32& nHeight) = 0;
    virtual int GetTextureID() = 0;
    virtual void Lock() = 0;
    virtual void UnLock() = 0;
    virtual int GetWidth() = 0;
    virtual int GetHeight() = 0;
    virtual bool IsValid() = 0;
    virtual uint8 GetFlags() const = 0;
    virtual void SetFlags([[maybe_unused]] uint8 flags) {}
    virtual bool Update(int nNewWidth, int nNewHeight) = 0;
    virtual void Apply(int nTUnit, int nTS = -1) = 0;
    virtual bool ClearRT() = 0;
    virtual bool SetRT(int nRT, bool bPush, struct SDepthTexture* pDepthSurf, bool bScreenVP = false) = 0;
    virtual bool SetRectStates() = 0;
    virtual bool RestoreRT(int nRT, bool bPop) = 0;
    virtual ITexture* GetTexture() = 0;
    virtual void SetUpdateMask() = 0;
    virtual void ResetUpdateMask() = 0;
    virtual bool IsSecondFrame() = 0;
    virtual bool GetImageData32([[maybe_unused]] uint8* pData, [[maybe_unused]] int nDataSize) { return 0; }
    // </interfuscator:shuffle>
};

// Animating Texture sequence definition
class ITexAnim
{
public:
    virtual ~ITexAnim() {};
    virtual void Release() = 0;
    virtual void AddRef() = 0;
};

struct AZ_DEPRECATED(STexAnim, "STexAnim has been deprecated and replaced by the abstract interface ITexAnim above and CTexAnim in RenderDLL/Common/Textures/Texture.h. This was done to keep proper ref counting between CryRenderDLL and EditorLib.") {};

struct STexComposition
{
    _smart_ptr<ITexture> pTexture;
    uint16 nSrcSlice;
    uint16 nDstSlice;
};

#endif // CRYINCLUDE_CRYCOMMON_ITEXTURE_H
