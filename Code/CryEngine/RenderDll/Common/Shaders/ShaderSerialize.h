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

#ifndef __SHADERSERIALIZE_H__
#define __SHADERSERIALIZE_H__

#if defined(SHADERS_SERIALIZING)

#include "../ResFile.h"

// Enable this for verbose messages related to r_shadersExport and r_shadersImport
//#define SHADER_SERIALIZE_VERBOSE

//
//  console enums taken from d3d9types.h
//

enum X360AddressModes
{
    X360TADDRESS_WRAP                    = 0,
    X360TADDRESS_MIRROR                  = 1,
    X360TADDRESS_CLAMP                   = 2,
    X360TADDRESS_MIRRORONCE              = 3,
    X360TADDRESS_BORDER_HALF             = 4,
    X360TADDRESS_MIRRORONCE_BORDER_HALF  = 5,
    X360TADDRESS_BORDER                  = 6,
    X360TADDRESS_MIRRORONCE_BORDER       = 7,
};

enum X360FilterType
{
    X360TEXF_NONE            = 2,
    X360TEXF_POINT           = 0,
    X360TEXF_LINEAR          = 1,
    X360TEXF_ANISOTROPIC     = 4,
};

inline void sAlignData(TArray<byte>& Dst, uint32 align)
{
    if (align > 0 && (Dst.Num() & (align - 1)))
    {
        const byte PAD = 0;
        int pad = align - (Dst.Num() & (align - 1));

        for (int i = 0; i < pad; i++)
        {
            Dst.push_back(PAD);
        }
    }
}

template <typename T>
void sAddData(TArray<byte>& Dst, T Src, uint32 align = 0)
{
    int nSize = sizeof(T);
    byte* pDst = Dst.Grow(nSize);

    if (CParserBin::m_bEndians)
    {
        T data = Src;
        SwapEndian(data, eBigEndian);
        memcpy(pDst, &data, nSize);
    }
    else
    {
        memcpy(pDst, &Src, nSize);
    }

    if (align > 0)
    {
        sAlignData(Dst, align);
    }
}

template <typename T>
void sAddDataArray_POD(TArray<byte>& Dst, TArray<T>& Src, uint32& nOffs, uint32 align = 0)
{
    nOffs = Dst.Num();
    int nSize = sizeof(T) * Src.Num();
    if (!nSize)
    {
        return;
    }
    T* pDst = (T*)Dst.Grow(nSize);

    if (CParserBin::m_bEndians)
    {
        for (uint32 i = 0; i < Src.size(); i++)
        {
            T d = Src[i];
            SwapEndian(d, eBigEndian);
            memcpy(pDst, &d, sizeof(T));
            pDst++;
        }
    }
    else
    {
        memcpy(pDst, &Src[0], nSize);
    }

    if (align > 0)
    {
        sAlignData(Dst, align);
    }
}

template <typename T>
void sExport(TArray<byte>& dst, T& data)
{
    int startNum = dst.Num();

    data.Export(dst);
    
    // DEBUG: Check we wrote the data we expected
    // Only works on native export since structures are different sizes on console :(
    if (!CParserBin::m_bEndians)
    {
        AZ_Assert(dst.Num() - startNum == sizeof(T), "ShaderSerialize export size mismatch");
    }
}

template <typename T>
void sAddDataArray(TArray<byte>& Dst, TArray<T>& Src, uint32& nOffs, uint32 align = 0)
{
    nOffs = Dst.Num();
    int nSize = sizeof(T) * Src.Num();
    if (!nSize)
    {
        return;
    }

    if (CParserBin::m_bEndians)
    {
        int startNum = Dst.Num();

        for (uint32 i = 0; i < Src.size(); i++)
        {
            sExport(Dst, Src[i]);
        }

        // DEBUG - compare src and dest, check export was successful
        if (!CParserBin::m_bEndians)
        {
            if (memcmp(&Src[0], &Dst[startNum], nSize) != 0)
            {
                CryFatalError("Copy failed");
            }
        }
    }
    else
    {
        byte* pDst = Dst.Grow(nSize);
        memcpy(pDst, &Src[0], nSize);
    }

    if (align > 0)
    {
        sAlignData(Dst, align);
    }
}

template <typename T>
void SwapEndianEnum(T& e, bool bSwapEndian)
{
    uint32 enumConv = e;
    SwapEndian(enumConv, bSwapEndian);
    e = (T)enumConv;
}

