/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ITexture.h>
#include <AzTest/AzTest.h>

class ITextureMock
    : public ITexture
{
public:
    MOCK_METHOD0(AddRef,
        int());
    MOCK_METHOD0(Release,
        int());
    MOCK_METHOD0(ReleaseForce,
        int());
    MOCK_CONST_METHOD0(GetClearColor,
        const ColorF& ());
    MOCK_CONST_METHOD0(GetDstFormat,
        const ETEX_Format());
    MOCK_CONST_METHOD0(GetSrcFormat,
        const ETEX_Format());
    MOCK_CONST_METHOD0(GetTexType,
        const ETEX_Type());
    MOCK_METHOD2(ApplyTexture,
        void(int, int));
    MOCK_CONST_METHOD0(GetName,
        const char*());
    MOCK_CONST_METHOD0(GetWidth,
        const int());
    MOCK_CONST_METHOD0(GetHeight,
        const int());
    MOCK_CONST_METHOD0(GetDepth,
        const int());
    MOCK_CONST_METHOD0(GetTextureID,
        const int());
    MOCK_CONST_METHOD0(GetFlags,
        const uint32());
    MOCK_CONST_METHOD0(GetNumMips,
        const int());
    MOCK_CONST_METHOD0(GetRequiredMip,
        const int());
    MOCK_CONST_METHOD0(GetDeviceDataSize,
        const int());
    MOCK_CONST_METHOD0(GetDataSize,
        const int());
    MOCK_CONST_METHOD0(GetTextureType,
        const ETEX_Type());
    MOCK_METHOD1(SetTextureType,
        void(ETEX_Type type));
    MOCK_CONST_METHOD0(IsTextureLoaded,
        const bool());
    MOCK_METHOD4(PrecacheAsynchronously,
        void(float, int, int, int));
    MOCK_METHOD4(GetData32,
        uint8 * (int nSide, int nLevel, uint8 * pDst, ETEX_Format eDstFormat));
    MOCK_METHOD1(SetFilter,
        bool(int nFilter));
    MOCK_METHOD1(SetClamp,
        void(bool bEnable));
    MOCK_CONST_METHOD0(GetAvgBrightness,
        float());
    MOCK_CONST_METHOD1(StreamCalculateMipsSigned,
        int(float fMipFactor));
    MOCK_CONST_METHOD0(GetStreamableMipNumber,
        int());
    MOCK_CONST_METHOD1(GetStreamableMemoryUsage,
        int(int nStartMip));
    MOCK_CONST_METHOD0(GetMinLoadedMip,
        int());
    MOCK_METHOD2(Readback,
        void(AZ::u32 subresourceIndex, StagingHook callback));
    MOCK_METHOD0(Reload,
        bool());
    MOCK_CONST_METHOD0(GetFormatName,
        const char*());
    MOCK_CONST_METHOD0(GetTypeName,
        const char*());
    MOCK_CONST_METHOD0(IsStreamedVirtual,
        const bool());
    MOCK_CONST_METHOD0(IsShared,
        const bool());
    MOCK_CONST_METHOD0(IsStreamable,
        const bool());
    MOCK_CONST_METHOD1(IsStreamedIn,
        bool(const int nMinPrecacheRoundIds[2]));
    MOCK_CONST_METHOD0(GetAccessFrameId,
        const int());
    MOCK_CONST_METHOD0(GetTextureDstFormat,
        const ETEX_Format());
    MOCK_CONST_METHOD0(GetTextureSrcFormat,
        const ETEX_Format());
    MOCK_CONST_METHOD0(IsPostponed,
        bool());
    MOCK_CONST_METHOD1(IsParticularMipStreamed,
        const bool(float fMipFactor));
    MOCK_METHOD3(GetLowResSystemCopy,
        const ColorB * (uint16 & nWidth, uint16 & nHeight, int** ppLowResSystemCopyAtlasId));
    MOCK_METHOD1(SetKeepSystemCopy,
        void(bool bKeepSystemCopy));
    MOCK_METHOD8(UpdateTextureRegion,
        void(const uint8_t * data, int nX, int nY, int nZ, int USize, int VSize, int ZSize, ETEX_Format eTFSrc));
    MOCK_CONST_METHOD0(GetDevTexture,
        CDeviceTexture * ());
};

class ITextureLoadHandlerMock
    : public ITextureLoadHandler
{
public:
    MOCK_METHOD2(LoadTextureData,
        bool(const char* path, STextureLoadData & loadData));
    MOCK_CONST_METHOD1(SupportsExtension,
        bool(const char* ext));
    MOCK_METHOD0(Update,
        void());
};

class IDynTextureMock
    : public IDynTexture
{
public:
    MOCK_METHOD0(Release,
        void());
    MOCK_METHOD4(GetSubImageRect,
        void(uint32 & nX, uint32 & nY, uint32 & nWidth, uint32 & nHeight));
    MOCK_METHOD4(GetImageRect,
        void(uint32 & nX, uint32 & nY, uint32 & nWidth, uint32 & nHeight));
    MOCK_METHOD0(GetTextureID,
        int());
    MOCK_METHOD0(Lock,
        void());
    MOCK_METHOD0(UnLock,
        void());
    MOCK_METHOD0(GetWidth,
        int());
    MOCK_METHOD0(GetHeight,
        int());
    MOCK_METHOD0(IsValid,
        bool());
    MOCK_CONST_METHOD0(GetFlags,
        uint8());
    MOCK_METHOD1(SetFlags,
        void(uint8 flags));
    MOCK_METHOD2(Update,
        bool(int nNewWidth, int nNewHeight));
    MOCK_METHOD2(Apply,
        void(int, int));
    MOCK_METHOD0(ClearRT,
        bool());
    MOCK_METHOD4(SetRT,
        bool(int nRT, bool bPush, struct SDepthTexture* pDepthSurf, bool bScreenVP));
    MOCK_METHOD0(SetRectStates,
        bool());
    MOCK_METHOD2(RestoreRT,
        bool(int nRT, bool bPop));
    MOCK_METHOD0(GetTexture,
        ITexture * ());
    MOCK_METHOD0(SetUpdateMask,
        void());
    MOCK_METHOD0(ResetUpdateMask,
        void());
    MOCK_METHOD0(IsSecondFrame,
        bool());
    MOCK_METHOD2(GetImageData32,
        bool(uint8 * pData, int nDataSize));
};