struct SSShaderCacheHeader
{
    int m_SizeOf;
    char m_szVer[16];
    int m_MajorVer;
    int m_MinorVer;
    uint32 m_CRC32;
    uint32 m_SourceCRC32;
    SSShaderCacheHeader()
    {
        memset(this, 0, sizeof(*this));
    }
    AUTO_STRUCT_INFO
};


struct SSShaderRes
{
    int m_nRefCount;
    CResFile* m_pRes[2];
    SSShaderCacheHeader m_Header[2];
    bool m_bReadOnly[2];
    SSShaderRes()
    {
        m_nRefCount = 1;
        m_pRes[0] = NULL;
        m_pRes[1] = NULL;
        m_bReadOnly[0] = true;
        m_bReadOnly[1] = true;
    }
    ~SSShaderRes()
    {
        SAFE_DELETE(m_pRes[0]);
        SAFE_DELETE(m_pRes[1]);
    }
};
typedef std::map<CCryNameTSCRC, SSShaderRes*> FXSShaderRes;
typedef FXSShaderRes::iterator FXSShaderResItor;

struct SSShader
{
    uint64 m_nMaskGenFX;

    EShaderDrawType m_eSHDType;

    uint32 m_Flags;
    uint32 m_Flags2;
    uint32 m_nMDV;

    uint32 m_vertexFormatEnum;   // Base vertex format for the shader (see VertexFormats.h)
    ECull m_eCull;                   // Global culling type

    EShaderType m_eShaderType;

    uint32 m_nTechniques;
    uint32 m_nPasses;
    uint32 m_nPublicParams;
    uint32 m_nFXParams;
    uint32 m_nFXSamplers;
    uint32 m_nFXTextures;
    uint32 m_nFXTexSamplers;
    uint32 m_nFXTexRTs;
    uint32 m_nDataSize;
    uint32 m_nStringsSize;

    uint32 m_nPublicParamsOffset;
    uint32 m_nFXParamsOffset;
    uint32 m_nFXSamplersOffset;
    uint32 m_nFXTexturesOffset;
    uint32 m_nFXTexSamplersOffset;
    uint32 m_nFXTexRTsOffset;
    uint32 m_nTechOffset;
    uint32 m_nPassOffset;
    uint32 m_nStringsOffset;
    uint32 m_nDataOffset;

    SSShader()
    {
        memset(this, 0, sizeof(*this));
    }

    void Export(TArray<byte>& dst)
    {
        uint32 startOffset = dst.Num();

        sAddData(dst, m_nMaskGenFX);
        sAddData(dst, (uint32)m_eSHDType);
        sAddData(dst, m_Flags);
        sAddData(dst, m_Flags2);
        sAddData(dst, m_nMDV);
        sAddData(dst, m_vertexFormatEnum);
        sAddData(dst, (uint32)m_eCull);
        sAddData(dst, (uint32)m_eShaderType);
        sAddData(dst, m_nTechniques);
        sAddData(dst, m_nPasses);
        sAddData(dst, m_nPublicParams);
        sAddData(dst, m_nFXParams);
        sAddData(dst, m_nFXSamplers);
        sAddData(dst, m_nFXTextures);
        sAddData(dst, m_nFXTexSamplers);
        sAddData(dst, m_nFXTexRTs);
        sAddData(dst, m_nDataSize);
        sAddData(dst, m_nStringsSize);

        sAddData(dst, m_nPublicParamsOffset);
        sAddData(dst, m_nFXParamsOffset);
        sAddData(dst, m_nFXSamplersOffset);
        sAddData(dst, m_nFXTexturesOffset);
        sAddData(dst, m_nFXTexSamplersOffset);
        sAddData(dst, m_nFXTexRTsOffset);
        sAddData(dst, m_nTechOffset);
        sAddData(dst, m_nPassOffset);
        sAddData(dst, m_nStringsOffset);
        sAddData(dst, m_nDataOffset);

        uint32 PAD = 0;
        sAddData(dst, PAD); //pad to 64bit
        
        AZ_Assert(dst.Num() - startOffset == sizeof(*this), "ShaderSerialize export size mismatch");
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_nMaskGenFX, eBigEndian);
            SwapEndianEnum(m_eSHDType, eBigEndian);
            ;
            SwapEndian(m_Flags, eBigEndian);
            ;
            SwapEndian(m_Flags2, eBigEndian);
            SwapEndian(m_nMDV, eBigEndian);
            SwapEndianEnum(m_vertexFormatEnum, eBigEndian);
            SwapEndianEnum(m_eCull, eBigEndian);
            SwapEndianEnum(m_eShaderType, eBigEndian);
            SwapEndian(m_nTechniques, eBigEndian);
            SwapEndian(m_nPasses, eBigEndian);
            SwapEndian(m_nPublicParams, eBigEndian);
            SwapEndian(m_nFXParams, eBigEndian);
            SwapEndian(m_nFXSamplers, eBigEndian);
            SwapEndian(m_nFXTextures, eBigEndian);
            SwapEndian(m_nFXTexSamplers, eBigEndian);
            SwapEndian(m_nFXTexRTs, eBigEndian);
            SwapEndian(m_nDataSize, eBigEndian);
            SwapEndian(m_nStringsSize, eBigEndian);
            SwapEndian(m_nPublicParamsOffset, eBigEndian);
            SwapEndian(m_nFXParamsOffset, eBigEndian);
            SwapEndian(m_nFXSamplersOffset, eBigEndian);
            SwapEndian(m_nFXTexturesOffset, eBigEndian);
            SwapEndian(m_nFXTexSamplersOffset, eBigEndian);
            SwapEndian(m_nFXTexRTsOffset, eBigEndian);
            SwapEndian(m_nTechOffset, eBigEndian);
            SwapEndian(m_nPassOffset, eBigEndian);
            SwapEndian(m_nStringsOffset, eBigEndian);
            SwapEndian(m_nDataOffset, eBigEndian);
        }
    }
};

struct SSShaderParam
{
    uint32 m_nameIdx;
    EParamType m_Type;
    UParamVal m_Value;
    int m_nScriptOffs;
    uint8 m_eSemantic;

    SSShaderParam()
    {
        memset(this, 0, sizeof(*this));
    }

    void Export(TArray<byte>& dst)
    {
        uint32 startOffset = dst.Num();

        sAddData(dst, m_nameIdx);
        sAddData(dst, (uint32)m_Type);

        int sizeWritten = 0;

        switch (m_Type)
        {
        case eType_BYTE:
            sAddData(dst, m_Value.m_Byte);
            sizeWritten = sizeof(m_Value.m_Byte);
            break;
        case eType_BOOL:
            sAddData(dst, m_Value.m_Bool);
            sizeWritten = sizeof(m_Value.m_Bool);
            break;
        case eType_SHORT:
            sAddData(dst, m_Value.m_Short);
            sizeWritten = sizeof(m_Value.m_Short);
            break;
        case eType_INT:
            sAddData(dst, m_Value.m_Int);
            sizeWritten = sizeof(m_Value.m_Int);
            break;
        case eType_HALF:
            //half behaves like float?
            sAddData(dst, m_Value.m_Float);
            sizeWritten = sizeof(m_Value.m_Float);
            break;
        case eType_FLOAT:
            sAddData(dst, m_Value.m_Float);
            sizeWritten = sizeof(m_Value.m_Float);
            break;
        case eType_FCOLOR:
        {
            for (int i = 0; i < 4; i++)
            {
                sAddData(dst, m_Value.m_Color[i]);
            }
            sizeWritten = sizeof(m_Value.m_Color);
        }
        break;
        case eType_VECTOR:
        {
            for (int i = 0; i < 3; i++)
            {
                sAddData(dst, m_Value.m_Vector[i]);
            }
            sizeWritten = sizeof(m_Value.m_Vector);
        }
        break;

        //case eType_STRING,
        //case eType_TEXTURE_HANDLE:
        //case eType_CAMERA:
        //case eType_UNKNOWN:
        default:
            CryFatalError("Shader param type not valid for export\n");
            break;
        }

        //Pad to union size
        byte PAD = 0;
        uint32 nPadding = sizeof(UParamVal) - sizeWritten;

        for (uint32 i = 0; i < nPadding; i++)
        {
            sAddData(dst, PAD);
        }

        sAddData(dst, m_nScriptOffs);
        sAddData(dst, m_eSemantic);

        // Align for struct padding
        sAlignData(dst, 8);
        
        AZ_Assert(dst.Num() - startOffset == sizeof(*this), "ShaderSerialize export size mismatch");
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_nameIdx, eBigEndian);
            SwapEndianEnum(m_Type, eBigEndian);

            for (int i = 0; i < 4; i++)
            {
                SwapEndian(m_Value.m_Color[i], eBigEndian);
            }
            SwapEndian(m_nScriptOffs, eBigEndian);
            SwapEndian(m_eSemantic, eBigEndian);
        }
    }
};

struct SSShaderTechnique
{
    int m_nNameOffs;
    int m_nPassesOffs;
    int m_nPasses;
    int m_Flags;
    int8 m_nTechnique[TTYPE_MAX]; //use CONSOLE_MAX for now, PC not supported
    int m_nREsOffs;
    int m_nREs;
    uint32 m_nPreprocessFlags;

    SSShaderTechnique()
    {
        memset(this, 0, sizeof(*this));
    }

    void Export(TArray<byte>& dst)
    {
        sAddData(dst, m_nNameOffs);
        sAddData(dst, m_nPassesOffs);
        sAddData(dst, m_nPasses);
        sAddData(dst, m_Flags);

        // TTYPE_MAX is different on console!
        int TECH_MAX = TTYPE_MAX;

        for (int i = 0; i < TECH_MAX; i++)
        {
            sAddData(dst, m_nTechnique[i]);
        }

        sAlignData(dst, 4);

        sAddData(dst, m_nREsOffs);
        sAddData(dst, m_nREs);
        sAddData(dst, m_nPreprocessFlags);
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            //Cannot import non native data due to TTYPE_MAX being different on different platforms
            CryFatalError("SSShaderTechnique non-native import not supported");

            SwapEndian(m_nNameOffs, eBigEndian);
            SwapEndian(m_nPassesOffs, eBigEndian);
            SwapEndian(m_nPasses, eBigEndian);
            SwapEndian(m_Flags, eBigEndian);

            for (int i = 0; i < TTYPE_MAX; i++)
            {
                SwapEndian(m_nTechnique[i], eBigEndian);
            }

            SwapEndian(m_nREsOffs, eBigEndian);
            SwapEndian(m_nREs, eBigEndian);
            SwapEndian(m_nPreprocessFlags, eBigEndian);
        }
    }
};

struct SSShaderPass
{
    uint32 m_RenderState;
    int8 m_eCull;
    uint8 m_AlphaRef;
    uint16 m_PassFlags;

    uint32 m_nVShaderOffs;
    uint32 m_nPShaderOffs;
    uint32 m_nGShaderOffs;
    uint32 m_nDShaderOffs;
    uint32 m_nCShaderOffs;
    uint32 m_nHShaderOffs;

    uint32 m_nRenderElemOffset;

    void Export(TArray<byte>& dst)
    {
        sAddData(dst, m_RenderState);
        sAddData(dst, m_eCull);
        sAddData(dst, m_AlphaRef);
        sAddData(dst, m_PassFlags);

        sAddData(dst, m_nVShaderOffs);
        sAddData(dst, m_nPShaderOffs);

#if 1
        //if (CParserBin::PlatformSupportsGeometryShaders() &&
        //    CParserBin::PlatformSupportsHullShaders() &&
        //    CParserBin::PlatformSupportsDomainShaders() &&
        //    CParserBin::PlatformSupportsComputeShaders())
        {
            sAddData(dst, m_nGShaderOffs);
            sAddData(dst, m_nDShaderOffs);
            sAddData(dst, m_nCShaderOffs);
            sAddData(dst, m_nHShaderOffs);
        }
#endif

        sAddData(dst, m_nRenderElemOffset);
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_RenderState, eBigEndian);
            SwapEndian(m_eCull, eBigEndian);
            SwapEndian(m_AlphaRef, eBigEndian);
            SwapEndian(m_PassFlags, eBigEndian);

            SwapEndian(m_nVShaderOffs, eBigEndian);
            SwapEndian(m_nPShaderOffs, eBigEndian);
            SwapEndian(m_nGShaderOffs, eBigEndian);
            SwapEndian(m_nDShaderOffs, eBigEndian);
            SwapEndian(m_nCShaderOffs, eBigEndian);
            SwapEndian(m_nHShaderOffs, eBigEndian);

            SwapEndian(m_nRenderElemOffset, eBigEndian);
        }
    }
};

struct SSLightEval
{};

struct SCHWShader
{
    uint64 m_nMaskGenShader;
    uint64 m_nMaskGenFX;
    uint64 m_nMaskAnd_RT;
    uint64 m_nMaskOr_RT;

    int m_Flags;
    uint32 m_nsNameSourceFX;
    uint32 m_nsName;
    uint32 m_nsEntryFunc;
    EHWShaderClass m_eSHClass;
    uint32 m_nTokens;
    uint32 m_nTableEntries;
    uint32 m_nSamplers;
    uint32 m_nParams;
    uint32 m_dwShaderType;

    SCHWShader()
    {
        memset(this, 0, sizeof(*this));
    }

    void Export(TArray<byte>& dst)
    {
        uint32 startOffset = dst.Num();

        sAddData(dst, m_nMaskGenShader);
        sAddData(dst, m_nMaskGenFX);
        sAddData(dst, m_nMaskAnd_RT);
        sAddData(dst, m_nMaskOr_RT);
        sAddData(dst, m_Flags);
        sAddData(dst, m_nsNameSourceFX);
        sAddData(dst, m_nsName);
        sAddData(dst, m_nsEntryFunc);
        sAddData(dst, (uint32)m_eSHClass);
        sAddData(dst, m_nTokens);
        sAddData(dst, m_nTableEntries);
        sAddData(dst, m_nSamplers);
        sAddData(dst, m_nParams);
        sAddData(dst, m_dwShaderType);

        //uint32 PAD=0;
        //sAddData(dst, PAD); //pad up to 64bit align due to uint64

        AZ_Assert(dst.Num() - startOffset == sizeof(*this), "ShaderSerialize export size mismatch");
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_nMaskGenShader, eBigEndian);
            SwapEndian(m_nMaskGenFX, eBigEndian);
            SwapEndian(m_nsNameSourceFX, eBigEndian);
            SwapEndian(m_nsName, eBigEndian);
            SwapEndian(m_nsEntryFunc, eBigEndian);
            SwapEndianEnum(m_eSHClass, eBigEndian);
            SwapEndian(m_nTokens, eBigEndian);
            SwapEndian(m_nTableEntries, eBigEndian);
            SwapEndian(m_nSamplers, eBigEndian);
            SwapEndian(m_nParams, eBigEndian);
            SwapEndian(m_dwShaderType, eBigEndian);
        }
    }
};

struct SSTexSamplerFX
{
    int m_nsName;
    int m_nsNameTexture;

    int m_eTexType;
    int m_nSamplerSlot;
    uint32 m_nFlags;
    uint32 m_nTexFlags;
    int m_nRTIdx;
    uint32 m_bTexState;
    STexState ST;

    SSTexSamplerFX()
    {
        // if any of these assert fails it means the platform headers have changed
        // we must update the cached equivalents (see top of file)

        memset(this, 0, sizeof(SSTexSamplerFX));
    }

    SSTexSamplerFX(const SSTexSamplerFX& rhs)
    {
        memcpy(this, &rhs, sizeof(SSTexSamplerFX));
    }

    SSTexSamplerFX& operator = (const SSTexSamplerFX& rhs)
    {
        memcpy(this, &rhs, sizeof(SSTexSamplerFX));
        return *this;
    }

    void Export(TArray<byte>& dst)
    {
        uint32 startOffset = dst.Num();

        sAddData(dst, m_nsName);
        sAddData(dst, m_nsNameTexture);
        sAddData(dst, m_eTexType);
        sAddData(dst, m_nSamplerSlot);
        sAddData(dst, m_nFlags);
        sAddData(dst, m_nTexFlags);
        sAddData(dst, m_nRTIdx);
        sAddData(dst, m_bTexState);

        sAddData(dst, ST.m_nMinFilter);
        sAddData(dst, ST.m_nMagFilter);
        sAddData(dst, ST.m_nMipFilter);
        sAddData(dst, ST.m_nAddressU);
        sAddData(dst, ST.m_nAddressV);
        sAddData(dst, ST.m_nAddressW);
        sAddData(dst, ST.m_nAnisotropy);
        sAddData(dst, ST.padding);
        sAddData(dst, ST.m_dwBorderColor);

        sAddData(dst, ST.m_MipBias);

        uint64 iPad = 0;
        sAddData(dst, iPad); //m_pDeviceState
        sAddData(dst, ST.m_bActive);
        sAddData(dst, ST.m_bComparison);
        sAddData(dst, ST.m_bSRGBLookup);
        byte bPad = 0;
        sAddData(dst, bPad);

        AZ_Assert(dst.Num() - startOffset == sizeof(*this), "ShaderSerialize export size mismatch");
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_nsName, eBigEndian);
            SwapEndian(m_nsNameTexture, eBigEndian);
            SwapEndian(m_eTexType, eBigEndian);
            SwapEndian(m_nSamplerSlot, eBigEndian);
            SwapEndian(m_nFlags, eBigEndian);
            SwapEndian(m_nTexFlags, eBigEndian);
            SwapEndian(m_nRTIdx, eBigEndian);
            SwapEndian(m_bTexState, eBigEndian);
            SwapEndian(ST.m_dwBorderColor, eBigEndian);
            SwapEndian(ST.m_bActive, eBigEndian);
            SwapEndian(ST.m_bComparison, eBigEndian);
            SwapEndian(ST.m_bSRGBLookup, eBigEndian);
        }

        ST.PostCreate();
    }
};

struct SSHRenderTarget
{
    ERenderOrder m_eOrder;
    int m_nProcessFlags;   // FSPR_ flags
    uint32 m_nsTargetName;
    int m_nWidth;
    int m_nHeight;
    ETEX_Format m_eTF;
    int m_nIDInPool;
    ERTUpdate m_eUpdateType;
    uint32 m_bTempDepth;
    ColorF m_ClearColor;
    float m_fClearDepth;
    uint32 m_nFlags;
    uint32 m_nFilterFlags;

    SSHRenderTarget()
    {
        memset(this, 0, sizeof(SSHRenderTarget));
    }

    // = operator to ensure padding is copied
    SSHRenderTarget& operator = (const SSHRenderTarget& rhs)
    {
        memcpy(this, &rhs, sizeof(SSHRenderTarget));
        return *this;
    }

    void Export(TArray<byte>& dst)
    {
        sAddData(dst, (uint32)m_eOrder);
        sAddData(dst, m_nProcessFlags);
        sAddData(dst, m_nsTargetName);
        sAddData(dst, m_nWidth);
        sAddData(dst, m_nHeight);

        sAddData(dst, (uint8)m_eTF);
        byte PAD = 0;
        sAddData(dst, PAD);
        sAddData(dst, PAD);
        sAddData(dst, PAD);

        sAddData(dst, m_nIDInPool);
        sAddData(dst, (uint32)m_eUpdateType);
        sAddData(dst, m_bTempDepth);
        sAddData(dst, m_ClearColor.r);
        sAddData(dst, m_ClearColor.g);
        sAddData(dst, m_ClearColor.b);
        sAddData(dst, m_ClearColor.a);
        sAddData(dst, m_fClearDepth);
        sAddData(dst, m_nFlags);
        sAddData(dst, m_nFilterFlags);
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndianEnum(m_eOrder, eBigEndian);
            SwapEndian(m_nProcessFlags, eBigEndian);
            SwapEndian(m_nsTargetName, eBigEndian);
            SwapEndian(m_nWidth, eBigEndian);
            SwapEndian(m_nHeight, eBigEndian);
            SwapEndianEnum(m_eTF, eBigEndian);
            SwapEndian(m_nIDInPool, eBigEndian);
            SwapEndianEnum(m_eUpdateType, eBigEndian);
            SwapEndian(m_bTempDepth, eBigEndian);
            SwapEndian(m_ClearColor.r, eBigEndian);
            SwapEndian(m_ClearColor.g, eBigEndian);
            SwapEndian(m_ClearColor.b, eBigEndian);
            SwapEndian(m_ClearColor.a, eBigEndian);
            SwapEndian(m_fClearDepth, eBigEndian);
            SwapEndian(m_nFlags, eBigEndian);
            SwapEndian(m_nFilterFlags, eBigEndian);
        }
    }
};

struct SSFXParam
{
    int m_nsName;      // Parameter name
    uint32 m_nFlags;
    short m_nParameters; // Number of paramters
    short m_nComps;     // Number of components in single parameter
    uint32 m_nsAnnotations; // Additional parameters (between <>)
    uint32 m_nsSemantic;    // Parameter app handling type (after ':')
    uint32 m_nsValues;    // Parameter values (after '=')
    byte   m_eType;     // EParamType
    int8   m_nCB;

    short  m_nRegister[eHWSC_Num];

    SSFXParam()
    {
        memset(this, 0, sizeof(*this));
    }

    void Export(TArray<byte>& dst)
    {
        uint32 startOffset = dst.Num();

        sAddData(dst, m_nsName);
        sAddData(dst, m_nFlags);
        sAddData(dst, m_nParameters);
        sAddData(dst, m_nComps);
        sAddData(dst, m_nsAnnotations);
        sAddData(dst, m_nsSemantic);
        sAddData(dst, m_nsValues);
        sAddData(dst, m_eType);
        sAddData(dst, m_nCB);

        for (int i = 0; i < eHWSC_Num; i++)
        {
            sAddData(dst, m_nRegister[i]);
        }

        // Align for struct padding
        sAlignData(dst, 8);

        AZ_Assert(dst.Num() - startOffset == sizeof(*this), "ShaderSerialize export size mismatch");
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_nsName, eBigEndian);
            SwapEndian(m_nFlags, eBigEndian);
            SwapEndian(m_nParameters, eBigEndian);
            SwapEndian(m_nComps, eBigEndian);
            SwapEndian(m_nsAnnotations, eBigEndian);
            SwapEndian(m_nsSemantic, eBigEndian);
            SwapEndian(m_nsValues, eBigEndian);
            SwapEndian(m_eType, eBigEndian);
            SwapEndian(m_nCB, eBigEndian);

            for (int i = 0; i < eHWSC_Num; i++)
            {
                SwapEndian(m_nRegister[i], eBigEndian);
            }
        }
    }
};

struct SSFXSampler
{
    int m_nsName;      // Parameter name
    uint32 m_nFlags;
    short m_nArray;       // Number of paramters
    uint32 m_nsAnnotations; // Additional parameters (between <>)
    uint32 m_nsSemantic;    // Parameter app handling type (after ':')
    uint32 m_nsValues;    // Parameter values (after '=')
    byte   m_eType;     // EParamType

    short  m_nRegister[eHWSC_Num];

    SSFXSampler()
    {
        memset(this, 0, sizeof(*this));
    }

    void Export(TArray<byte>& dst)
    {
        uint32 startOffset = dst.Num();

        sAddData(dst, m_nsName);
        sAddData(dst, m_nFlags);
        sAddData(dst, m_nArray);
        sAddData(dst, m_nsAnnotations);
        sAddData(dst, m_nsSemantic);
        sAddData(dst, m_nsValues);
        sAddData(dst, m_eType);

        for (int i = 0; i < eHWSC_Num; i++)
        {
            sAddData(dst, m_nRegister[i]);
        }
        
        // Align for struct padding
        sAlignData(dst, 8);

        AZ_Assert(dst.Num() - startOffset == sizeof(*this), "ShaderSerialize export size mismatch");
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_nsName, eBigEndian);
            SwapEndian(m_nFlags, eBigEndian);
            SwapEndian(m_nArray, eBigEndian);
            SwapEndian(m_nsAnnotations, eBigEndian);
            SwapEndian(m_nsSemantic, eBigEndian);
            SwapEndian(m_nsValues, eBigEndian);
            SwapEndian(m_eType, eBigEndian);

            for (int i = 0; i < eHWSC_Num; i++)
            {
                SwapEndian(m_nRegister[i], eBigEndian);
            }
        }
    }
};
struct SSFXTexture
{
    int m_nsName;      // Parameter name
    int m_nsNameTexture;
    uint32 m_nFlags;
    short m_nArray;       // Number of paramters
    uint32 m_nsAnnotations; // Additional parameters (between <>)
    uint32 m_nsSemantic;    // Parameter app handling type (after ':')
    uint32 m_nsValues;    // Parameter values (after '=')
    bool   m_bSRGBLookup;
    byte   m_eType;     // EParamType

    short  m_nRegister[eHWSC_Num];

    SSFXTexture()
    {
        memset(this, 0, sizeof(*this));
    }

    void Export(TArray<byte>& dst)
    {
        uint32 startOffset = dst.Num();

        sAddData(dst, m_nsName);
        sAddData(dst, m_nsNameTexture);
        sAddData(dst, m_nFlags);
        sAddData(dst, m_nArray);
        sAddData(dst, m_nsAnnotations);
        sAddData(dst, m_nsSemantic);
        sAddData(dst, m_nsValues);
        sAddData(dst, m_eType);
        sAddData(dst, m_bSRGBLookup);

        for (int i = 0; i < eHWSC_Num; i++)
        {
            sAddData(dst, m_nRegister[i]);
        }
        
        // Align for struct padding
        sAlignData(dst, 8);

        AZ_Assert(dst.Num() - startOffset == sizeof(*this), "ShaderSerialize export size mismatch");
    }

    void Import(const byte* pData)
    {
        memcpy(this, pData, sizeof(*this));

        if (CParserBin::m_bEndians)
        {
            SwapEndian(m_nsName, eBigEndian);
            SwapEndian(m_nsNameTexture, eBigEndian);
            SwapEndian(m_nFlags, eBigEndian);
            SwapEndian(m_nArray, eBigEndian);
            SwapEndian(m_nsAnnotations, eBigEndian);
            SwapEndian(m_nsSemantic, eBigEndian);
            SwapEndian(m_nsValues, eBigEndian);
            SwapEndian(m_eType, eBigEndian);
            SwapEndian(m_bSRGBLookup, eBigEndian);

            for (int i = 0; i < eHWSC_Num; i++)
            {
                SwapEndian(m_nRegister[i], eBigEndian);
            }
        }
    }
};

struct SShaderSerializeContext
{
    SSShader SSR;
    TArray<SSShaderParam> Params;
    TArray<SSFXParam> FXParams;
    TArray<SSFXSampler> FXSamplers;
    TArray<SSFXTexture> FXTextures;
    TArray<SSTexSamplerFX> FXTexSamplers;
    TArray<SSHRenderTarget> FXTexRTs;
    TArray<SSShaderTechnique> Techniques;
    TArray<SSShaderPass> Passes;
    TArray<char> Strings;
    TArray<byte> Data;

    std::map<uint32, uint32> m_strTable;

    uint32 AddString(const char* pStr)
    {
        uint32 crc = CCrc32::Compute(pStr);

        std::map<uint32, uint32>::iterator strIt = m_strTable.find(crc);

        if (strIt != m_strTable.end())
        {
            uint32 offset = strIt->second;

            // Debug, check strings are correct
#if 0
            if (strcmp(&Strings[offset], pStr))
            {
                CryLogAlways("Error: %s is not %s\n", pStr, &Strings[offset]);
            }
#endif

            return offset;
        }

        uint32 nChars = Strings.Num();
        m_strTable[crc] = nChars;
        Strings.AddString(pStr);
        return nChars;
    }
};

class CShaderSerialize
{
    friend class CShaderMan;
    friend class CShaderManBin;

public:
    enum ShaderImportResults
    {
        SHADER_IMPORT_SUCCESS,          // Shader mask exists in the fxb lookup table
        SHADER_IMPORT_MISSING_ENTRY,    // We have a valid fxb lookup table, but the current permutation is missing
        SHADER_IMPORT_FAILURE,          // No fxb table exists for this entire shader
    };

    void ClearSResourceCache();

private:
    bool _OpenSResource(float fVersion, SSShaderRes* pSR, CShader* pSH, int nCache, CResFile* pRF, bool bReadOnly);
    bool OpenSResource(const char* szName,  SSShaderRes* pSR, CShader* pSH, bool bDontUseUserFolder, bool bReadOnly);
    bool CreateSResource(CShader* pSH, SSShaderRes* pSR, CCryNameTSCRC& SName, bool bDontUseUserFolder, bool bReadOnly);
    SSShaderRes* InitSResource(CShader* pSH, bool bDontUseUserFolder, bool bReadOnly);
    
    // Simple query to see if the SResource exists in the hash table
    bool DoesSResourceExist(CShader* pSH);

    // Helper function to abstract calling ExportHWShader per shader stage  
    void ExportHWShaderStage(SShaderSerializeContext& SC, CHWShader* shader, uint32& outShaderOffset);

    bool ExportHWShader(CHWShader* pShader, struct SShaderSerializeContext& SC);

    CHWShader* ImportHWShader(SShaderSerializeContext& SC, int nOffs, uint32 CRC32, CShader* pSH);

    bool ExportShader(CShader* pSH, CShaderManBin& binShaderMgr);
    ShaderImportResults ImportShader(CShader* pSH, CShaderManBin& binShaderMgr);
    bool CheckFXBExists(CShader* pSH);

    FXSShaderRes m_SShaderResources;
    string m_customSerialisePath;
};

_inline const char* sString(int nOffs, TArray<char>& Strings)
{
    return &Strings[nOffs];
}

#endif // SHADERS_SERIALIZING

#endif

