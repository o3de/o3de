
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

// Description : rendering


#include "Cry3DEngine_precompiled.h"

#include "3dEngine.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "Ocean.h"
#include <Terrain/Bus/TerrainProviderBus.h>
#include <AzFramework/Terrain/TerrainDataRequestBus.h>
#include "DecalManager.h"
#include "SkyLightManager.h"

#include "CullBuffer.h"
#include "LightEntity.h"
#include "FogVolumeRenderNode.h"
#include "ObjectsTree.h"
#include "CloudsManager.h"
#include "MatMan.h"
#include "VolumeObjectRenderNode.h"
#include "CryPath.h"
#include "ILocalMemoryUsage.h"
#include "BitFiddling.h"
#include "ObjMan.h"
#include "GeomCacheManager.h"
#include "ClipVolumeManager.h"
#include "ITimeOfDay.h"
#include "Environment/OceanEnvironmentBus.h"

#include <ThermalInfo.h>



#ifdef GetCharWidth
#undef GetCharWidth
#endif //GetCharWidth


#ifdef WIN32
#include <CryWindows.h>
#endif

#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/API/AtomActiveInterface.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN
#include <AzCore/Interface/Interface.h>
#include "../RenderDll/Common/Memory/VRAMDrillerBus.h"

////////////////////////////////////////////////////////////////////////////////////////
// RenderScene
////////////////////////////////////////////////////////////////////////////////////////
#define FREE_MEMORY_YELLOW_LIMIT (30)
#define FREE_MEMORY_RED_LIMIT    (10)
#define DISPLAY_INFO_SCALE       (1.25f)
#define DISPLAY_INFO_SCALE_SMALL (1.1f)
#define STEP_SMALL_DIFF          (2.f)

#if defined(WIN32) || defined(WIN64) || defined(MAC)
// for panorama screenshots
class CStitchedImage
    : public Cry3DEngineBase
{
public:
    CStitchedImage(C3DEngine&  rEngine,
        const uint32            dwWidth,
        const uint32            dwHeight,
        const uint32            dwVirtualWidth,
        const uint32            dwVirtualHeight,
        const uint32            dwSliceCount,
        const f32                   fTransitionSize,
        const bool              bMetaData = false)
        : m_rEngine(rEngine)
        , m_dwWidth(dwWidth)
        , m_dwHeight(dwHeight)
        , m_fInvWidth(1.f / static_cast<f32>(dwWidth))
        , m_fInvHeight(1.f / static_cast<f32>(dwHeight))
        , m_dwVirtualWidth(dwVirtualWidth)
        , m_dwVirtualHeight(dwVirtualHeight)
        , m_fInvVirtualWidth(1.f / static_cast<f32>(dwVirtualWidth))
        , m_fInvVirtualHeight(1.f / static_cast<f32>(dwVirtualHeight))
        , m_nFileId(0)
        , m_dwSliceCount(dwSliceCount)
        , m_fHorizFOV(2 * gf_PI / dwSliceCount)
        , m_bFlipY(false)
        , m_fTransitionSize(fTransitionSize)
        , m_bMetaData(bMetaData)
    {
        assert(dwWidth);
        assert(dwHeight);

        m_RGB.resize(m_dwWidth * 3 * m_dwHeight);

        // ratio between width and height defines angle 1 (angle from mid to cylinder edges)
        float fVert1Frac = (2 * gf_PI * m_dwHeight) / m_dwWidth;

        // slice count defines angle 2
        float fHorizFrac = tanf(GetHorizFOVWithBorder() * 0.5f);
        float fVert2Frac = 2.0f * fHorizFrac / rEngine.GetRenderer()->GetWidth() * rEngine.GetRenderer()->GetHeight();
        //      float fVert2Frac = 2.0f * fHorizFrac / rEngine.GetRenderer()->GetWidth() * rEngine.GetRenderer()->GetHeight();

        // the bigger one defines the needed angle
        float fVertFrac = max(fVert1Frac, fVert2Frac);

        // planar image becomes a barrel after projection and we need to zoom in to only utilize the usable part (inner rect)
        // this is not always needed - for quality with low slice count we could be save some quality here
        fVertFrac /= cosf(GetHorizFOVWithBorder() * 0.5f);

        // compute FOV from Frac
        float fVertFOV = 2 * atanf(0.5f * fVertFrac);

        m_fPanoramaShotVertFOV = fabsf(fVertFOV);

        CryLog("RenderFov = %f degrees (%f = max(%f,%f)*fix)", RAD2DEG(m_fPanoramaShotVertFOV), fVertFrac, fVert1Frac, fVert2Frac);
        Clear();
    }

    void Clear()
    {
        memset(&m_RGB[0], 0, m_dwWidth * m_dwHeight * 3);
    }

    // szDirectory + "/" + file_id + "." + extension
    // logs errors in the case there are problems
    bool SaveImage(const char* szDirectory)
    {
        assert(szDirectory);

        const char* szExtension = m_rEngine.GetCVars()->e_ScreenShotFileFormat->GetString();

        if (azstricmp(szExtension, "dds") != 0  &&
            azstricmp(szExtension, "tga") != 0    &&
            azstricmp(szExtension, "jpg") != 0)
        {
            gEnv->pLog->LogError("Format e_ScreenShotFileFormat='%s' not supported", szExtension);
            return false;
        }

        const char* sRequestedName = m_rEngine.GetCVars()->e_ScreenShotFileName->GetString();

        char sFileName[AZ_MAX_PATH_LEN];

        if (azstricmp(sRequestedName, "") != 0)
        {
            AZStd::string folderPath;
            AZStd::string fileName;
            AzFramework::StringFunc::Path::Split(sRequestedName, nullptr, &folderPath, &fileName);
            gEnv->pFileIO->CreatePath((AZStd::string("@user@/ScreenShots/") + folderPath).c_str());
            azsnprintf(sFileName, sizeof(sFileName), "@user@/ScreenShots/%s.%s", sRequestedName, szExtension);
        }
        else
        {
            azsnprintf(sFileName, sizeof(sFileName), "@user@/ScreenShots/%s", szDirectory);
            gEnv->pFileIO->CreatePath(sFileName);

            // find free file id
            for (;; )
            {
                azsnprintf(sFileName, sizeof(sFileName), "@user@/ScreenShots/%s/%.5d.%s", szDirectory, m_nFileId, szExtension);

                AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(sFileName, "rb");

                if (fileHandle == AZ::IO::InvalidHandle)
                {
                    break; // file doesn't exist
                }

                gEnv->pCryPak->FClose(fileHandle);
                m_nFileId++;
            }
        }

        bool bOk;

        if (azstricmp(szExtension, "dds") == 0)
        {
            bOk = gEnv->pRenderer->WriteDDS((byte*)&m_RGB[0], m_dwWidth, m_dwHeight, 3, sFileName, eTF_BC3, 1);
        }
        else
        if (azstricmp(szExtension, "tga") == 0)
        {
            bOk = gEnv->pRenderer->WriteTGA((byte*)&m_RGB[0], m_dwWidth, m_dwHeight, sFileName, 24, 24);
        }
        else
        {
            bOk = gEnv->pRenderer->WriteJPG((byte*)&m_RGB[0], m_dwWidth, m_dwHeight, sFileName, 24);
        }

        if (!bOk)
        {
            gEnv->pLog->LogError("Failed to write '%s' (not supported on this platform?)", sFileName);
        }
        else //write meta data
        {
            if (m_bMetaData)
            {
                const f32   fSizeX  =   GetCVars()->e_ScreenShotMapSizeX;
                const f32   fSizeY  =   GetCVars()->e_ScreenShotMapSizeY;
                const f32   fTLX    =   GetCVars()->e_ScreenShotMapCenterX - fSizeX;
                const f32   fTLY    =   GetCVars()->e_ScreenShotMapCenterY - fSizeY;
                const f32   fBRX    =   GetCVars()->e_ScreenShotMapCenterX + fSizeX;
                const f32   fBRY    =   GetCVars()->e_ScreenShotMapCenterY + fSizeY;

                snprintf(sFileName, sizeof(sFileName), "@user@/ScreenShots/%s/%.5d.%s", szDirectory, m_nFileId, "xml");

                AZ::IO::HandleType metaFileHandle = gEnv->pCryPak->FOpen(sFileName, "wt");
                if (metaFileHandle != AZ::IO::InvalidHandle)
                {
                    char sFileData[1024];
                    snprintf(sFileData, sizeof(sFileData), "<MiniMap Filename=\"%.5d.%s\" startX=\"%f\" startY=\"%f\" endX=\"%f\" endY=\"%f\"/>",
                        m_nFileId, szExtension, fTLX, fTLY, fBRX, fBRY);
                    string data(sFileData);
                    gEnv->pCryPak->FWrite(data.c_str(), data.size(), metaFileHandle);
                    gEnv->pCryPak->FClose(metaFileHandle);
                }
            }
        }

        // reset filename when done so user doesn't overwrite other screen shots (unless they want to)
        // this is done here as there is no callback for standard screenshots to allow the user to clear
        // this when done with the screen shot, so I decided to just always clear it when done
        m_rEngine.GetCVars()->e_ScreenShotFileName->Set("");

        return bOk;
    }

    // rasterize rectangle
    // Arguments:
    //   x0 - <x1, including
    //   y0 - <y1, including
    //   x1 - >x0, excluding
    //   y1 - >y0, excluding
    void RasterizeRect(const uint32*   pRGBAImage,
        const uint32    dwWidth,
        const uint32    dwHeight,
        const uint32    dwSliceX,
        const uint32    dwSliceY,
        const f32           fTransitionSize,
        const bool      bFadeBordersX,
        const bool      bFadeBordersY)
    {
        {
            //calculate rect inside the whole image
            const int32 OrgX0 =   static_cast<int32>(static_cast<f32>((dwSliceX * dwWidth) * m_dwWidth) * m_fInvVirtualWidth);
            const int32 OrgY0 =   static_cast<int32>(static_cast<f32>((dwSliceY * dwHeight) * m_dwHeight) * m_fInvVirtualHeight);
            const int32 OrgX1 =   min(static_cast<int32>(static_cast<f32>(((dwSliceX + 1) * dwWidth) * m_dwWidth) * m_fInvVirtualWidth), static_cast<int32>(m_dwWidth)) - (m_rEngine.GetCVars()->e_ScreenShotDebug == 1 ? 1 : 0);
            const int32 OrgY1 =   min(static_cast<int32>(static_cast<f32>(((dwSliceY + 1) * dwHeight) * m_dwHeight) * m_fInvVirtualHeight), static_cast<int32>(m_dwHeight)) - (m_rEngine.GetCVars()->e_ScreenShotDebug == 1 ? 1 : 0);
            //expand bounds for borderblending
            const int32 CenterX   =   (OrgX0 + OrgX1) / 2;
            const int32 CenterY   =   (OrgY0 + OrgY1) / 2;
            const int32 X0    =   static_cast<int32>(static_cast<f32>(OrgX0 - CenterX) * (1.f + fTransitionSize)) + CenterX;
            const int32 Y0    =   static_cast<int32>(static_cast<f32>(OrgY0 - CenterY) * (1.f + fTransitionSize)) + CenterY;
            const int32 X1    =   static_cast<int32>(static_cast<f32>(OrgX1 - CenterX) * (1.f + fTransitionSize)) + CenterX;
            const int32 Y1    =   static_cast<int32>(static_cast<f32>(OrgY1 - CenterY) * (1.f + fTransitionSize)) + CenterY;
            const f32 InvBlendX   =   1.f / max(static_cast<f32>(X1 - OrgX1), 0.01f);//0.5 is here because the border is two times wider then the border of the single segment in total
            const f32 InvBlendY   =   1.f / max(static_cast<f32>(Y1 - OrgY1), 0.01f);
            const int32 DebugScale =   (m_rEngine.GetCVars()->e_ScreenShotDebug == 2) ? 65536 : 0;
            for (int32 y = max(Y0, 0); y < Y1 && y < (int)m_dwHeight; y++)
            {
                const f32 WeightY   =   bFadeBordersY ? min(1.f, static_cast<f32>(min(y - Y0, Y1 - y)) * InvBlendY) : 1.f;
                for (int32 x = max(X0, 0); x < X1 && x < (int)m_dwWidth; x++)
                {
                    uint8* pDst = &m_RGB[m_bFlipY ? 3 * (x + (m_dwHeight - y - 1) * m_dwWidth) : 3 * (x + y * m_dwWidth)];
                    const f32 WeightX =   bFadeBordersX ? min(1.f, static_cast<f32>(min(x - X0, X1 - x)) * InvBlendX) : 1.f;
                    GetBilinearFilteredBlend(static_cast<int32>(static_cast<f32>(x - X0) / static_cast<f32>(X1 - X0) * static_cast<f32>(dwWidth) * 16.f),
                        static_cast<int32>(static_cast<f32>(y - Y0) / static_cast<f32>(Y1 - Y0) * static_cast<f32>(dwHeight) * 16.f),
                        pRGBAImage, dwWidth, dwHeight,
                        max(static_cast<int32>(WeightX * WeightY * 65536.f), DebugScale), pDst);
                }
            }
        }
    }

    void RasterizeCylinder(const uint32* pRGBAImage,
        const uint32 dwWidth,
        const uint32 dwHeight,
        const uint32 dwSlice,
        const bool bFadeBorders)
    {
        float fSrcAngleMin = GetSliceAngle(dwSlice - 1);
        float fFractionVert = tanf(m_fPanoramaShotVertFOV * 0.5f);
        float fFractionHoriz = fFractionVert * gEnv->pRenderer->GetCamera().GetProjRatio();
        float fInvFractionHoriz = 1.0f / fFractionHoriz;

        // for soft transition
        float fFadeOutFov = GetHorizFOVWithBorder();
        float fFadeInFov = GetHorizFOV();

        int x0 = 0, y0 = 0, x1 = m_dwWidth, y1 = m_dwHeight;

        float fScaleX = 1.0f / m_dwWidth;
        float fScaleY = 0.5f * fInvFractionHoriz / (m_dwWidth / (2 * gf_PI)) / dwHeight * dwWidth;                          // this value is not correctly computed yet - but using many slices reduced the problem

        if (m_bFlipY)
        {
            fScaleY = -fScaleY;
        }


        // it's more efficient to process colums than lines
        for (int x = x0; x < x1; ++x)
        {
            uint8* pDst = &m_RGB[3 * (x + y0 * m_dwWidth)];
            float fSrcX = x * fScaleX - 0.5f; // -0.5 .. 0.5
            float fSrcAngleX = fSrcAngleMin + 2 * gf_PI * fSrcX;

            if (fSrcAngleX > gf_PI)
            {
                fSrcAngleX -= 2 * gf_PI;
            }
            if (fSrcAngleX < -gf_PI)
            {
                fSrcAngleX += 2 * gf_PI;
            }

            if (fabs(fSrcAngleX) > fFadeOutFov * 0.5f)
            {
                continue;                                                   // clip away curved parts of the barrel
            }
            float fScrPosX = (tanf(fSrcAngleX) * 0.5f * fInvFractionHoriz + 0.5f) * dwWidth;
            //          float fInvCosSrcX = 1.0f / cos(fSrcAngleX);
            float fInvCosSrcX = 1.0f / cosf(fSrcAngleX);

            if (fScrPosX >= 0 && fScrPosX <= (float)dwWidth) // this is an optimization - but it could be done even more efficient
            {
                if (fInvCosSrcX > 0)                                                  // don't render the viewer opposing direction
                {
                    int iSrcPosX16 = (int)(fScrPosX * 16.0f);

                    float fYOffset = 16 * 0.5f * dwHeight - 16 * 0.5f * m_dwHeight * fScaleY * fInvCosSrcX * dwHeight;
                    float fYMul = 16 * fScaleY * fInvCosSrcX * dwHeight;

                    float fSrcY = y0 * fYMul + fYOffset;

                    uint32 dwLerp64k = 256 * 256 - 1;

                    if (!bFadeBorders)
                    {
                        // first pass - every second image without soft borders
                        for (int y = y0; y < y1; ++y, fSrcY += fYMul, pDst += m_dwWidth * 3)
                        {
                            GetBilinearFiltered(iSrcPosX16, (int)fSrcY, pRGBAImage, dwWidth, dwHeight, pDst);
                        }
                    }
                    else
                    {
                        // second pass - do all the inbetween with soft borders
                        float fOffSlice = fabs(fSrcAngleX / fFadeInFov) - 0.5f;

                        if (fOffSlice < 0)
                        {
                            fOffSlice = 0; // no transition in this area
                        }
                        float fBorder = (fFadeOutFov - fFadeInFov) * 0.5f;

                        if (fBorder < 0.001f)
                        {
                            fBorder = 0.001f; // we do not have border
                        }
                        float fFade = 1.0f - fOffSlice * fFadeInFov / fBorder;

                        if (fFade < 0.0f)
                        {
                            fFade = 0.0f; // don't use this slice here
                        }
                        dwLerp64k = (uint32)(fFade * (256.0f * 256.0f - 1.0f)); // 0..64k

                        if (dwLerp64k)                                              // optimization
                        {
                            for (int y = y0; y < y1; ++y, fSrcY += fYMul, pDst += m_dwWidth * 3)
                            {
                                GetBilinearFilteredBlend(iSrcPosX16, (int)fSrcY, pRGBAImage, dwWidth, dwHeight, dwLerp64k, pDst);
                            }
                        }
                    }
                }
            }
        }
    }

    // fast, rgb only
    static inline ColorB lerp(const ColorB x, const ColorB y, const uint32 a, const uint32 dwBase)
    {
        const int32 b = dwBase - a;
        const int32 RC  =   dwBase / 2;//rounding correction


        return ColorB(((int)x.r * b + (int)y.r * a + RC) / dwBase,
            ((int)x.g * b + (int)y.g * a + RC) / dwBase,
            ((int)x.b * b + (int)y.b * a + RC) / dwBase);
    }

    static inline ColorB Mul(const ColorB x, const int32 a, const int32 dwBase)
    {
        return ColorB(((int)x.r * (int)a) / dwBase,
            ((int)x.g * (int)a) / dwBase,
            ((int)x.b * (int)a) / dwBase);
    }
    static inline ColorB MadSaturate(const ColorB x, const int32 a, const int32 dwBase, const ColorB y)
    {
        const int32 MAX_COLOR   =   0xff;
        const ColorB PreMuled   =   Mul(x, a, dwBase);
        return ColorB(min((int)PreMuled.r + (int)y.r, MAX_COLOR),
            min((int)PreMuled.g + (int)y.g, MAX_COLOR),
            min((int)PreMuled.b + (int)y.b, MAX_COLOR));
    }

    // bilinear filtering in fixpoint,
    // 4bit fractional part -> multiplier 16
    // --lookup outside of the image is not defined
    //  lookups outside the image are now clamped, needed due to some float inaccuracy while rasterizing a rect-screenshot
    // Arguments:
    //   iX16 - fX mul 16
    //   iY16 - fY mul 16
    //   result - [0]=red, [1]=green, [2]=blue
    static inline bool GetBilinearFilteredRaw(const int iX16, const int iY16,
        const uint32* pRGBAImage,
        const uint32 dwWidth, const uint32 dwHeight,
        ColorB& result)
    {
        int iLocalX = min(max(iX16 / 16, 0), static_cast<int>(dwWidth - 1));
        int iLocalY = min(max(iY16 / 16, 0), static_cast<int>(dwHeight - 1));

        int iLerpX = iX16 & 0xf;      // 0..15
        int iLerpY = iY16 & 0xf;      // 0..15

        ColorB colS[4];

        const uint32* pRGBA = &pRGBAImage[iLocalX + iLocalY * dwWidth];

        colS[0] = pRGBA[0];
        colS[1] = pRGBA[1];
        colS[2] = pRGBA[iLocalY + 1uL < dwHeight ? dwWidth : 0];
        colS[3] = pRGBA[(iLocalX + 1uL < dwWidth ? 1 : 0) + (iLocalY + 1uL < dwHeight ? dwWidth : 0)];

        ColorB colTop, colBottom;

        colTop = lerp(colS[0], colS[1], iLerpX, 16);
        colBottom = lerp(colS[2], colS[3], iLerpX, 16);

        result = lerp(colTop, colBottom, iLerpY, 16);
        return true;
    }


    // blend with background
    static inline bool GetBilinearFiltered(const int iX16, const int iY16,
        const uint32* pRGBAImage,
        const uint32 dwWidth, const uint32 dwHeight,
        uint8 result[3])
    {
        ColorB colFiltered;
        if (GetBilinearFilteredRaw(iX16, iY16, pRGBAImage, dwWidth, dwHeight, colFiltered))
        {
            result[0] = colFiltered.r;
            result[1] = colFiltered.g;
            result[2] = colFiltered.b;
            return true;
        }
        return false;
    }

    static inline bool GetBilinearFilteredBlend(const int iX16, const int iY16,
        const uint32* pRGBAImage,
        const uint32 dwWidth, const uint32 dwHeight,
        const uint32 dwLerp64k,
        uint8 result[3])
    {
        ColorB colFiltered;
        if (GetBilinearFilteredRaw(iX16, iY16, pRGBAImage, dwWidth, dwHeight, colFiltered))
        {
            ColorB colRet = lerp(ColorB(result[0], result[1], result[2]), colFiltered, dwLerp64k, 256 * 256);

            result[0] = colRet.r;
            result[1] = colRet.g;
            result[2] = colRet.b;
            return true;
        }
        return false;
    }

    static inline bool GetBilinearFilteredAdd(const int iX16, const int iY16,
        const uint32* pRGBAImage,
        const uint32 dwWidth, const uint32 dwHeight,
        const uint32 dwLerp64k,
        uint8 result[3])
    {
        ColorB colFiltered;
        if (GetBilinearFilteredRaw(iX16, iY16, pRGBAImage, dwWidth, dwHeight, colFiltered))
        {
            ColorB colRet = MadSaturate(colFiltered, dwLerp64k, 256 * 256, ColorB(result[0], result[1], result[2]));

            result[0] = colRet.r;
            result[1] = colRet.g;
            result[2] = colRet.b;
            return true;
        }
        return false;
    }


    float GetSliceAngle(const uint32 dwSlice) const
    {
        uint32 dwAlternatingSlice = (dwSlice * 2) % m_dwSliceCount;

        float fAngleStep = m_fHorizFOV;

        float fRet = fAngleStep * dwAlternatingSlice;

        if (dwSlice * 2 >= m_dwSliceCount)
        {
            fRet += fAngleStep;
        }

        return fRet;
    }

    float GetHorizFOV() const
    {
        return m_fHorizFOV;
    }

    float GetHorizFOVWithBorder() const
    {
        return m_fHorizFOV * (1.0f + m_fTransitionSize);
    }

    void* GetBuffer(){ return &m_RGB[0]; }
    uint32 GetWidth() { return m_dwWidth; }
    uint32 GetHeight() { return m_dwHeight; }


    //private: // -------------------------------------------------------------------

    uint32                      m_dwWidth;                  // >0
    uint32                      m_dwHeight;                 // >0
    f32                         m_fInvWidth;                // >0
    f32                         m_fInvHeight;               // >0
    uint32                      m_dwVirtualWidth;           // >0
    uint32                      m_dwVirtualHeight;          // >0
    f32                         m_fInvVirtualWidth;         // >0
    f32                         m_fInvVirtualHeight;        // >0
    std::vector<uint8>          m_RGB;                      // [channel + x*3 + m_dwWidth*3*y], channel=0..2, x<m_dwWidth, y<m_dwHeight, no alpha channel to occupy less memory
    uint32                      m_nFileId;                  // counts up until it finds free file id
    bool                        m_bFlipY;                   // might be useful for some image formats
    bool                        m_bMetaData;                // output additional metadata

    float                       m_fPanoramaShotVertFOV;     // -1 means not set yet - in radians

private:

    uint32                      m_dwSliceCount;             //
    C3DEngine&                  m_rEngine;                  //
    float                       m_fHorizFOV;                // - in radians
    float                       m_fTransitionSize;          // [0..1], 0=no transition, 1.0=full transition
};
#endif

enum EScreenShotType
{
    ESST_NONE = 0,
    ESST_HIGHRES = 1,
    ESST_PANORAMA,
    ESST_MAP_DELAYED,
    ESST_MAP,
    ESST_SWMAP,
    ESST_SWMAP_DELAYED,
};

void C3DEngine::ScreenshotDispatcher([[maybe_unused]] const int nRenderFlags, [[maybe_unused]] const SRenderingPassInfo& passInfo)
{
#if defined(WIN32) || defined(WIN64) || defined(MAC)
    CStitchedImage*   pStitchedImage = 0;
    const uint32  dwPanWidth          = max(1, GetCVars()->e_ScreenShotWidth);
    const uint32  dwPanHeight         = max(1, GetCVars()->e_ScreenShotHeight);
    const f32         fTransitionSize = min(1.f, abs(GetCVars()->e_ScreenShotQuality) * 0.01f);

    const uint32 widthSlices  = (dwPanWidth  + GetRenderer()->GetWidth()  - 1) / GetRenderer()->GetWidth();
    const uint32 heightSlices = (dwPanHeight + GetRenderer()->GetHeight() - 1) / GetRenderer()->GetHeight();
    uint32 MinSlices    = max(widthSlices, heightSlices);
    MinSlices           = max(MinSlices, (uint32)GetCVars()->e_ScreenShotMinSlices);

    const uint32  dwVirtualWidth  =   GetRenderer()->GetWidth() * MinSlices;
    const uint32  dwVirtualHeight =   GetRenderer()->GetHeight() * MinSlices;

    GetRenderer()->StartScreenShot(GetCVars()->e_ScreenShot);

    switch (abs(GetCVars()->e_ScreenShot))
    {
    case ESST_HIGHRES:
        GetConsole()->ShowConsole(false);

        MinSlices = max(MinSlices, 1u);
        pStitchedImage  =   new CStitchedImage(*this, dwPanWidth, dwPanHeight, dwVirtualWidth, dwVirtualHeight, MinSlices, fTransitionSize);

        ScreenShotHighRes(pStitchedImage, nRenderFlags, passInfo, MinSlices, fTransitionSize);
        pStitchedImage->SaveImage("HiRes");
        pStitchedImage->Clear();    // good for debugging
        delete pStitchedImage;
        if (GetCVars()->e_ScreenShot > 0)     // <0 is used for multiple frames (videos)
        {
            GetCVars()->e_ScreenShot = 0;
        }
        break;
    case ESST_PANORAMA:
        GetConsole()->ShowConsole(false);

        // Panorama screenshots will exhibit artifacts if insufficient slices are used to render them
        // 20 slices yields great quality.
        MinSlices = max(MinSlices, 20u);
        pStitchedImage  =   new CStitchedImage(*this, dwPanWidth, dwPanHeight, dwVirtualWidth, dwVirtualHeight, MinSlices, fTransitionSize);
        
        ScreenShotPanorama(pStitchedImage, nRenderFlags, passInfo, MinSlices, fTransitionSize);
        pStitchedImage->SaveImage("Panorama");
        pStitchedImage->Clear();    // good for debugging
        delete pStitchedImage;
        if (GetCVars()->e_ScreenShot > 0)     // <0 is used for multiple frames (videos)
        {
            GetCVars()->e_ScreenShot = 0;
        }
        break;
    case ESST_MAP_DELAYED:
    {
        GetCVars()->e_ScreenShot  =   sgn(GetCVars()->e_ScreenShot) * ESST_MAP;   // sgn() to keep sign bit , <0 is used for multiple frames (videos)
    }
    break;
    case ESST_SWMAP_DELAYED:
    {
        GetCVars()->e_ScreenShot  =   sgn(GetCVars()->e_ScreenShot) * ESST_SWMAP;     // sgn() to keep sign bit , <0 is used for multiple frames (videos)
    }
    break;
    case ESST_SWMAP:
    case ESST_MAP:
    {
        static const unsigned int nMipMapSnapshotSize = 2048;
        GetRenderer()->ChangeViewport(0, 0, nMipMapSnapshotSize, nMipMapSnapshotSize);
        uint32 TmpHeight, TmpWidth, TmpVirtualHeight, TmpVirtualWidth;
        TmpHeight = TmpWidth = TmpVirtualHeight = TmpVirtualWidth = 1;

        while ((TmpHeight << 1) <= dwPanHeight)
        {
            TmpHeight <<= 1;
        }
        while ((TmpWidth << 1) <= dwPanWidth)
        {
            TmpWidth <<= 1;
        }
        const uint32  TmpMinSlices                =   max(max(1, GetCVars()->e_ScreenShotMinSlices),
                max(static_cast<int>((TmpWidth + nMipMapSnapshotSize - 1) / nMipMapSnapshotSize),
                    static_cast<int>((TmpHeight + nMipMapSnapshotSize - 1) / nMipMapSnapshotSize)));
        while ((TmpVirtualHeight << 1) <= TmpMinSlices * nMipMapSnapshotSize)
        {
            TmpVirtualHeight <<= 1;
        }
        while ((TmpVirtualWidth << 1) <= TmpMinSlices * nMipMapSnapshotSize)
        {
            TmpVirtualWidth <<= 1;
        }

        GetConsole()->ShowConsole(false);
        pStitchedImage    =   new CStitchedImage(*this, TmpWidth, TmpHeight, TmpVirtualWidth, TmpVirtualHeight, TmpMinSlices, fTransitionSize, true);
        ScreenShotMap(pStitchedImage, nRenderFlags, passInfo, TmpMinSlices, fTransitionSize);
        if (abs(GetCVars()->e_ScreenShot) == ESST_MAP)
        {
            pStitchedImage->SaveImage("Map");
        }

        if (m_pScreenshotCallback)
        {
            const f32   fSizeX  =   GetCVars()->e_ScreenShotMapSizeX;
            const f32   fSizeY  =   GetCVars()->e_ScreenShotMapSizeY;
            const f32   fTLX    =   GetCVars()->e_ScreenShotMapCenterX - fSizeX;
            const f32   fTLY    =   GetCVars()->e_ScreenShotMapCenterY - fSizeY;
            const f32   fBRX    =   GetCVars()->e_ScreenShotMapCenterX + fSizeX;
            const f32   fBRY    =   GetCVars()->e_ScreenShotMapCenterY + fSizeY;

            m_pScreenshotCallback->SendParameters(pStitchedImage->GetBuffer(), pStitchedImage->GetWidth(), pStitchedImage->GetHeight(), fTLX, fTLY, fBRX, fBRY);
        }

        pStitchedImage->Clear();    // good for debugging
        delete pStitchedImage;
    }
        if (GetCVars()->e_ScreenShot > 0)     // <0 is used for multiple frames (videos)
        {
            GetCVars()->e_ScreenShot = 0;
        }

        break;
    default:
        GetCVars()->e_ScreenShot = 0;
    }

    GetRenderer()->EndScreenShot(GetCVars()->e_ScreenShot);

#endif //#if defined(WIN32) || defined(WIN64)
}



struct SDebugFrustrum
{
    Vec3                      m_vPos[8];
    const char*      m_szName;
    CTimeValue            m_TimeStamp;
    ColorB                    m_Color;
    float                     m_fQuadDist;      // < 0 if not used
};

static StaticInstance<std::vector<SDebugFrustrum>> g_DebugFrustrums;

void C3DEngine::DebugDraw_Draw()
{
#ifndef _RELEASE
    if (m_DebugDrawListMgr.IsEnabled())
    {
        m_DebugDrawListMgr.Update();
    }

    CTimeValue CurrentTime = gEnv->pTimer->GetFrameStartTime();

    IRenderAuxGeom* pAux = GetRenderer()->GetIRenderAuxGeom();

    SAuxGeomRenderFlags   oldFlags = pAux->GetRenderFlags();
    SAuxGeomRenderFlags   newFlags;
    newFlags.SetAlphaBlendMode(e_AlphaBlended);
    newFlags.SetCullMode(e_CullModeNone);
    newFlags.SetDepthWriteFlag(e_DepthWriteOff);
    pAux->SetRenderFlags(newFlags);
    std::vector<SDebugFrustrum>::iterator it;

    for (it = g_DebugFrustrums.begin(); it != g_DebugFrustrums.end(); )
    {
        SDebugFrustrum& ref = *it;

        float fRatio = (CurrentTime - ref.m_TimeStamp).GetSeconds() * 2.0f;

        if (fRatio > 1.0f)
        {
            it = g_DebugFrustrums.erase(it);
            continue;
        }

        vtx_idx pnInd[8] = {    0, 4, 1, 5, 2, 6, 3, 7    };

        float fRadius = ((ref.m_vPos[0] + ref.m_vPos[1] + ref.m_vPos[2] + ref.m_vPos[3]) - (ref.m_vPos[4] + ref.m_vPos[5] + ref.m_vPos[6] + ref.m_vPos[7])).GetLength() * 0.25f;
        float fDistance = min(fRadius, 33.0f);  // in meters

        float fRenderRatio = fRatio * fDistance / fRadius;

        if (ref.m_fQuadDist > 0)
        {
            fRenderRatio = ref.m_fQuadDist / fRadius;
        }

        Vec3 vPos[4];

        for (uint32 i = 0; i < 4; ++i)
        {
            vPos[i] = ref.m_vPos[i] * fRenderRatio + ref.m_vPos[i + 4] * (1.0f - fRenderRatio);
        }

        Vec3 vMid = (vPos[0] + vPos[1] + vPos[2] + vPos[3]) * 0.25f;

        ColorB col = ref.m_Color;

        if (ref.m_fQuadDist <= 0)
        {
            for (uint32 i = 0; i < 4; ++i)
            {
                vPos[i] = vPos[i] * 0.95f + vMid * 0.05f;
            }

            // quad
            if (ref.m_fQuadDist != -999.f)
            {
                pAux->DrawTriangle(vPos[0], col, vPos[2], col, vPos[1], col);
                pAux->DrawTriangle(vPos[2], col, vPos[0], col, vPos[3], col);
            }
            // projection lines
            pAux->DrawLines(ref.m_vPos, 8, pnInd, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
            pAux->DrawLines(ref.m_vPos, 8, pnInd + 2, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
            pAux->DrawLines(ref.m_vPos, 8, pnInd + 4, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
            pAux->DrawLines(ref.m_vPos, 8, pnInd + 6, 2, RGBA8(0xff, 0xff, 0x1f, 0xff));
        }
        else
        {
            // rectangle
            pAux->DrawPolyline(vPos, 4, true, RGBA8(0xff, 0xff, 0x1f, 0xff));
        }

        ++it;
    }

    pAux->SetRenderFlags(oldFlags);


    if (GetCVars()->e_DebugDraw == 16)
    {
        DebugDraw_UpdateDebugNode();
    }
    else
    {
        GetRenderer()->SetDebugRenderNode(NULL);
    }

#endif //_RELEASE
}

void C3DEngine::DebugDraw_UpdateDebugNode()
{
#ifndef _RELEASE


#endif //_RELEASE
}

void C3DEngine::RenderWorld(const int nRenderFlags, const SRenderingPassInfo& passInfo, const char* szDebugName)
{
    AZ_TRACE_METHOD();

    if (nRenderFlags & SHDF_ALLOW_AO)
    {
        SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::OnFrameStart, passInfo);
    }

    if (m_szLevelFolder[0] != 0)
    {
        m_nFramesSinceLevelStart++;
    }

    assert(szDebugName);

    if (!GetCVars()->e_Render)
    {
        return;
    }

    IF (!m_bEditor && (m_bInShutDown || m_bInUnload) && !GetRenderer()->IsPost3DRendererEnabled(), 0)
    {
        // Do not render during shutdown/unloading (should never reach here, unless something wrong with game/editor code)
        return;
    }

    FUNCTION_PROFILER_3DENGINE;

    if (GetCVars()->e_ScreenShot)
    {
        ScreenshotDispatcher(nRenderFlags, passInfo);
        // screenshots can mess up the frame ids, be safe and recreate the rendering passinfo object after a screenshot
        const_cast<SRenderingPassInfo&>(passInfo) = SRenderingPassInfo::CreateGeneralPassRenderingInfo(passInfo.GetCamera());
    }

    if (GetCVars()->e_DefaultMaterial)
    {
        _smart_ptr<IMaterial> pMat = GetMaterialManager()->LoadMaterial("Materials/material_default");
        _smart_ptr<IMaterial> pTerrainMat = GetMaterialManager()->LoadMaterial("Materials/material_terrain_default");
        GetRenderer()->SetDefaultMaterials(pMat, pTerrainMat);
    }
    else
    {
        GetRenderer()->SetDefaultMaterials(NULL, NULL);
    }

    // skip rendering if camera is invalid
    if (IsCameraAnd3DEngineInvalid(passInfo, szDebugName))
    {
        return;
    }

    // this will also set the camera in passInfo for the General Pass (done here to support e_camerafreeze)
    UpdateRenderingCamera(szDebugName, passInfo);

    RenderInternal(nRenderFlags, passInfo, szDebugName);

#if !defined(_RELEASE)
    PrintDebugInfo(passInfo);
#endif
}

void C3DEngine::RenderInternal(const int nRenderFlags, const SRenderingPassInfo& passInfo, [[maybe_unused]] const char* szDebugName)
{
    assert(m_pObjManager);


    if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
    {
        GetRenderer()->EF_EndEf3D(
            IsShadersSyncLoad() ? (nRenderFlags | SHDF_NOASYNC | SHDF_STREAM_SYNC) : nRenderFlags,
            GetObjManager()->GetUpdateStreamingPrioriryRoundId(),
            GetObjManager()->GetUpdateStreamingPrioriryRoundIdFast(),
            passInfo);
    }
    else
    {
        UpdatePreRender(passInfo);
        RenderScene(nRenderFlags, passInfo);
        UpdatePostRender(passInfo);
    }
}


void C3DEngine::PreWorldStreamUpdate(const CCamera& cam)
{
    if (m_szLevelFolder[0] != 0)
    {
        m_nStreamingFramesSinceLevelStart++;
    }

    // force preload terrain data if camera was teleported more than 32 meters
    if (!IsAreaActivationInUse() || m_bLayersActivated)
    {
        float fDistance = m_vPrevMainFrameCamPos.GetDistance(cam.GetPosition());

        if (m_vPrevMainFrameCamPos != Vec3(-1000000.f, -1000000.f, -1000000.f))
        {
            m_vAverageCameraMoveDir = m_vAverageCameraMoveDir * .75f + (cam.GetPosition() - m_vPrevMainFrameCamPos) / max(0.01f, GetTimer()->GetFrameTime()) * .25f;
            if (m_vAverageCameraMoveDir.GetLength() > 10.f)
            {
                m_vAverageCameraMoveDir.SetLength(10.f);
            }

            float fNewSpeed = fDistance / max(0.001f, gEnv->pTimer->GetFrameTime());
            if (fNewSpeed > m_fAverageCameraSpeed)
            {
                m_fAverageCameraSpeed = fNewSpeed * .20f + m_fAverageCameraSpeed * .80f;
            }
            else
            {
                m_fAverageCameraSpeed = fNewSpeed * .02f + m_fAverageCameraSpeed * .98f;
            }
            m_fAverageCameraSpeed = CLAMP(m_fAverageCameraSpeed, 0, 10.f);
        }

        // Adjust streaming mip bias based on camera speed and depending on installed on HDD or not
        bool bStreamingFromHDD = gEnv->pSystem->GetStreamEngine()->IsStreamDataOnHDD();
        if (GetCVars()->e_StreamAutoMipFactorSpeedThreshold)
        {
            if (m_fAverageCameraSpeed > GetCVars()->e_StreamAutoMipFactorSpeedThreshold)
            {
                GetRenderer()->SetTexturesStreamingGlobalMipFactor(bStreamingFromHDD ? GetCVars()->e_StreamAutoMipFactorMax * .5f : GetCVars()->e_StreamAutoMipFactorMax);
            }
            else
            {
                GetRenderer()->SetTexturesStreamingGlobalMipFactor(bStreamingFromHDD ? GetCVars()->e_StreamAutoMipFactorMin * .5f : GetCVars()->e_StreamAutoMipFactorMin);
            }
        }
        else
        {
            if (bStreamingFromHDD)
            {
                GetRenderer()->SetTexturesStreamingGlobalMipFactor(0);
            }
            else
            {
                GetRenderer()->SetTexturesStreamingGlobalMipFactor(GetCVars()->e_StreamAutoMipFactorMaxDVD);
            }
        }

        if (GetCVars()->e_AutoPrecacheCameraJumpDist && fDistance > GetCVars()->e_AutoPrecacheCameraJumpDist)
        {
            m_bContentPrecacheRequested = true;

            // Invalidate existing precache info
            m_pObjManager->IncrementUpdateStreamingPrioriryRoundIdFast(8);
            m_pObjManager->IncrementUpdateStreamingPrioriryRoundId(8);
        }

        m_vPrevMainFrameCamPos = cam.GetPosition();
    }
}

void C3DEngine::WorldStreamUpdate()
{
#if defined(STREAMENGINE_ENABLE_STATS)
    static uint32 nCurrentRequestCount = 0;
    static uint64 nCurrentBytesRead = 0;
    if (m_nStreamingFramesSinceLevelStart == 1)
    {
        // store current streaming stats
        SStreamEngineStatistics& fullStats = gEnv->pSystem->GetStreamEngine()->GetStreamingStatistics();
        nCurrentBytesRead = fullStats.nTotalBytesRead;
        nCurrentRequestCount = fullStats.nTotalRequestCount;
    }
#endif

    static float fTestStartTime = 0;
    if (m_nStreamingFramesSinceLevelStart == 1)
    {
        fTestStartTime = GetCurAsyncTimeSec();
        gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_FIRST_FRAME, 0, 0);
    }

    // Simple streaming performance test: Wait until all startup texture streaming jobs finish and print a message
    if (!m_bEditor)
    {
        if (!m_bPreCacheEndEventSent)
        {
            IStreamEngine* pSE = gEnv->pSystem->GetStreamEngine();
            SStreamEngineOpenStats openStats;
            pSE->GetStreamingOpenStatistics(openStats);
            bool bStarted =
                (openStats.nOpenRequestCountByType[eStreamTaskTypeTexture] > 0) ||
                (openStats.nOpenRequestCountByType[eStreamTaskTypeGeometry] > 0);

            float fTime = GetCurAsyncTimeSec() - fTestStartTime;

            switch (m_nStreamingFramesSinceLevelStart)
            {
            case 1:
                pSE->PauseStreaming(true, (1 << eStreamTaskTypeTexture) | (1 << eStreamTaskTypeGeometry));
                break;
            case 4:
                pSE->PauseStreaming(false, (1 << eStreamTaskTypeGeometry));
                break;
            case 8:
                pSE->PauseStreaming(false, (1 << eStreamTaskTypeTexture));
                break;
            }

            int nGlobalSystemState = gEnv->pSystem->GetSystemGlobalState();

            if ((nGlobalSystemState != ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE && (!bStarted || fTime >= 10.0f)) && m_nStreamingFramesSinceLevelStart > 16)
            {
                gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE);

                if (!bStarted)
                {
                    PrintMessage("Textures startup streaming finished in %.1f sec", fTime);
                }
                else
                {
                    PrintMessage("Textures startup streaming timed out after %.1f sec", fTime);
                }

                m_fTimeStateStarted = fTime;
            }

            if (nGlobalSystemState == ESYSTEM_GLOBAL_STATE_LEVEL_LOAD_COMPLETE && (fTime - m_fTimeStateStarted) > 0.4f)
            {
                pSE->PauseStreaming(false, (1 << eStreamTaskTypeTexture) | (1 << eStreamTaskTypeGeometry));

                m_bPreCacheEndEventSent = true;
                gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_RUNNING);
                gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_END, 0, 0);

                fTestStartTime = 0.f;

#if defined(STREAMENGINE_ENABLE_STATS)
                SStreamEngineStatistics& fullStats = pSE->GetStreamingStatistics();
                uint64 nBytesRead = fullStats.nTotalBytesRead - nCurrentBytesRead;
                uint32 nRequestCount = fullStats.nTotalRequestCount - nCurrentRequestCount;

                uint32 nOverallFileReadKB = (uint32)(nBytesRead / 1024);
                uint32 nOverallFileReadNum = nRequestCount;
                uint32 nBlockSize = (uint32)(nBytesRead / max((uint32)1, nRequestCount));
                float fReadBandwidthMB = (float)fullStats.nTotalSessionReadBandwidth / (1024 * 1024);

                PrintMessage("Average block size: %d KB, Average throughput: %.1f MB/sec, Jobs processed: %d (%.1f MB), File IO Bandwidth: %.2fMB/s",
                    (nBlockSize) / 1024, (float)(nOverallFileReadKB / max(fTime, 1.f)) / 1024.f,
                    nOverallFileReadNum, (float)nOverallFileReadKB / 1024.f,
                    fReadBandwidthMB);

                if (GetCVars()->e_StreamSaveStartupResultsIntoXML)
                {
                    const char* testResultsFile = "@usercache@/TestResults/Streaming_Level_Start_Throughput.xml";

                    AZ::IO::HandleType resultsFile = gEnv->pCryPak->FOpen(testResultsFile, "wb");
                    if (resultsFile != AZ::IO::InvalidHandle)
                    {
                        AZ::IO::Print(resultsFile,
                            "<phase name=\"Streaming_Level_Start_Throughput\">\n"
                            "<metrics name=\"Streaming\">\n"
                            "<metric name=\"Duration_Sec\" value=\"%.1f\"/>\n"
                            "<metric name=\"BlockSize_KB\" value=\"%d\"/>\n"
                            "<metric name=\"Throughput_MB_Sec\" value=\"%.1f\"/>\n"
                            "<metric name=\"Jobs_Num\" value=\"%d\"/>\n"
                            "<metric name=\"Read_MB\" value=\"%.1f\"/>\n"
                            "</metrics>\n"
                            "</phase>\n",
                            fTime,
                            (nOverallFileReadKB / nOverallFileReadNum),
                            (float)nOverallFileReadKB / max(fTime, 1.f) / 1024.f,
                            nOverallFileReadNum,
                            (float)nOverallFileReadKB / 1024.f);
                        gEnv->pCryPak->FClose(resultsFile);
                    }
                }
#endif
                // gEnv->pCryPak->GetFileReadSequencer()->EndSection(); // STREAMING
            }
            else if (m_szLevelFolder[0])
            {
                ProposeContentPrecache();
            }
        }
    }
    else
    {
        if (!m_bPreCacheEndEventSent && m_nStreamingFramesSinceLevelStart == 4)
        {
            m_bPreCacheEndEventSent = true;
            gEnv->pSystem->SetSystemGlobalState(ESYSTEM_GLOBAL_STATE_RUNNING);
            gEnv->pSystem->GetISystemEventDispatcher()->OnSystemEvent(ESYSTEM_EVENT_LEVEL_PRECACHE_END, 0, 0);
        }
    }
}

void C3DEngine::PrintDebugInfo(const SRenderingPassInfo& passInfo)
{
    if (GetCVars()->e_DebugDraw)
    {
        f32 fColor[4] = {1, 1, 0, 1};

        float fYLine = 8.0f, fYStep = 20.0f;

        GetRenderer()->Draw2dLabel(8.0f, fYLine += fYStep, 2.0f, fColor, false, "e_DebugDraw = %d", GetCVars()->e_DebugDraw);

        const char* szMode = "";

        switch (static_cast<int>(GetCVars()->e_DebugDraw))
        {
        case  -1:
            szMode = "Showing bounding boxes";
            break;
        case  1:
            szMode = "bounding boxes, name of the used cgf, polycount, used LOD";
            break;
        case  -2:
        case  2:
            szMode = "color coded polygon count(red,yellow,green,turqoise, blue)";
            break;
        case  -3:
            szMode = "show color coded LODs count, flashing color indicates LOD.";
            break;
        case  3:
            szMode = "show color coded LODs count, flashing color indicates LOD.\nFormat: (Current LOD [Min LOD; Max LOD] (LOD Ratio / Distance to camera)";
            break;
        case  -4:
        case  4:
            szMode = "object texture memory usage in KB";
            break;
        case  -5:
        case  5:
            szMode = "number of render materials (color coded)";
            break;
        case  6:
            szMode = "ambient color (R,G,B,A)";
            break;
        case  7:
            szMode = "triangle count, number of render materials, texture memory in KB";
            break;
        case  8:
            szMode = "Free slot";
            break;
        case  9:
            szMode = "Free slot";
            break;
        case 10:
            szMode = "Deprecated option, use \"r_showlines 2\" instead";
            break;
        case 11:
            szMode = "Free slot";
            break;
        case 12:
            szMode = "Free slot";
            break;
        case 13:
            szMode = "occlusion amount (used during AO computations)";
            break;
        //          case 14:    szMode="";break;
        case 15:
            szMode = "display helpers";
            break;
        case 16:
            szMode = "Debug Gun";
            break;
        case 17:
            szMode = "streaming: buffer sizes (black: geometry, blue: texture)";
            if (gEnv->pLocalMemoryUsage)
            {
                gEnv->pLocalMemoryUsage->OnRender(GetRenderer(), &passInfo.GetCamera());
            }
            break;
        case 18:
            szMode = "Free slot";
            break;
        case 19:
            szMode = "physics proxy triangle count";
            break;
        case 20:
            szMode = "Character attachments texture memory usage";
            break;
        case 21:
            szMode = "Display animated objects distance to camera";
            break;
        case -22:
        case 22:
            szMode = "object's current LOD vertex count";
            break;
        case 23:
            szMode = "Display shadow casters in red";
            break;
        case 24:
            szMode = "Objects without LODs.\n    name - (triangle count)\n    draw calls - zpass/general/transparent/shadows/misc";
            break;
        case 25:
            szMode = "Objects without LODs (Red). Objects that need more LODs (Blue)\n    name - (triangle count)\n    draw calls - zpass/general/transparent/shadows/misc";
            break;

        default:
            assert(0);
        }

        GetRenderer()->Draw2dLabel(8.0f, fYLine += fYStep, 2.0f, fColor, false, "   %s", szMode);

        if (GetCVars()->e_DebugDraw == 17)
        {
            GetRenderer()->Draw2dLabel(8.0f, fYLine += fYStep, 2.0f, fColor, false, "   StatObj geometry used: %.2fMb / %dMb", CObjManager::s_nLastStreamingMemoryUsage / (1024.f * 1024.f), GetCVars()->e_StreamCgfPoolSize);

            ICVar* cVar = GetConsole()->GetCVar("r_TexturesStreaming");
            if (!cVar || !cVar->GetIVal())
            {
                GetRenderer()->Draw2dLabel(8.0f, fYLine += fYStep, 2.0f, fColor, false, "   You have to set r_TexturesStreaming = 1 to see texture information!");
            }
        }
    }

    float fTextPosX = 10, fTextPosY = 10, fTextStepY = 12;

    // print list of streamed meshes
    if (m_pObjManager && GetCVars()->e_StreamCgf && GetCVars()->e_StreamCgfDebug >= 3)
    {
        // overall status
        {
            static char szCGFStreaming[256] = "";
            static SObjectsStreamingStatus objectsStreamingStatus = {0};

            {
                m_pObjManager->GetObjectsStreamingStatus(objectsStreamingStatus);
                sprintf_s(szCGFStreaming, 256, "CgfStrm: Loaded:%d InProg:%d All:%d Act:%d MemUsed:%2.2f MemReq:%2.2f Pool:%d",
                    objectsStreamingStatus.nReady, objectsStreamingStatus.nInProgress, objectsStreamingStatus.nTotal, objectsStreamingStatus.nActive, float(objectsStreamingStatus.nAllocatedBytes) / 1024 / 1024, float(objectsStreamingStatus.nMemRequired) / 1024 / 1024, GetCVars()->e_StreamCgfPoolSize);
            }

            bool bOutOfMem((float(objectsStreamingStatus.nMemRequired) / 1024 / 1024) > GetCVars()->e_StreamCgfPoolSize);
            bool bCloseToOutOfMem((float(objectsStreamingStatus.nMemRequired) / 1024 / 1024) > GetCVars()->e_StreamCgfPoolSize * 90 / 100);

            ColorF color = Col_White;
            if (bOutOfMem)
            {
                color = Col_Red;
            }
            else if (bCloseToOutOfMem)
            {
                color = Col_Orange;
            }

            DrawTextLeftAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, color, szCGFStreaming);
            fTextPosY += fTextStepY;
        }

        DrawTextLeftAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_White, "------------------- List of meshes bigger than %d KB -------------------", GetCVars()->e_StreamCgfDebugMinObjSize);

        for (int nObjId = 0; nObjId < m_pObjManager->GetArrStreamableObjects().Count(); nObjId++)
        {
            CStatObj* pStatObj = (CStatObj*)m_pObjManager->GetArrStreamableObjects()[nObjId].GetStreamAbleObject();

            int nKB = pStatObj->GetStreamableContentMemoryUsage() >> 10;
            int nSel = (pStatObj->m_nSelectedFrameId >= passInfo.GetMainFrameID() - 2);

            string sName;
            pStatObj->GetStreamableName(sName);

            if ((nKB >= GetCVars()->e_StreamCgfDebugMinObjSize && strstr(sName.c_str(), GetCVars()->e_StreamCgfDebugFilter->GetString())) || nSel)
            {
                const char* pComment = 0;

                if (!pStatObj->m_bCanUnload)
                {
                    pComment = "NO_STRM";
                }
                else if (pStatObj->m_pLod0)
                {
                    pComment = "  LOD_X";
                }
                else if (!pStatObj->m_bLodsAreLoadedFromSeparateFile && pStatObj->m_nLoadedLodsNum > 1)
                {
                    pComment = " SINGLE";
                }
                else if (pStatObj->m_nLoadedLodsNum > 1)
                {
                    pComment = "  LOD_0";
                }
                else
                {
                    pComment = "NO_LODS";
                }

                int nDiff = SATURATEB(int(float(nKB - GetCVars()->e_StreamCgfDebugMinObjSize) / max(1, (int)GetCVars()->e_StreamCgfDebugMinObjSize) * 255));
                ColorB col(nDiff, 255 - nDiff, 0, 255);
                if (nSel && (1 & (int)(GetCurTimeSec() * 5.f)))
                {
                    col = Col_Yellow;
                }
                ColorF fColor(col[0] / 255.f, col[1] / 255.f, col[2] / 255.f, col[3] / 255.f);

                const char* pStatusText = "Unload";
                if (pStatObj->m_eStreamingStatus == ecss_Ready)
                {
                    pStatusText = "Ready ";
                }
                else if (pStatObj->m_eStreamingStatus == ecss_InProgress)
                {
                    pStatusText = "InProg";
                }

                DrawTextLeftAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, fColor, "%1.2f mb, %s, %s, %s",
                    1.f / 1024.f * nKB, pComment, pStatusText, sName.c_str());

                if (fTextPosY > (float)gEnv->pRenderer->GetHeight())
                {
                    break;
                }
            }
        }
    }

    if (m_arrProcessStreamingLatencyTestResults.Count())
    {
        float fAverTime = 0;
        for (int i = 0; i < m_arrProcessStreamingLatencyTestResults.Count(); i++)
        {
            fAverTime += m_arrProcessStreamingLatencyTestResults[i];
        }
        fAverTime /= m_arrProcessStreamingLatencyTestResults.Count();

        int nAverTexNum = 0;
        for (int i = 0; i < m_arrProcessStreamingLatencyTexNum.Count(); i++)
        {
            nAverTexNum += m_arrProcessStreamingLatencyTexNum[i];
        }
        nAverTexNum /= m_arrProcessStreamingLatencyTexNum.Count();

        DrawTextLeftAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_Yellow, "------ SQT Average Time = %.1f, TexNum = %d ------",  fAverTime, nAverTexNum);

        for (int i = 0; i < m_arrProcessStreamingLatencyTestResults.Count(); i++)
        {
            DrawTextLeftAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_Yellow, "Run %d: Time = %.1f, TexNum = %d",
                i, m_arrProcessStreamingLatencyTestResults[i], m_arrProcessStreamingLatencyTexNum[i]);
        }
    }

#if defined(USE_GEOM_CACHES)
#ifndef _RELEASE
    if (GetCVars()->e_GeomCacheDebug)
    {
        m_pGeomCacheManager->DrawDebugInfo();
    }
    else
    {
        m_pGeomCacheManager->ResetDebugInfo();
    }
#endif
#endif
}

void C3DEngine::UpdatePreRender(const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    FUNCTION_PROFILER(GetISystem(), PROFILE_3DENGINE);

    assert(passInfo.IsGeneralPass());

    // Compute global shadow cascade parameters.
    {
        m_fGsmRange = GetCVars()->e_GsmRange;
        m_fGsmRangeStep = GetCVars()->e_GsmRangeStep;

        //!!!also formulas for computing biases per gsm needs to be changed
        m_fShadowsConstBias = GetCVars()->e_ShadowsConstBias;
        m_fShadowsSlopeBias = GetCVars()->e_ShadowsSlopeBias;

        if (m_eShadowMode == ESM_HIGHQUALITY)
        {
            m_fGsmRange = min(0.15f, GetCVars()->e_GsmRange);
            m_fGsmRangeStep = min(2.8f, GetCVars()->e_GsmRangeStep);

            m_fShadowsConstBias = min(GetCVars()->e_ShadowsConstBiasHQ, GetCVars()->e_ShadowsConstBias);
            m_fShadowsSlopeBias = min(GetCVars()->e_ShadowsSlopeBiasHQ, GetCVars()->e_ShadowsSlopeBias);
        }

        const int nCascadeCount = Get3DEngine()->GetShadowsCascadeCount(NULL);
        m_pObjManager->SetGSMMaxDistance(Get3DEngine()->m_fGsmRange * powf(Get3DEngine()->m_fGsmRangeStep, (float)nCascadeCount));
    }

    // (bethelz) This has to happen before particle updates.
    m_PhysicsAreaUpdates.Update();

    if (passInfo.RenderClouds())
    {
        if (m_pCloudsManager)
        {
            m_pCloudsManager->MoveClouds();
        }

        CVolumeObjectRenderNode::MoveVolumeObjects();
    }

    UpdateSun(passInfo);

    // Set traceable fog volume areas
    CFogVolumeRenderNode::SetTraceableArea(AABB(passInfo.GetCamera().GetPosition(), 1024.0f), passInfo);
}

void C3DEngine::UpdatePostRender(const SRenderingPassInfo& passInfo)
{
    AZ_TRACE_METHOD();
    FUNCTION_PROFILER(GetISystem(), PROFILE_3DENGINE);

    assert (m_pObjManager);

    m_pObjManager->CheckTextureReadyFlag();
    if (GetCVars()->e_StreamCgf)
    {
        static Array2d<int> memUsage;

        int nArrayDim = 256;
#ifndef CONSOLE_CONST_CVAR_MODE
        if (GetCVars()->e_StreamCgfDebugHeatMap == 1)
        {
            memUsage.Allocate(nArrayDim);
            CCamera camOld = passInfo.GetCamera();

            PrintMessage("Computing mesh streaming heat map");

            //The assumption is that this is called on Main Thread, otherwise the loop
            //Should be wrapped inside a EnumerateHandlers lambda.
            auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
            const float defaultTerrainHeight = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();

            const AZ::Aabb terrainAabb = terrain ? terrain->GetTerrainAabb() : AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
            const int nTerrainSizeX = static_cast<int>(terrainAabb.GetXExtent());
            const int nTerrainSizeY = static_cast<int>(terrainAabb.GetYExtent());
            const int nStepX = nTerrainSizeX / nArrayDim;
            const int nStepY = nTerrainSizeY / nArrayDim;

            for (int x = 0; x < nTerrainSizeX; x += nStepX)
            {
                for (int y = 0; y < nTerrainSizeY; y += nStepY)
                {
                    CCamera camTmp = camOld;
                    float terrainHeight = terrain ? terrain->GetHeightFromFloats((float)x, (float)y) : defaultTerrainHeight;
                    camTmp.SetPosition(Vec3((float)x + (float)nStepX / 2.f, (float)y + (float)nStepY / 2.f, terrainHeight));
                    //SetCamera(camTmp);
                    m_pObjManager->ProcessObjectsStreaming(passInfo);

                    SObjectsStreamingStatus objectsStreamingStatus;
                    m_pObjManager->GetObjectsStreamingStatus(objectsStreamingStatus);

                    memUsage[x / nStepX][y / nStepY] = objectsStreamingStatus.nMemRequired;
                }

                if (!((x / nStepX) & 31))
                {
                    PrintMessage(" working ...");
                }
            }

            PrintMessage(" done");

            GetCVars()->e_StreamCgfDebugHeatMap = 2;
            //SetCamera(camOld);
        }
        else if (GetCVars()->e_StreamCgfDebugHeatMap == 2)
        {
            auto terrain = AzFramework::Terrain::TerrainDataRequestBus::FindFirstHandler();
            const float defaultTerrainHeight = AzFramework::Terrain::TerrainDataRequests::GetDefaultTerrainHeight();

            const AZ::Aabb terrainAabb = terrain ? terrain->GetTerrainAabb() : AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
            const float terrainSizeX = terrainAabb.GetXExtent();
            const float terrainSizeY = terrainAabb.GetYExtent();
            const float fStepX = terrainSizeX / nArrayDim;
            const float fStepY = terrainSizeY / nArrayDim;

            for (int x = 0; x < memUsage.GetSize(); x++)
            {
                for (int y = 0; y < memUsage.GetSize(); y++)
                {
                    float terrainHeight = terrain ? terrain->GetHeightFromFloats((float)x * fStepX, (float)y * fStepY) : defaultTerrainHeight;
                    Vec3 v0((float)x* fStepX,       (float)y* fStepY,       terrainHeight);
                    Vec3 v1((float)x* fStepX + fStepX, (float)y* fStepY + fStepY, v0.z + fStepX);
                    v0 += Vec3(.25f, .25f, .25f);
                    v1 -= Vec3(.25f, .25f, .25f);
                    AABB box(v0, v1);
                    if (!passInfo.GetCamera().IsAABBVisible_F(box))
                    {
                        continue;
                    }

                    int nMemUsageMB = memUsage[(int)(x)][(int)(y)] / 1024 / 1024;

                    int nOverLoad = nMemUsageMB - GetCVars()->e_StreamCgfPoolSize;

                    ColorB col = Col_Red;

                    if (nOverLoad < GetCVars()->e_StreamCgfPoolSize / 2)
                    {
                        col = Col_Yellow;
                    }

                    if (nOverLoad < 0)
                    {
                        col = Col_Green;
                    }

                    DrawBBox(box, col);
                }
            }
        }
#endif //CONSOLE_CONST_CVAR_MODE
        m_pObjManager->ProcessObjectsStreaming(passInfo);
    }
    else
    {
        m_pObjManager->GetStreamPreCacheCameras()[0].vPosition = passInfo.GetCamera().GetPosition();
        if (Distance::Point_AABBSq(m_pObjManager->GetStreamPreCacheCameras()[0].vPosition, m_pObjManager->GetStreamPreCacheCameras()[0].bbox) > 0.0f)
        {
            m_pObjManager->GetStreamPreCacheCameras()[0].bbox = AABB(m_pObjManager->GetStreamPreCacheCameras()[0].vPosition, GetCVars()->e_StreamPredictionBoxRadius);
        }
        m_pObjManager->UpdateObjectsStreamingPriority(false, passInfo);
    }

    // (bethelz) Per-frame precache request handled by streaming systems.
    m_bContentPrecacheRequested = false;
}

int __cdecl C3DEngine__Cmp_SRNInfo(const void* v1, const void* v2)
{
    SRNInfo* p1 = (SRNInfo*)v1;
    SRNInfo* p2 = (SRNInfo*)v2;

    float fViewDist1 = p1->fMaxViewDist - p1->objSphere.radius;
    float fViewDist2 = p2->fMaxViewDist - p2->objSphere.radius;

    // if same - give closest sectors higher priority
    if (fViewDist1 > fViewDist2)
    {
        return 1;
    }
    else if (fViewDist1 < fViewDist2)
    {
        return -1;
    }

    return 0;
}

void C3DEngine::SetSkyMaterialPath(const string& skyMatName)
{
    m_skyMatName = skyMatName;
    m_pSkyMat = nullptr;
}

void C3DEngine::SetSkyLowSpecMaterialPath(const string& skyLowSpecMatName)
{
    m_skyLowSpecMatName = skyLowSpecMatName;
    m_pSkyLowSpecMat = nullptr;
}

void C3DEngine::LoadSkyMaterial()
{
    const int skyType = GetCVars()->e_SkyType;
    if (skyType == 0)
    {
        if (!m_pSkyLowSpecMat)
        {
            m_pSkyLowSpecMat = m_skyLowSpecMatName.empty() ? nullptr : m_pMatMan->LoadMaterial(m_skyLowSpecMatName.c_str(), false, false, MTL_FLAG_IS_SKY);
            AZ_Warning("3DEngine", m_pSkyLowSpecMat, "Missing low spec sky material: %s", m_skyLowSpecMatName.c_str());
        }
    }
    else
    {
        if (!m_pSkyMat)
        {
            m_pSkyMat = m_skyMatName.empty() ? nullptr : m_pMatMan->LoadMaterial(m_skyMatName.c_str(), false, false, MTL_FLAG_IS_SKY);
            AZ_Warning("3DEngine", m_pSkyMat, "Missing sky material: %s", m_skyMatName.c_str());
        }
    }
    m_previousSkyType = skyType;
}

_smart_ptr<IMaterial> C3DEngine::GetSkyMaterial()
{
    const int skyType = GetCVars()->e_SkyType;

    // If e_SkyType has changed, then we may need to load a different sky material.
    if (skyType != m_previousSkyType)
    {
        LoadSkyMaterial();
    }

    return (skyType == 0) ? m_pSkyLowSpecMat : m_pSkyMat;
}

void C3DEngine::SetSkyMaterial(_smart_ptr<IMaterial> pSkyMat)
{
    m_pSkyMat = pSkyMat;
}

bool C3DEngine::IsHDRSkyMaterial(_smart_ptr<IMaterial> pMat) const
{
    return pMat && !azstricmp(pMat->GetSafeSubMtl(0)->GetShaderItem().m_pShader->GetName(), "SkyHDR");
}

void C3DEngine::RenderScene(const int nRenderFlags, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();
    CRY_ASSERT(passInfo.IsGeneralPass());
    CRY_ASSERT(m_pVisAreaManager);
    CRY_ASSERT(m_pClipVolumeManager);
    CRY_ASSERT(m_pDecalManager);

    GetObjManager()->GetCullThread().SetActive(true);

    if (GetCVars()->e_CoverageBuffer)
    {
        m_pCoverageBuffer->BeginFrame(passInfo);
    }

    if (m_pVisAreaManager != nullptr)
    {
        m_pVisAreaManager->DrawOcclusionAreasIntoCBuffer(m_pCoverageBuffer, passInfo);
        m_pVisAreaManager->CheckVis(passInfo);
    }

    if (m_pClipVolumeManager)
    {
        m_pClipVolumeManager->PrepareVolumesForRendering(passInfo);
    }

    if (m_pObjManager)
    {
        m_pObjManager->RenderAllObjectDebugInfo();
    }
    SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);

    // make sure all jobs from the previous frame have finished
    threadID nPrevThreadID = 0;
    gEnv->pRenderer->EF_Query(EFQ_RenderThreadList, nPrevThreadID);
    gEnv->pRenderer->GetFinalizeRendItemJobExecutor(nPrevThreadID)->WaitForCompletion();
    gEnv->pRenderer->GetFinalizeShadowRendItemJobExecutor(nPrevThreadID)->WaitForCompletion();

    GetRenderer()->EF_ClearSkinningDataPool();
    GetRenderer()->BeginSpawningGeneratingRendItemJobs(passInfo.ThreadID());

    GetRenderer()->EF_StartEf(passInfo);

    m_bIsInRenderScene = true;
    COctreeNode::ReleaseEmptyNodes();

    m_LightVolumesMgr.Clear(passInfo);

    SubmitSun(passInfo);

    if (GetCVars()->e_StatObjBufferRenderTasks && m_pObjManager != nullptr)
    {
        m_pObjManager->BeginOcclusionCulling(passInfo);
    }

    if (m_pVisAreaManager != nullptr)
    {
        m_pVisAreaManager->DrawVisibleSectors(passInfo, rendItemSorter);
    }
    m_nOceanRenderFlags &= ~OCR_OCEANVOLUME_VISIBLE;

    if (IsOutdoorVisible() || GetRenderer()->IsPost3DRendererEnabled())
    {
        if (m_pVisAreaManager != nullptr && m_pVisAreaManager->m_lstOutdoorPortalCameras.Count()   &&
            (m_pVisAreaManager->m_pCurArea || m_pVisAreaManager->m_pCurPortal))
        { // enable multi-camera culling
            const_cast<CCamera&>(passInfo.GetCamera()).m_pMultiCamera = &m_pVisAreaManager->m_lstOutdoorPortalCameras;
        }

        if (IsOutdoorVisible())
        {
            RenderSkyBox(GetSkyMaterial(), passInfo);
        }

        rendItemSorter.IncreaseOctreeCounter();
        {
            FRAME_PROFILER_LEGACYONLY("COctreeNode::Render_____", GetSystem(), PROFILE_3DENGINE);
            AZ_TRACE_METHOD_NAME("COctreeNode::Render");
            if (m_pObjectsTree != nullptr)
            {
                m_pObjectsTree->Render_Object_Nodes(false, OCTREENODE_RENDER_FLAG_OBJECTS, passInfo, rendItemSorter);
            }
        }
        rendItemSorter.IncreaseGroupCounter();
    }
    else if (m_pVisAreaManager && m_pVisAreaManager->IsSkyVisible())
    {
        RenderSkyBox(GetSkyMaterial(), passInfo);
    }
    
    // Outdoor is not visible, that means there is no SkyBox to render.
    // So we want to clear the GBuffer RT/background in order to avoid artifacts.
    GetRenderer()->SetClearBackground(!IsOutdoorVisible());

    if (nRenderFlags & SHDF_ALLOW_AO)
    {
        SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::UpdateRenderData);
    }

    {
        FRAME_PROFILER_LEGACYONLY("COctreeNode::Render_Object_Nodes_NEAR", GetSystem(), PROFILE_3DENGINE);
        AZ_TRACE_METHOD_NAME("COctreeNode::Render_Object_Nodes_NEAR");
        rendItemSorter.IncreaseOctreeCounter();
        if (GetCVars()->e_PortalsBigEntitiesFix)
        {
            if (!IsOutdoorVisible() && GetVisAreaManager() != nullptr && GetVisAreaManager()->GetCurVisArea())
            {
                if (GetVisAreaManager()->GetCurVisArea()->IsConnectedToOutdoor())
                {
                    CCamera cam = passInfo.GetCamera();
                    cam.SetFrustum(cam.GetViewSurfaceX(), cam.GetViewSurfaceZ(), cam.GetFov(), min(cam.GetNearPlane(), 1.f), 2.f, cam.GetPixelAspectRatio());
                    m_pObjectsTree->Render_Object_Nodes(false, OCTREENODE_RENDER_FLAG_OBJECTS | OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES,  SRenderingPassInfo::CreateTempRenderingInfo(cam, passInfo), rendItemSorter);
                }
            }
        }
    }
    rendItemSorter.IncreaseGroupCounter();

    // render special objects like laser beams intersecting entire level
    for (int i = 0; i < m_lstAlwaysVisible.Count(); i++)
    {
        IRenderNode* pObj = m_lstAlwaysVisible[i];
        const AABB& objBox = pObj->GetBBox();
        // don't frustum cull the HUD. When e.g. zooming the FOV for this camera is very different to the
        // fixed HUD FOV, and this can cull incorrectly.
        const unsigned int dwRndFlags = pObj->GetRndFlags();
        if (dwRndFlags & ERF_HUD || passInfo.GetCamera().IsAABBVisible_E(objBox))
        {
            FRAME_PROFILER_LEGACYONLY("C3DEngine::RenderScene_DrawAlwaysVisible", GetSystem(), PROFILE_3DENGINE);
            AZ_TRACE_METHOD_NAME("COctreeNode::RenderScene_DrawAlwaysVisible");

            Vec3 vCamPos = passInfo.GetCamera().GetPosition();
            float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
            assert(fEntDistance >= 0 && _finite(fEntDistance));
            if (fEntDistance < pObj->m_fWSMaxViewDist && GetObjManager() != nullptr)
            {
                GetObjManager()->RenderObject(pObj, objBox, fEntDistance, pObj->GetRenderNodeType(), passInfo, rendItemSorter);
            }
        }
    }
    rendItemSorter.IncreaseGroupCounter();

    if (m_pOcean)
    {
        ProcessOcean(passInfo);
    }

    if (passInfo.RenderDecals() && m_pDecalManager != nullptr)
    {
        m_pDecalManager->Render(passInfo);
    }

    // tell the occlusion culler that no new work will be submitted
    if (GetCVars()->e_StatObjBufferRenderTasks == 1 && GetObjManager() != nullptr)
    {
        GetObjManager()->PushIntoCullQueue(SCheckOcclusionJobData::CreateQuitJobData());
    }

    // fill shadow list here to allow more time between starting and waiting for the occlusion buffer
    InitShadowFrustums(passInfo);

    gEnv->pSystem->DoWorkDuringOcclusionChecks();

    if (GetCVars()->e_StatObjBufferRenderTasks && m_pObjManager != nullptr)
    {
        m_pObjManager->RenderBufferedRenderMeshes(passInfo);
    }

    // don't start shadow jobs if we aren't generating shadows
    if ((nRenderFlags & SHDF_NO_SHADOWGEN) == 0)
    {
        GetRenderer()->EF_InvokeShadowMapRenderJobs(IsShadersSyncLoad() ? (nRenderFlags | SHDF_NOASYNC | SHDF_STREAM_SYNC) : nRenderFlags);
    }

    m_LightVolumesMgr.Update(passInfo);

    SetupDistanceFog();

    SetupClearColor();

    {
        FRAME_PROFILER("Renderer::EF_EndEf3D", GetSystem(), PROFILE_RENDERER);
        // TODO: separate SHDF_NOASYNC and SHDF_STREAM_SYNC flags
        GetRenderer()->EF_EndEf3D(IsShadersSyncLoad() ? (nRenderFlags | SHDF_NOASYNC | SHDF_STREAM_SYNC) : nRenderFlags,  GetObjManager()->GetUpdateStreamingPrioriryRoundId(), GetObjManager()->GetUpdateStreamingPrioriryRoundIdFast(), passInfo);
    }

    GetRenderer()->EnableFog(false);

    bool bIsMultiThreadedRenderer = false;
    gEnv->pRenderer->EF_Query(EFQ_RenderMultithreaded, bIsMultiThreadedRenderer);
    if (bIsMultiThreadedRenderer)
    {
        gEnv->pRenderer->EndSpawningGeneratingRendItemJobs();
    }

    m_bIsInRenderScene = false;

#ifndef _RELEASE
    IF (GetCVars()->e_LightVolumesDebug, 0)
    {
        m_LightVolumesMgr.DrawDebug(passInfo);
    }
#endif
}

void C3DEngine::WaitForCullingJobsCompletion()
{
    const bool waitForOcclusionJobCompletion = true;
    m_pObjManager->EndOcclusionCulling(waitForOcclusionJobCompletion);
    COctreeNode::WaitForContentJobCompletion();
}

void C3DEngine::RenderSceneReflection(const int nRenderFlags, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();
    CRY_ASSERT(passInfo.IsRecursivePass());
    CRY_ASSERT(passInfo.GetRecursiveLevel() < MAX_RECURSION_LEVELS);
    CRY_ASSERT(m_pVisAreaManager);
    CRY_ASSERT(m_pClipVolumeManager);
    CRY_ASSERT(m_pDecalManager);

    if (!GetCVars()->e_Recursion)
    {
        return;
    }

    if (m_pVisAreaManager != nullptr)
    {
        m_pVisAreaManager->CheckVis(passInfo);
    }

    if (m_pClipVolumeManager != nullptr)
    {
        m_pClipVolumeManager->PrepareVolumesForRendering(passInfo);
    }
    ////////////////////////////////////////////////////////////////////////////////////////
    // From here we add render elements of main scene
    ////////////////////////////////////////////////////////////////////////////////////////
    SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);

    GetRenderer()->EF_StartEf(passInfo);

    if (m_pVisAreaManager != nullptr)
    {
        m_pVisAreaManager->DrawVisibleSectors(passInfo, rendItemSorter);
    }

    if (IsOutdoorVisible() || GetRenderer()->IsPost3DRendererEnabled())
    {
        if (m_pVisAreaManager != nullptr && m_pVisAreaManager->m_lstOutdoorPortalCameras.Count() &&
            (m_pVisAreaManager->m_pCurArea || m_pVisAreaManager->m_pCurPortal))
        { // enable multi-camera culling
            const_cast<CCamera&>(passInfo.GetCamera()).m_pMultiCamera = &m_pVisAreaManager->m_lstOutdoorPortalCameras;
        }

        if (IsOutdoorVisible())
        {
            RenderSkyBox(GetSkyMaterial(), passInfo);
        }

        {
            rendItemSorter.IncreaseOctreeCounter();
            FRAME_PROFILER("COctreeNode::Render_____", GetSystem(), PROFILE_3DENGINE);
            if (m_pObjectsTree != nullptr)
            {
                m_pObjectsTree->Render_Object_Nodes(false, OCTREENODE_RENDER_FLAG_OBJECTS, passInfo, rendItemSorter);
            }
        }
        rendItemSorter.IncreaseGroupCounter();
    }
    else if (m_pVisAreaManager != nullptr && m_pVisAreaManager->IsSkyVisible())
    {
        RenderSkyBox(GetSkyMaterial(), passInfo);
    }

    {
        FRAME_PROFILER("COctreeNode::Render_Object_Nodes_NEAR", GetSystem(), PROFILE_3DENGINE);
        rendItemSorter.IncreaseOctreeCounter();
        if (GetCVars()->e_PortalsBigEntitiesFix)
        {
            if (!IsOutdoorVisible() && GetVisAreaManager() != nullptr && GetVisAreaManager()->GetCurVisArea())
            {
                if (GetVisAreaManager()->GetCurVisArea()->IsConnectedToOutdoor())
                {
                    CCamera cam = passInfo.GetCamera();
                    cam.SetFrustum(cam.GetViewSurfaceX(), cam.GetViewSurfaceZ(), cam.GetFov(), min(cam.GetNearPlane(), 1.f), 2.f, cam.GetPixelAspectRatio());
                    if (m_pObjectsTree != nullptr)
                    {
                        m_pObjectsTree->Render_Object_Nodes(false, OCTREENODE_RENDER_FLAG_OBJECTS | OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES, SRenderingPassInfo::CreateTempRenderingInfo(cam, passInfo), rendItemSorter);
                    }
                }
            }
        }
    }
    rendItemSorter.IncreaseGroupCounter();

    // render special objects like laser beams intersecting entire level
    for (int i = 0; i < m_lstAlwaysVisible.Count(); i++)
    {
        IRenderNode* pObj = m_lstAlwaysVisible[i];
        const AABB& objBox = pObj->GetBBox();
        // don't frustum cull the HUD. When e.g. zooming the FOV for this camera is very different to the
        // fixed HUD FOV, and this can cull incorrectly.
        const unsigned int dwRndFlags = pObj->GetRndFlags();
        if (dwRndFlags & ERF_HUD || passInfo.GetCamera().IsAABBVisible_E(objBox))
        {
            FRAME_PROFILER("C3DEngine::RenderScene_DrawAlwaysVisible", GetSystem(), PROFILE_3DENGINE);

            Vec3 vCamPos = passInfo.GetCamera().GetPosition();
            float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
            assert(fEntDistance >= 0 && _finite(fEntDistance));
            if (fEntDistance < pObj->m_fWSMaxViewDist)
            {
                GetObjManager()->RenderObject(pObj, objBox, fEntDistance, pObj->GetRenderNodeType(), passInfo, rendItemSorter);
            }
        }
    }
    rendItemSorter.IncreaseGroupCounter();

    if (m_pOcean)
    {
        ProcessOcean(passInfo);
    }

    //Update light volumes again. Processing particles may have resulted in an increase in the number of light volumes.
    m_LightVolumesMgr.Update(passInfo);

    if (passInfo.RenderDecals() && m_pDecalManager != nullptr)
    {
        m_pDecalManager->Render(passInfo);
    }

    {
        FRAME_PROFILER("Renderer::EF_EndEf3D", GetSystem(), PROFILE_RENDERER);
        GetRenderer()->EF_EndEf3D(IsShadersSyncLoad() ? (nRenderFlags | SHDF_NOASYNC | SHDF_STREAM_SYNC) : nRenderFlags,  GetObjManager()->GetUpdateStreamingPrioriryRoundId(), GetObjManager()->GetUpdateStreamingPrioriryRoundIdFast(), passInfo);
    }
}

void C3DEngine::ProcessOcean(const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();

    AZ_Assert(m_pOcean != nullptr, "Ocean pointer must be validated before calling ProcessOcean");

    if (GetOceanRenderFlags() & OCR_NO_DRAW || !GetVisAreaManager() || GetCVars()->e_DefaultMaterial)
    {
        return;
    }

    bool bOceanIsForcedByVisAreaFlags = GetVisAreaManager()->IsOceanVisible();

    if (!IsOutdoorVisible() && !bOceanIsForcedByVisAreaFlags)
    {
        return;
    }

    bool bOceanVisible = false;
    if (OceanToggle::IsActive())
    {
        bOceanVisible = OceanRequest::OceanIsEnabled();
    }
    else
    {
        bOceanVisible = true;
    }

    if (bOceanVisible && passInfo.RenderWaterOcean() && m_bOcean)
    {
        Vec3 vCamPos = passInfo.GetCamera().GetPosition();
        float fWaterPlaneSize = passInfo.GetCamera().GetFarPlane();
        const float fOceanLevel = OceanToggle::IsActive() ? OceanRequest::GetOceanLevel():  m_pOcean->GetWaterLevel();

        AABB boxOcean(Vec3(vCamPos.x - fWaterPlaneSize, vCamPos.y - fWaterPlaneSize, std::numeric_limits<float>::lowest()),
            Vec3(vCamPos.x + fWaterPlaneSize, vCamPos.y + fWaterPlaneSize, fOceanLevel + 0.5f));

        if ((!bOceanIsForcedByVisAreaFlags && passInfo.GetCamera().IsAABBVisible_EM(boxOcean)) ||
            (bOceanIsForcedByVisAreaFlags && passInfo.GetCamera().IsAABBVisible_E (boxOcean)))
        {
            bool bOceanIsVisibleFromIndoor = true;
            if (class PodArray<CCamera>* pMultiCamera = passInfo.GetCamera().m_pMultiCamera)
            {
                for (int i = 0; i < pMultiCamera->Count(); i++)
                {
                    CVisArea* pExitPortal = (CVisArea*)(pMultiCamera->Get(i))->m_pPortal;
                    float fMinZ = pExitPortal->GetAABBox()->min.z;
                    float fMaxZ = pExitPortal->GetAABBox()->max.z;

                    if (!bOceanIsForcedByVisAreaFlags)
                    {
                        if (fMinZ > fOceanLevel && vCamPos.z < fMinZ)
                        {
                            bOceanIsVisibleFromIndoor = false;
                        }

                        if (fMaxZ < fOceanLevel && vCamPos.z > fMaxZ)
                        {
                            bOceanIsVisibleFromIndoor = false;
                        }
                    }
                }
            }

            if (bOceanIsVisibleFromIndoor)
            {
                m_pOcean->Update(passInfo);

                if ((GetOceanRenderFlags() & OCR_OCEANVOLUME_VISIBLE))
                {
                    if (passInfo.RenderWaterOcean())
                    {
                        m_pOcean->Render(passInfo);
                        m_pOcean->SetLastFov(passInfo.GetCamera().GetFov());
                    }
                }
            }
        }
    }

    if (GetCVars()->e_WaterRipplesDebug > 0)
    {
        GetRenderer()->EF_DrawWaterSimHits();
    }
}

void C3DEngine::RenderSkyBox(_smart_ptr<IMaterial> pMat, const SRenderingPassInfo& passInfo)
{
    FUNCTION_PROFILER_3DENGINE_LEGACYONLY;
    AZ_TRACE_METHOD();

    if (!Get3DEngine()->GetCoverageBuffer()->IsOutdooVisible())
    {
        return;
    }

    const float fForceDrawLastSortOffset = 100000.0f;

    // hdr sky dome
    // TODO: temporary workaround to force the right sky dome for the selected shader
    if (m_pREHDRSky && IsHDRSkyMaterial(pMat))
    {
        if (GetCVars()->e_SkyBox)
        {
#ifndef CONSOLE_CONST_CVAR_MODE
            if (GetCVars()->e_SkyQuality < 1)
            {
                GetCVars()->e_SkyQuality = 1;
            }
            else if (GetCVars()->e_SkyQuality > 2)
            {
                GetCVars()->e_SkyQuality = 2;
            }
#endif
            m_pSkyLightManager->SetQuality(GetCVars()->e_SkyQuality);

            // set sky light incremental update rate and perform update
            if (GetCVars()->e_SkyUpdateRate <= 0.0f)
            {
                GetCVars()->e_SkyUpdateRate = 0.01f;
            }
            m_pSkyLightManager->IncrementalUpdate(GetCVars()->e_SkyUpdateRate, passInfo);

            // prepare render object
            CRenderObject* pObj = GetRenderer()->EF_GetObject_Temp(passInfo.ThreadID());
            if (!pObj)
            {
                return;
            }
            pObj->m_II.m_Matrix.SetTranslationMat(passInfo.GetCamera().GetPosition());
            pObj->m_pRenderNode = 0;//m_pREHDRSky;
            pObj->m_fSort = fForceDrawLastSortOffset; // force sky to draw last

            /*          if( 0 == m_nRenderStackLevel )
            {
            // set scissor rect
            pObj->m_nScissorX1 = GetCamera().m_ScissorInfo.x1;
            pObj->m_nScissorY1 = GetCamera().m_ScissorInfo.y1;
            pObj->m_nScissorX2 = GetCamera().m_ScissorInfo.x2;
            pObj->m_nScissorY2 = GetCamera().m_ScissorInfo.y2;
            }*/

            m_pREHDRSky->m_pRenderParams = m_pSkyLightManager->GetRenderParams();
            m_pREHDRSky->m_moonTexId = m_nNightMoonTexId;

            // add sky dome to render list
            SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);
            GetRenderer()->EF_AddEf(m_pREHDRSky, pMat->GetSafeSubMtl(0)->GetShaderItem(), pObj, passInfo, EFSLIST_GENERAL, 1, rendItemSorter);
        }
    }
    // skybox
    else
    {
        if (pMat && m_pRESky && GetCVars()->e_SkyBox)
        {
            CRenderObject* pObj = GetRenderer()->EF_GetObject_Temp(passInfo.ThreadID());
            if (!pObj)
            {
                return;
            }
            pObj->m_II.m_Matrix.SetTranslationMat(passInfo.GetCamera().GetPosition());
            pObj->m_II.m_Matrix = pObj->m_II.m_Matrix * Matrix33::CreateRotationZ(DEG2RAD(m_fSkyBoxAngle));
            pObj->m_fSort = fForceDrawLastSortOffset; // force sky to draw last

            if (OceanToggle::IsActive())
            {
                m_pRESky->m_fTerrainWaterLevel = OceanRequest::GetOceanLevelOrDefault(-100000.0f);
            }
            else
            {
                const float waterLevel = m_pOcean ? m_pOcean->GetWaterLevel() : 0.0f;
                m_pRESky->m_fTerrainWaterLevel = max(0.0f, waterLevel);
            }
            m_pRESky->m_fSkyBoxStretching = m_fSkyBoxStretching;

            SRendItemSorter rendItemSorter = SRendItemSorter::CreateRendItemSorter(passInfo);
            GetRenderer()->EF_AddEf(m_pRESky, pMat->GetSafeSubMtl(0)->GetShaderItem(), pObj, passInfo, EFSLIST_GENERAL, 1, rendItemSorter);
        }
    }
}

void C3DEngine::DrawTextRightAligned(const float x, const float y, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    SDrawTextInfo ti;
    ti.flags = eDrawText_FixedSize | eDrawText_Right | eDrawText_2D | eDrawText_Monospace;
    ti.xscale = ti.yscale = DISPLAY_INFO_SCALE;
    GetRenderer()->DrawTextQueued(Vec3(x, y, 1.0f), ti, format, args);

    va_end(args);
}

void C3DEngine::DrawTextAligned(int flags, const float x, const float y, const float scale, const ColorF& color, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    SDrawTextInfo ti;
    ti.flags = flags;
    ti.color[0] = color[0];
    ti.color[1] = color[1];
    ti.color[2] = color[2];
    ti.color[3] = color[3];
    ti.xscale = ti.yscale = scale;
    GetRenderer()->DrawTextQueued(Vec3(x, y, 1.0f), ti, format, args);

    va_end(args);
}

void C3DEngine::DrawTextLeftAligned(const float x, const float y, const float scale, const ColorF& color, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    SDrawTextInfo ti;
    ti.flags = eDrawText_FixedSize | eDrawText_2D | eDrawText_Monospace;
    ti.color[0] = color[0];
    ti.color[1] = color[1];
    ti.color[2] = color[2];
    ti.color[3] = color[3];
    ti.xscale = ti.yscale = scale;
    GetRenderer()->DrawTextQueued(Vec3(x, y, 1.0f), ti, format, args);

    va_end(args);
}

void C3DEngine::DrawTextRightAligned(const float x, const float y, const float scale, const ColorF& color, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    SDrawTextInfo ti;
    ti.flags = eDrawText_FixedSize | eDrawText_Right | eDrawText_2D | eDrawText_Monospace;
    ti.color[0] = color[0];
    ti.color[1] = color[1];
    ti.color[2] = color[2];
    ti.color[3] = color[3];
    ti.xscale = ti.yscale = scale;
    GetRenderer()->DrawTextQueued(Vec3(x, y, 1.0f), ti, format, args);

    va_end(args);
}

int __cdecl C3DEngine__Cmp_FPS(const void* v1, const void* v2)
{
    float f1 = *(float*)v1;
    float f2 = *(float*)v2;

    if (f1 > f2)
    {
        return 1;
    }
    else if (f1 < f2)
    {
        return -1;
    }

    return 0;
}

inline void Blend(float& Stat, float StatCur, float fBlendCur)
{
    Stat = Stat * (1.f - fBlendCur) + StatCur * fBlendCur;
}

inline void Blend(float& Stat, int& StatCur, float fBlendCur)
{
    Blend(Stat, float(StatCur), fBlendCur);
    StatCur = int_round(Stat);
}

#ifdef ENABLE_LW_PROFILERS
static void AppendString(char*& szEnd, const char* szToAppend)
{
    assert(szToAppend);

    while (*szToAppend)
    {
        *szEnd++ = *szToAppend++;
    }

    *szEnd++ = ' ';
    *szEnd = 0;
}
#endif

void C3DEngine::DisplayInfo([[maybe_unused]] float& fTextPosX, [[maybe_unused]] float& fTextPosY, [[maybe_unused]] float& fTextStepY, [[maybe_unused]] const bool bEnhanced)
{
#ifdef ENABLE_LW_PROFILERS
    //  FUNCTION_PROFILER_3DENGINE; causes 0 fps in stats
    static ICVar* pDisplayInfo = GetConsole()->GetCVar("r_DisplayInfo");
    assert(pDisplayInfo);
    if (pDisplayInfo && pDisplayInfo->GetIVal() == 0)
    {
        return;
    }

    if (gEnv->IsDedicated())
    {
        return;
    }

#if defined(INFO_FRAME_COUNTER)
    static int frameCounter = 0;
#endif
    GetRenderer()->SetState(GS_NODEPTHTEST);

    fTextPosY = -10;
    fTextStepY = 13;
    fTextPosX = (float)GetRenderer()->GetOverlayWidth() - 5.0f;

    const char* description = GetRenderer()->GetRenderDescription();
    if (description && description[0] != 0)
    {
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 1.5f, ColorF(1.0f, 1.0f, 0.5f, 1.0f),
            "%s", description);
    }

    // If stat averaging is on, compute blend amount for current stats.
    float fFPS = GetTimer()->GetFrameRate();

    // Limit the FPS history for a single level to ~1 hour.
    // This vector is cleared on each level load, but during a soak test this continues to grow every frame
    const AZStd::size_t maxFPSEntries = 60 * 60 * 60; // 60ms * 60s * 60min
    if (arrFPSforSaveLevelStats.size() < maxFPSEntries)
    {
        arrFPSforSaveLevelStats.push_back(SATURATEB((int)fFPS));
    }

    float fBlendTime = GetTimer()->GetCurrTime();
    int iBlendMode = 0;
    float fBlendCur = GetTimer()->GetProfileFrameBlending(&fBlendTime, &iBlendMode);

    if (pDisplayInfo && pDisplayInfo->GetIVal() == 3)
    {
        static float fCurrentFPS, fCurrentFrameTime;
        Blend(fCurrentFPS, fFPS, fBlendCur);
        Blend(fCurrentFrameTime, GetTimer()->GetRealFrameTime() * 1000.0f, fBlendCur);
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 1.5f, ColorF(1.0f, 1.0f, 0.5f, 1.0f),
            "FPS %.1f - %.1fms", fCurrentFPS, fCurrentFrameTime);
        return;
    }

    // make level name
    char szLevelName[128];

    *szLevelName = 0;
    {
        int ii;
        for (ii = strlen(m_szLevelFolder) - 2; ii > 0; ii--)
        {
            if (m_szLevelFolder[ii] == '\\' || m_szLevelFolder[ii] == '/')
            {
                break;
            }
        }

        if (ii >= 0)
        {
            cry_strcpy(szLevelName, &m_szLevelFolder[ii + 1]);

            for (int i = strlen(szLevelName) - 1; i > 0; i--)
            {
                if (szLevelName[i] == '\\' || szLevelName[i] == '/')
                {
                    szLevelName[i] = 0;
                }
            }
        }
    }

    Matrix33 m = Matrix33(GetRenderingCamera().GetMatrix());
    //m.OrthonormalizeFast();       // why is that needed? is it?
    Ang3 aAng = RAD2DEG(Ang3::GetAnglesXYZ(m));
    Vec3 vPos = GetRenderingCamera().GetPosition();

    // Time of day info
    int hours = 0;
    int minutes = 0;
    ITimeOfDay* timeOfDay = GetTimeOfDay();
    if (timeOfDay)
    {
        float time = timeOfDay->GetTime();
        hours = (int)time;
        minutes = (int)((time - hours) * 60);
    }

    // display out of memory message if an allocation failed
    IF (gEnv->bIsOutOfMemory, 0)
    {
        ColorF fColor(1.0f, 0.0f, 0.0f, 1.0f);
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 4.0f, fColor, "**** Out of Memory ****");
        fTextPosY += 40.0f;
    }
    // display out of memory message if an allocation failed
    IF (gEnv->bIsOutOfVideoMemory, 0)
    {
        ColorF fColor(1.0f, 0.0f, 0.0f, 1.0f);
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 4.0f, fColor, "**** Out of Video Memory ****");
        fTextPosY += 40.0f;
    }

    float fogCullDist = 0.0f;
    Vec2 vViewportScale = Vec2(0.0f, 0.0f);
    m_pRenderer->EF_Query(EFQ_GetFogCullDistance, fogCullDist);
    m_pRenderer->EF_Query(EFQ_GetViewportDownscaleFactor, vViewportScale);

    DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "CamPos=%.2f %.2f %.2f Angl=%3d %2d %3d ZN=%.2f ZF=%d",
        vPos.x, vPos.y, vPos.z, (int)aAng.x, (int)aAng.y, (int)aAng.z,
        GetRenderingCamera().GetNearPlane(), (int)GetRenderingCamera().GetFarPlane());

    DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "Cam FC=%.2f VS=%.2f,%.2f Zoom=%.2f Speed=%1.2f TimeOfDay=%02d:%02d",
        fogCullDist, vViewportScale.x, vViewportScale.y,
        GetZoomFactor(), GetAverageCameraSpeed(), hours, minutes);

    // get version
    const SFileVersion& ver = GetSystem()->GetFileVersion();
    //char sVersion[128];
    //ver.ToString(sVersion);

    // Get memory usage.
    static IMemoryManager::SProcessMemInfo processMemInfo;
    {
        static int nGetMemInfoCount = 0;
        if ((nGetMemInfoCount & 0x1F) == 0 && GetISystem()->GetIMemoryManager())
        {
            // Only get mem stats every 32 frames.
            GetISystem()->GetIMemoryManager()->GetProcessMemInfo(processMemInfo);
        }
        nGetMemInfoCount++;
    }

    bool bMultiGPU;
    m_pRenderer->EF_Query(EFQ_MultiGPUEnabled, bMultiGPU);

    const char* pRenderType(0);

    if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
    {
        pRenderType = "DX11";
    }
    else
    {
        switch (gEnv->pRenderer->GetRenderType())
        {
        case eRT_OpenGL:
            pRenderType = "GL";
            break;
        case eRT_DX11:
            pRenderType = "DX11";
            break;
        case eRT_DX12:
            pRenderType = "DX12";
            break;
        case eRT_Jasper: 
            pRenderType = "Jasper";
            break;
        case eRT_Provo:
            pRenderType = "Provo";
            break;
        case eRT_Metal:
            pRenderType = "Metal";
            break;
        case eRT_Null:
            pRenderType = "Null";
            break;
        case eRT_Undefined:
        default:
            assert(0);
            pRenderType = "Undefined";
            break;
        }
    }
    assert(gEnv->pSystem);
    bool bTextureStreamingEnabled = false;
    m_pRenderer->EF_Query(EFQ_TextureStreamingEnabled, bTextureStreamingEnabled);
    const bool bCGFStreaming = GetCVars()->e_StreamCgf && m_pObjManager;
    const bool bTexStreaming = gEnv->pSystem->GetStreamEngine() && bTextureStreamingEnabled;
    char szFlags[128], * szFlagsEnd = szFlags;

#ifndef _RELEASE
    ESystemConfigSpec spec = GetISystem()->GetConfigSpec();
    switch (spec)
    {
    case CONFIG_AUTO_SPEC:
        AppendString(szFlagsEnd, "Auto");
        break;
    case CONFIG_LOW_SPEC:
        AppendString(szFlagsEnd, "LowSpec");
        break;
    case CONFIG_MEDIUM_SPEC:
        AppendString(szFlagsEnd, "MedSpec");
        break;
    case CONFIG_HIGH_SPEC:
        AppendString(szFlagsEnd, "HighSpec");
        break;
    case CONFIG_VERYHIGH_SPEC:
        AppendString(szFlagsEnd, "VeryHighSpec");
        break;
    default:
        assert(0);
    }
#endif
#ifndef CONSOLE_CONST_CVAR_MODE
    static ICVar* pMultiThreaded = GetConsole()->GetCVar("r_MultiThreaded");
    if (pMultiThreaded && pMultiThreaded->GetIVal() > 0)
#endif
    AppendString(szFlagsEnd, "MT");

    char* sAAMode = NULL;
    m_pRenderer->EF_Query(EFQ_AAMode, sAAMode);
    AppendString(szFlagsEnd, sAAMode);

    if (IsAreaActivationInUse())
    {
        AppendString(szFlagsEnd, "LA");
    }

    if (bMultiGPU)
    {
        AppendString(szFlagsEnd, "MGPU");
    }

    if (gEnv->pSystem->IsDevMode())
    {
        AppendString(szFlagsEnd, gEnv->IsEditor() ? "DevMode (Editor)" : "DevMode");
    }

    if (bCGFStreaming || bTexStreaming)
    {
        if (bCGFStreaming && !bTexStreaming)
        {
            AppendString(szFlagsEnd, "StG");
        }
        if (bTexStreaming && !bCGFStreaming)
        {
            AppendString(szFlagsEnd, "StT");
        }
        if (bTexStreaming && bCGFStreaming)
        {
            AppendString(szFlagsEnd, "StGT");
        }
    }

    // remove last space
    if (szFlags != szFlagsEnd)
    {
        *(szFlagsEnd - 1) = 0;
    }
#ifdef _RELEASE
    const char* mode = "Release";
#else
    const char* mode = "Profile";
#endif

    DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "%s %s %dbit %s %s [%d.%d]",
        pRenderType, mode, (int)sizeof(char*) * 8, szFlags, szLevelName, ver.v[1], ver.v[0]);

    // Polys in scene
    int nPolygons, nShadowPolygons;
    GetRenderer()->GetPolyCount(nPolygons, nShadowPolygons);
    int nDrawCalls,  nShadowGenDrawCalls;
    GetRenderer()->GetCurrentNumberOfDrawCalls(nDrawCalls,  nShadowGenDrawCalls);

    int nGeomInstances = GetRenderer()->GetNumGeomInstances();
    int nGeomInstanceDrawCalls = GetRenderer()->GetNumGeomInstanceDrawCalls();

    if (fBlendCur != 1.f)
    {
        // Smooth over time.
        static float fPolygons, fShadowVolPolys, fDrawCalls, fShadowGenDrawCalls, fGeomInstances, fGeomInstanceDrawCalls;
        Blend(fPolygons, nPolygons, fBlendCur);
        Blend(fShadowVolPolys, nShadowPolygons, fBlendCur);
        Blend(fDrawCalls, nDrawCalls, fBlendCur);
        Blend(fShadowGenDrawCalls, nShadowGenDrawCalls, fBlendCur);
        Blend(fGeomInstances, nGeomInstances, fBlendCur);
        Blend(fGeomInstanceDrawCalls, nGeomInstanceDrawCalls, fBlendCur);
    }

    //
    static float m_lastAverageDPTime = -FLT_MAX;
    float curTime = gEnv->pTimer->GetAsyncCurTime();
    static int lastDrawCalls = 0;
    static int lastShadowGenDrawCalls = 0;
    static int avgPolys = 0;
    static int avgShadowPolys = 0;
    static int sumPolys = 0;
    static int sumShadowPolys = 0;
    static int nPolysFrames = 0;
    if (curTime < m_lastAverageDPTime)
    {
        m_lastAverageDPTime = curTime;
    }
    if (curTime - m_lastAverageDPTime > 1.0f)
    {
        lastDrawCalls = nDrawCalls;
        lastShadowGenDrawCalls = nShadowGenDrawCalls;
        m_lastAverageDPTime = curTime;
        avgPolys = nPolysFrames ? sumPolys / nPolysFrames : 0;
        avgShadowPolys = nPolysFrames ? sumShadowPolys / nPolysFrames : 0;
        sumPolys = nPolygons;
        sumShadowPolys = nShadowPolygons;
        nPolysFrames = 1;
    }
    else
    {
        nPolysFrames++;
        sumPolys += nPolygons;
        sumShadowPolys += nShadowPolygons;
    }
    //

    int     nMaxDrawCalls = GetCVars()->e_MaxDrawCalls <= 0 ? 2000 : GetCVars()->e_MaxDrawCalls;
    bool    bInRed = (nDrawCalls + nShadowGenDrawCalls) > nMaxDrawCalls;

    DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, bInRed ? Col_Red : Col_White, "DP: %04d (%04d) ShadowGen:%04d (%04d) - Total: %04d Instanced: %04d",
        nDrawCalls, lastDrawCalls, nShadowGenDrawCalls, lastShadowGenDrawCalls, nDrawCalls + nShadowGenDrawCalls, nDrawCalls + nShadowGenDrawCalls - nGeomInstances + nGeomInstanceDrawCalls);
#if defined(MOBILE)
    bInRed = nPolygons > 500000;
#else
    bInRed = nPolygons > 1500000;
#endif

    DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, bInRed ? Col_Red : Col_White, "Polys: %03d,%03d (%03d,%03d) Shadow:%03d,%03d (%03d,%03d)",
        nPolygons / 1000, nPolygons % 1000, avgPolys / 1000, avgPolys % 1000,
        nShadowPolygons / 1000, nShadowPolygons % 1000, avgShadowPolys / 1000, avgShadowPolys % 1000);

    {
        SShaderCacheStatistics stats;
        m_pRenderer->EF_Query(EFQ_GetShaderCacheInfo, stats);
        {
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_White, "ShaderCache: %d GCM | %d Async Reqs | Compile: %s",
                (int)stats.m_nGlobalShaderCacheMisses, (int)stats.m_nNumShaderAsyncCompiles, stats.m_bShaderCompileActive ? "On" : "Off");
        }
    }

    // print stats about CGF's streaming
    if (bCGFStreaming)
    {
        static char szCGFStreaming[256] = "";
        static SObjectsStreamingStatus objectsStreamingStatus = {0};

        if (!(GetRenderer()->GetFrameID(false) & 15) || !szCGFStreaming[0] || GetCVars()->e_StreamCgfDebug)
        {
            m_pObjManager->GetObjectsStreamingStatus(objectsStreamingStatus);
            sprintf_s(szCGFStreaming, 256, "CgfStrm: Loaded:%d InProg:%d All:%d Act:%d PcP:%d MemUsed:%2.2f MemReq:%2.2f Pool:%d",
                objectsStreamingStatus.nReady, objectsStreamingStatus.nInProgress, objectsStreamingStatus.nTotal, objectsStreamingStatus.nActive,
                (int)m_pObjManager->GetStreamPreCachePointDefs().size(),
                float(objectsStreamingStatus.nAllocatedBytes) / 1024 / 1024, float(objectsStreamingStatus.nMemRequired) / 1024 / 1024, GetCVars()->e_StreamCgfPoolSize);
        }

        bool bOutOfMem((float(objectsStreamingStatus.nMemRequired) / 1024 / 1024) > GetCVars()->e_StreamCgfPoolSize);
        bool bCloseToOutOfMem((float(objectsStreamingStatus.nMemRequired) / 1024 / 1024) > GetCVars()->e_StreamCgfPoolSize * 90 / 100);

        ColorF color = Col_White;
        if (bOutOfMem)
        {
            color = Col_Red;
        }
        else if (bCloseToOutOfMem)
        {
            color = Col_Orange;
        }
        //      if(bTooManyRequests)
        //      color = Col_Magenta;

        if ((pDisplayInfo->GetIVal() == 2 || GetCVars()->e_StreamCgfDebug) || bOutOfMem || bCloseToOutOfMem)
        {
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, color, szCGFStreaming);
        }
    }

    // print stats about textures' streaming
    if (bTexStreaming)
    {
        static char szTexStreaming[256] = "";
        static bool bCloseToOutOfMem = false;
        static bool bOutOfMem = false;
        static bool bTooManyRequests = false;
        static bool bOverloadedPool = false;
        static uint32 nTexCount = 0;
        static uint32 nTexSize = 0;

        float fTexBandwidthRequired = 0.f;
        m_pRenderer->GetBandwidthStats(&fTexBandwidthRequired);

        if (!(GetRenderer()->GetFrameID(false) % 30) || !szTexStreaming[0])
        {
            STextureStreamingStats stats(!(GetRenderer()->GetFrameID(false) % 120));
            m_pRenderer->EF_Query(EFQ_GetTexStreamingInfo, stats);

            if (!(GetRenderer()->GetFrameID(false) % 120))
            {
                bOverloadedPool = stats.bPoolOverflowTotally;
                nTexCount = stats.nRequiredStreamedTexturesCount;
                nTexSize = stats.nRequiredStreamedTexturesSize;
            }

            int nPlatformSize = nTexSize;

            const int iPercentage = int((float)stats.nCurrentPoolSize / stats.nMaxPoolSize * 100.f);
            const int iStaticPercentage = int((float)stats.nStaticTexturesSize / stats.nMaxPoolSize * 100.f);
            sprintf_s(szTexStreaming, "TexStrm: TexRend: %u NumTex: %u Req:%.1fMB Mem(strm/stat/tot):%.1f/%.1f/%.1fMB(%d%%/%d%%) PoolSize:%" PRISIZE_T "MB PoolFrag:%.1f%%",
                stats.nNumTexturesPerFrame, nTexCount, (float)nPlatformSize / 1024 / 1024,
                (float)stats.nStreamedTexturesSize / 1024 / 1024, (float)stats.nStaticTexturesSize / 1024 / 1024, (float)stats.nCurrentPoolSize / 1024 / 1024,
                iPercentage, iStaticPercentage, stats.nMaxPoolSize / 1024 / 1024,
                stats.fPoolFragmentation * 100.0f
                );
            bOverloadedPool |= stats.bPoolOverflowTotally;

            bCloseToOutOfMem = iPercentage >= 90;
            bOutOfMem = stats.bPoolOverflow;
        }

        if (pDisplayInfo->GetIVal() == 2 || bCloseToOutOfMem || bTooManyRequests || bOverloadedPool)
        {
            ColorF color = Col_White;
            if (bOutOfMem)
            {
                color = Col_Red;
            }
            else if (bCloseToOutOfMem)
            {
                color = Col_Orange;
            }
            if (bTooManyRequests)
            {
                color = Col_Magenta;
            }
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, color, "%s", szTexStreaming);
        }

        if (pDisplayInfo->GetIVal() > 0 && bOverloadedPool)
        {
            DrawTextLeftAligned(0, 10, 2.3f, Col_Red, "Texture pool totally overloaded!");
        }
    }



    {
        static char szMeshPoolUse[256] = "";
        static unsigned nFlushFrameId = 0U;
        static unsigned nFallbackFrameId = 0U;
        static SMeshPoolStatistics lastStats;
        static SMeshPoolStatistics stats;

        const unsigned nMainFrameId = GetRenderer()->GetFrameID(false);
        m_pRenderer->EF_Query(EFQ_GetMeshPoolInfo, stats);
        const int iPercentage = int((float)stats.nPoolInUse / (stats.nPoolSize ? stats.nPoolSize : 1U) * 100.f);
        const int iVolatilePercentage = int((float)stats.nInstancePoolInUse / (stats.nInstancePoolSize ? stats.nInstancePoolSize : 1U) * 100.f);
        nFallbackFrameId = lastStats.nFallbacks < stats.nFallbacks ? nMainFrameId : nFallbackFrameId;
        nFlushFrameId = lastStats.nFlushes < stats.nFlushes ? nMainFrameId : nFlushFrameId;
        const bool bOverflow = nMainFrameId - nFlushFrameId < 50;
        const bool bFallback = nMainFrameId - nFallbackFrameId < 50;

        sprintf_s(szMeshPoolUse,
            "Mesh Pool: MemUsed:%.2fKB(%d%%%%) Peak %.fKB PoolSize:%" PRISIZE_T "KB Flushes %" PRISIZE_T " Fallbacks %.3fKB %s",
            (float)stats.nPoolInUse / 1024,
            iPercentage,
            (float)stats.nPoolInUsePeak / 1024,
            stats.nPoolSize / 1024,
            stats.nFlushes,
            (float)stats.nFallbacks / 1024.0f,
            (bFallback ? "FULL!" : bOverflow ? "OVERFLOW" : ""));

        if (stats.nPoolSize && (pDisplayInfo->GetIVal() == 2 || bOverflow || bFallback))
        {
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE,
                bFallback ? Col_Red : bOverflow ? Col_Orange : Col_White,
                szMeshPoolUse);
        }
        if (stats.nPoolSize && pDisplayInfo->GetIVal() == 2)
        {
            char szVolatilePoolsUse[256];
            sprintf_s(szVolatilePoolsUse,
                "Mesh Instance Pool: MemUsed:%.2fKB(%d%%%%) Peak %.fKB PoolSize:%" PRISIZE_T "KB Fallbacks %.3fKB",
                (float)stats.nInstancePoolInUse / 1024,
                iVolatilePercentage,
                (float)stats.nInstancePoolInUsePeak / 1024,
                stats.nInstancePoolSize / 1024,
                (float)stats.nInstanceFallbacks / 1024.0f);

            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE,
                Col_White, szVolatilePoolsUse);
        }

        memcpy(&lastStats, &stats, sizeof(lastStats));
    }

    // streaming info
    {
        IStreamEngine* pSE = gEnv->pSystem->GetStreamEngine();
        if (pSE)
        {
            SStreamEngineStatistics& stats = pSE->GetStreamingStatistics();
            SStreamEngineOpenStats openStats;
            pSE->GetStreamingOpenStatistics(openStats);

            static char szStreaming[128] = "";
            if (!(GetRenderer()->GetFrameID(false) & 7))
            {
                if (pDisplayInfo->GetIVal() == 2)
                {
                    sprintf_s(szStreaming, "Streaming IO: ACT: %3dmsec, Jobs:%2d Total:%5d",
                        (uint32)stats.fAverageCompletionTime, openStats.nOpenRequestCount, stats.nTotalStreamingRequestCount);
                }
                else
                {
                    sprintf_s(szStreaming, "Streaming IO: ACT: %3dmsec, Jobs:%2d",
                        (uint32)stats.fAverageCompletionTime, openStats.nOpenRequestCount);
                }
            }
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, szStreaming);

            if (stats.bTempMemOutOfBudget)
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 1.3f, Col_Red, "Temporary Streaming Memory Pool Out of Budget!");
            }
        }

        if (pDisplayInfo && pDisplayInfo->GetIVal() == 2) // more streaming info
        {
            SStreamEngineStatistics& stats = gEnv->pSystem->GetStreamEngine()->GetStreamingStatistics();

            { // HDD stats
                static char szStreaming[512] = "";
                sprintf_s(szStreaming, "HDD: BW:%1.2f|%1.2fMb/s (Eff:%2.1f|%2.1fMb/s) - Seek:%1.2fGB - Active:%2.1f%%%%",
                    (float)stats.hddInfo.nCurrentReadBandwidth / (1024 * 1024), (float)stats.hddInfo.nSessionReadBandwidth / (1024 * 1024),
                    (float)stats.hddInfo.nActualReadBandwidth / (1024 * 1024), (float)stats.hddInfo.nAverageActualReadBandwidth / (1024 * 1024),
                    (float)stats.hddInfo.nAverageSeekOffset / (1024 * 1024), stats.hddInfo.fAverageActiveTime);

                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, szStreaming);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Display Info about dynamic lights.
    //////////////////////////////////////////////////////////////////////////
    {
        {
#ifndef _RELEASE
            // Checkpoint loading information
            if (!gEnv->bMultiplayer)
            {
                ISystem::ICheckpointData data;
                gEnv->pSystem->GetCheckpointData(data);

                if (data.m_loadOrigin != ISystem::eLLO_Unknown)
                {
                    static const char* loadStates[] =
                    {
                        "",
                        "New Level",
                        "Level to Level",
                        "Resumed Game",
                        "Map Command",
                    };

                    DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 1.3f, Col_White, "%s, Checkpoint loads: %i", loadStates[(int)data.m_loadOrigin], (int)data.m_totalLoads);
                }
            }
#endif

            int nPeakMemMB = (int)(processMemInfo.PeakPagefileUsage >> 20);
            int nVirtMemMB = (int)(processMemInfo.PagefileUsage >> 20);
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "Mem=%d Peak=%d DLights=(%d)", nVirtMemMB, nPeakMemMB, m_nRealLightsNum + m_nDeferredLightsNum);

            uint32 nShadowFrustums = 0;
            uint32 nShadowAllocs = 0;
            uint32 nShadowMaskChannels = 0;
            m_pRenderer->EF_Query(EFQ_GetShadowPoolFrustumsNum, nShadowFrustums);
            m_pRenderer->EF_Query(EFQ_GetShadowPoolAllocThisFrameNum, nShadowAllocs);
            m_pRenderer->EF_Query(EFQ_GetShadowMaskChannelsNum, nShadowMaskChannels);
            bool bThrash = (nShadowAllocs & 0x80000000) ? true : false;
            nShadowAllocs &= ~0x80000000;
            uint32 nAvailableShadowMaskChannels = nShadowMaskChannels >> 16;
            uint32 nUsedShadowMaskChannels = nShadowMaskChannels & 0xFFFF;
            bool bTooManyLights = nUsedShadowMaskChannels > nAvailableShadowMaskChannels ? true : false;

            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, (nShadowFrustums || nShadowAllocs) ? Col_Yellow : Col_White, "%d Shadow Mask Channels, %3d Shadow Frustums, %3d Frustum Renders This Frame",
                nUsedShadowMaskChannels, nShadowFrustums, nShadowAllocs);

            if (bThrash)
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_Red, "SHADOW POOL THRASHING!!!");
            }

            if (bTooManyLights)
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_Red, "TOO MANY SHADOW CASTING LIGHTS (%d/%d)!!!", nUsedShadowMaskChannels, nAvailableShadowMaskChannels);
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_Red, "Consider increasing 'r_ShadowCastingLightsMaxCount'");
            }

#ifndef _RELEASE
            uint32 numTiledShadingSkippedLights;
            m_pRenderer->EF_Query(EFQ_GetTiledShadingSkippedLightsNum, numTiledShadingSkippedLights);
            if (numTiledShadingSkippedLights > 0)
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, Col_Red, "TILED SHADING: SKIPPED %d LIGHTS", numTiledShadingSkippedLights);
            }

            if (GetCVars()->e_levelStartupFrameNum)
            {
                static float startupAvgFPS = 0.f;
                static float levelStartupTime = 0;
                static int levelStartupFrameEnd = GetCVars()->e_levelStartupFrameNum + GetCVars()->e_levelStartupFrameDelay;
                int curFrameID = GetRenderer()->GetFrameID(false);

                if (curFrameID >= GetCVars()->e_levelStartupFrameDelay)
                {
                    if (curFrameID == GetCVars()->e_levelStartupFrameDelay)
                    {
                        levelStartupTime = gEnv->pTimer->GetAsyncCurTime();
                    }
                    if (curFrameID == levelStartupFrameEnd)
                    {
                        startupAvgFPS = (float)GetCVars()->e_levelStartupFrameNum / (gEnv->pTimer->GetAsyncCurTime() - levelStartupTime);
                    }
                    if (curFrameID >= levelStartupFrameEnd)
                    {
                        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 2.f, Col_Red, "Startup AVG FPS: %.2f", startupAvgFPS);
                        fTextPosY += fTextStepY;
                    }
                }
            }
#endif //_RELEASE
        }

        m_nDeferredLightsNum = 0;
    }

    assert(pDisplayInfo);
    if (bEnhanced)
    {
#define CONVX(x) (((x) / (float)gUpdateTimesNum))
#define CONVY(y) (1.f - ((y) / 720.f))
#define TICKS_TO_MS(t)  (1000.f * gEnv->pTimer->TicksToSeconds(t))
# define MAX_PHYS_TIME 32.f
# define MAX_PLE_TIME 4.f
        uint32 gUpdateTimeIdx = 0, gUpdateTimesNum = 0;
        const sUpdateTimes* gUpdateTimes = gEnv->pSystem->GetUpdateTimeStats(gUpdateTimeIdx, gUpdateTimesNum);
        if (pDisplayInfo->GetIVal() >= 5)
        {
            const SAuxGeomRenderFlags flags = gEnv->pRenderer->GetIRenderAuxGeom()->GetRenderFlags();
            SAuxGeomRenderFlags newFlags(flags);
            newFlags.SetAlphaBlendMode(e_AlphaNone);
            newFlags.SetMode2D3DFlag(e_Mode2D);
            newFlags.SetCullMode(e_CullModeNone);
            newFlags.SetDepthWriteFlag(e_DepthWriteOff);
            newFlags.SetDepthTestFlag(e_DepthTestOff);
            newFlags.SetFillMode(e_FillModeSolid);
            gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(newFlags);
            const ColorF colorPhysFull = Col_Blue;
            const ColorF colorSysFull = Col_Green;
            const ColorF colorRenFull = Col_Red;
            const ColorF colorPhysHalf = colorPhysFull * 0.15f;
            const ColorF colorSysHalf = colorSysFull * 0.15f;
            const ColorF colorRenHalf = colorRenFull * 0.15f;
            float phys = (TICKS_TO_MS(gUpdateTimes[0].PhysStepTime) / 66.f) * 720.f;
            float sys = (TICKS_TO_MS(gUpdateTimes[0].SysUpdateTime) / 66.f) * 720.f;
            float ren = (TICKS_TO_MS(gUpdateTimes[0].RenderTime) / 66.f) * 720.f;
            float _lerp = ((float)(max((int)gUpdateTimeIdx - (int)0, 0) / (float)gUpdateTimesNum));
            ColorB colorPhysLast;
            colorPhysLast.lerpFloat(colorPhysFull, colorPhysHalf, _lerp);
            ColorB colorSysLast;
            colorSysLast.lerpFloat(colorSysFull, colorSysHalf, _lerp);
            ColorB colorRenLast;
            colorRenLast.lerpFloat(colorRenFull, colorRenHalf, _lerp);
            Vec3 lastPhys(CONVX(0), CONVY(phys), 1.f);
            Vec3 lastSys(CONVX(0), CONVY(sys), 1.f);
            Vec3 lastRen(CONVX(0), CONVY(ren), 1.f);
            for (uint32 i = 0; i < gUpdateTimesNum; ++i)
            {
                const float x = (float)i;
                _lerp = ((float)(max((int)gUpdateTimeIdx - (int)i, 0) / (float)gUpdateTimesNum));
                const sUpdateTimes& sample = gUpdateTimes[i];
                phys = (TICKS_TO_MS(sample.PhysStepTime) / 66.f) * 720.f;
                sys = (TICKS_TO_MS(sample.SysUpdateTime) / 66.f) * 720.f;
                ren = (TICKS_TO_MS(sample.RenderTime) / 66.f) * 720.f;
                Vec3 curPhys(CONVX(x), CONVY(phys), 1.f);
                Vec3 curSys(CONVX(x), CONVY(sys), 1.f);
                Vec3 curRen(CONVX(x), CONVY(ren), 1.f);
                ColorB colorPhys;
                colorPhys.lerpFloat(colorPhysFull, colorPhysHalf, _lerp);
                ColorB colorSys;
                colorSys.lerpFloat(colorSysFull, colorSysHalf, _lerp);
                ColorB colorRen;
                colorRen.lerpFloat(colorRenFull, colorRenHalf, _lerp);
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(lastPhys, colorPhysLast, curPhys, colorPhys);
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(lastSys, colorSysLast, curSys, colorSys);
                gEnv->pRenderer->GetIRenderAuxGeom()->DrawLine(lastRen, colorRenLast, curRen, colorRen);
                lastPhys = curPhys;
                colorPhysLast = colorPhys;
                lastSys = curSys;
                colorSysLast = colorSys;
                lastRen = curRen;
                colorRenLast = colorRen;
            }
            gEnv->pRenderer->GetIRenderAuxGeom()->SetRenderFlags(flags);
        }
        const float curPhysTime = TICKS_TO_MS(gUpdateTimes[gUpdateTimeIdx].PhysStepTime);
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE_SMALL, curPhysTime > MAX_PHYS_TIME ? Col_Red : Col_White, "%3.1f ms      Phys", curPhysTime);
        const float curPhysWaitTime = TICKS_TO_MS(gUpdateTimes[gUpdateTimeIdx].physWaitTime);
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE_SMALL, curPhysTime > MAX_PHYS_TIME ? Col_Red : Col_White, "%3.1f ms   WaitPhys", curPhysWaitTime);

        float partTicks = 0;

        //3dengine stats from RenderWorld
        {
#if defined(MOBILE)
            const float maxVal = 12.f;
#else
            const float maxVal = 50.f;
#endif
            float fTimeMS = TICKS_TO_MS(m_nRenderWorldUSecs) - partTicks;
            DrawTextRightAligned(fTextPosX, fTextPosY += (fTextStepY - STEP_SMALL_DIFF), DISPLAY_INFO_SCALE_SMALL, fTimeMS > maxVal ? Col_Red : Col_White, "%.2f ms RendWorld", fTimeMS);
        }

        {
            SStreamEngineStatistics stat =  gEnv->pSystem->GetStreamEngine()->GetStreamingStatistics();

            float fTimeMS = 1000.0f * gEnv->pTimer->TicksToSeconds(stat.nMainStreamingThreadWait);
            DrawTextRightAligned(fTextPosX, fTextPosY += (fTextStepY - STEP_SMALL_DIFF),
                DISPLAY_INFO_SCALE_SMALL, Col_White, "%3.1f ms     StreamFin", fTimeMS);
        }
    }

#undef MAX_PHYS_TIME
#undef TICKS_TO_MS
#undef CONVY
#undef CONVX

    //////////////////////////////////////////////////////////////////////////
    // Display Thermal information of the device (if supported)
    //////////////////////////////////////////////////////////////////////////

    if (ThermalInfoRequestsBus::GetTotalNumOfEventHandlers())
    {
        const int thermalSensorCount = static_cast<int>(ThermalSensorType::Count);
        const char* sensorStrings[thermalSensorCount] = { "CPU", "GPU", "Battery" };
        for (int i = 0; i < thermalSensorCount; ++i)
        {
            float temperature = 0.f;
            ThermalSensorType sensor = static_cast<ThermalSensorType>(i);
            EBUS_EVENT_RESULT(temperature, ThermalInfoRequestsBus, GetSensorTemp, sensor);
            AZStd::string tempText;
            ColorF tempColor;
            if (temperature > 0.f)
            {
                float overheatingTemp = 0.f;
                EBUS_EVENT_RESULT(overheatingTemp, ThermalInfoRequestsBus, GetSensorOverheatingTemp, sensor);
                tempText = AZStd::string::format(" %.1f C", temperature);
                tempColor = temperature >= overheatingTemp ? Col_Red : Col_White;
            }
            else
            {
                tempText = "N/A";
                tempColor = Col_White;
            }
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE, tempColor, "%s Temp %s", sensorStrings[i], tempText.c_str());
        }       
    }

    //////////////////////////////////////////////////////////////////////////
    // Display Current fps
    //////////////////////////////////////////////////////////////////////////

    if (iBlendMode)
    {
        // Track FPS frequency, report min/max.
        Blend(m_fAverageFPS, fFPS, fBlendCur);

        Blend(m_fMinFPSDecay, fFPS, fBlendCur);
        if (fFPS <= m_fMinFPSDecay)
        {
            m_fMinFPS = m_fMinFPSDecay = fFPS;
        }

        Blend(m_fMaxFPSDecay, fFPS, fBlendCur);
        if (fFPS >= m_fMaxFPSDecay)
        {
            m_fMaxFPS = m_fMaxFPSDecay = fFPS;
        }

        const char* sMode = "";
        switch (iBlendMode)
        {
        case 1:
            sMode = "frame avg";
            break;
        case 2:
            sMode = "time avg";
            break;
        case 3:
            sMode = "peak hold";
            break;
        }
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 1.5f, ColorF(1.0f, 1.0f, 0.5f, 1.0f),
            "FPS %.1f [%.0f..%.0f], %s over %.1f s",
            m_fAverageFPS, m_fMinFPS, m_fMaxFPS, sMode, fBlendTime);
    }
    else
    {
        const int nHistorySize = 16;
        static float arrfFrameRateHistory[nHistorySize] = {0};

        static int nFrameId = 0;
        nFrameId++;
        int nSlotId = nFrameId % nHistorySize;
        assert(nSlotId >= 0 && nSlotId < nHistorySize);
        arrfFrameRateHistory[nSlotId] = min(9999.f, GetTimer()->GetFrameRate());

        float fMinFPS = 9999.0f;
        float fMaxFPS = 0;
        for (int i = 0; i < nHistorySize; i++)
        {
            if (arrfFrameRateHistory[i] < fMinFPS)
            {
                fMinFPS = arrfFrameRateHistory[i];
            }
            if (arrfFrameRateHistory[i] > fMaxFPS)
            {
                fMaxFPS = arrfFrameRateHistory[i];
            }
        }

        float fFrameRate = 0;
        float fValidFrames = 0;
        for (int i = 0; i < nHistorySize; i++)
        {
            int s = (nFrameId - i) % nHistorySize;
            fFrameRate += arrfFrameRateHistory[s];
            fValidFrames++;
        }
        fFrameRate /= fValidFrames;

        m_fAverageFPS = fFrameRate;
        m_fMinFPS = m_fMinFPSDecay = fMinFPS;
        m_fMaxFPS = m_fMaxFPSDecay = fMaxFPS;


        //only difference to r_DisplayInfo 1, need ms for GPU time
        float fMax = (int(GetCurTimeSec() * 2) & 1) ? 999.f : 888.f;
        if (bEnhanced)
        {
            /*          DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY, "%6.2f ~%6.2f ms (%6.2f..%6.2f) CPU",
            GetTimer()->GetFrameTime()*1000.0f, 1000.0f/max(0.0001f,fFrameRate),
            1000.0f/max(0.0001f,fMinFPS),
            1000.0f/max(0.0001f,fMaxFPS));
            */
            const RPProfilerStats* pFrameRPPStats = GetRenderer()->GetRPPStats(eRPPSTATS_OverallFrame);
            float gpuTime = pFrameRPPStats ? pFrameRPPStats->gpuTime : 0.0f;
            static float sGPUTime = 0.f;
            if (gpuTime < 1000.f && gpuTime > 0.01f)
            {
                sGPUTime = gpuTime;                              //catch sporadic jumps
            }
            if (sGPUTime > 0.01f)
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, DISPLAY_INFO_SCALE_SMALL, (gpuTime >= 40.f) ? Col_Red : Col_White, "%3.1f ms       GPU", sGPUTime);
            }
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 1.4f, ColorF(1.0f, 1.0f, 0.2f, 1.0f), "FPS %5.1f (%3d..%3d)(%3.1f ms)",
                min(fMax, fFrameRate), (int)min(fMax, fMinFPS), (int)min(fMax, fMaxFPS), GetTimer()->GetFrameTime() * 1000.0f);
        }
        else
        {
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, 1.4f, ColorF(1.0f, 1.0f, 0.2f, 1.0f), "FPS %5.1f (%3d..%3d)",
                min(fMax, fFrameRate), (int)min(fMax, fMinFPS), (int)min(fMax, fMaxFPS));
        }
    }

#ifndef _RELEASE
    if (GetCVars()->e_GsmStats)
    {
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "--------------- GSM Stats ---------------");

        if (m_pSun && m_pSun->m_pShadowMapInfo)
        {
            CLightEntity::ShadowMapInfo* pSMI = m_pSun->m_pShadowMapInfo;
            int arrGSMCastersCount[MAX_GSM_LODS_NUM];
            memset(arrGSMCastersCount, 0, sizeof(arrGSMCastersCount));
            char szText[256] = "Objects count per shadow map: ";
            for (int nLod = 0; nLod < Get3DEngine()->GetShadowsCascadeCount(NULL) && nLod < MAX_GSM_LODS_NUM; nLod++)
            {
                ShadowMapFrustum*& pLsource = pSMI->pGSM[nLod];
                if (nLod)
                {
                    azstrcat(szText, AZ_ARRAY_SIZE(szText), ", ");
                }

                char* pstr = szText + strlen(szText);
                sprintf_s(pstr, sizeof(szText) - (pstr - szText), "%d", pLsource->m_castersList.Count());
            }

            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, szText);
        }

        for (int nSunInUse = 0; nSunInUse < 2; nSunInUse++)
        {
            if (nSunInUse)
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "WithSun  ListId   FrNum UserNum");
            }
            else
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "NoSun    ListId   FrNum UserNum");
            }

            // TODO: For Nick, check if needed anymore
            //for(ShadowFrustumListsCache::iterator it = m_FrustumsCache[nSunInUse].begin(); it != m_FrustumsCache[nSunInUse].end(); ++it)
            //{
            //  int nListId = (int)it->first;
            //  PodArray<ShadowMapFrustum*> * pList = it->second;

            //  DrawTextRightAligned( fTextPosX, fTextPosY+=fTextStepY,
            //    "%8d %8d %8d",
            //    nListId,
            //    pList->Count(), m_FrustumsCacheUsers[nSunInUse][nListId]);
            //}
        }
    }

    // objects counter
    if (GetCVars()->e_ObjStats)
    {
#define DRAW_OBJ_STATS(_var) DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "%s: %d", (#_var), GetInstCount(_var))

        DRAW_OBJ_STATS(eERType_NotRenderNode);
        DRAW_OBJ_STATS(eERType_Light);
        DRAW_OBJ_STATS(eERType_Cloud);
        DRAW_OBJ_STATS(eERType_FogVolume);
        DRAW_OBJ_STATS(eERType_Decal);
        DRAW_OBJ_STATS(eERType_WaterVolume);
        DRAW_OBJ_STATS(eERType_DistanceCloud);
        DRAW_OBJ_STATS(eERType_VolumeObject);
        DRAW_OBJ_STATS(eERType_Rope);
        DRAW_OBJ_STATS(eERType_PrismObject);
        DRAW_OBJ_STATS(eERType_RenderComponent);
        DRAW_OBJ_STATS(eERType_StaticMeshRenderComponent);
        DRAW_OBJ_STATS(eERType_DynamicMeshRenderComponent);
        DRAW_OBJ_STATS(eERType_SkinnedMeshRenderComponent);
        DRAW_OBJ_STATS(eERType_GameEffect);
        DRAW_OBJ_STATS(eERType_BreakableGlass);

        if (IsObjectTreeReady())
        {
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "--- By list type: ---");
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "  Main:      %d", m_pObjectsTree->GetObjectsCount(eMain));
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "Caster:      %d", m_pObjectsTree->GetObjectsCount(eCasters));
            DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "LigAll:      %d", m_lstStaticLights.Count());
        }

        int nFree = m_LTPRootFree.Count();
        int nUsed = m_LTPRootUsed.Count();
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "RNTmpData(Used+Free): %d + %d = %d (%d KB)",
            nUsed, nFree, nUsed + nFree, (nUsed + nFree) * (int)sizeof(CRNTmpData) / 1024);

        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "COctreeNode::m_arrEmptyNodes.Count() = %d", COctreeNode::m_arrEmptyNodes.Count());
    }

    CCullBuffer* pCB = GetCoverageBuffer();
    if (pCB && GetCVars()->e_CoverageBuffer && GetCVars()->e_CoverageBufferDebug && pCB->TrisWritten())
    {
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY,
            "CB: Write:%3d/%2d Test:%4d/%4d/%3d ZFarM:%.2f ZNearM:%.2f Res:%d OI:%s",
            pCB->TrisWritten(), pCB->ObjectsWritten(),
            pCB->TrisTested(), pCB->ObjectsTested(), pCB->ObjectsTestedAndRejected(),
            pCB->GetZFarInMeters(), pCB->GetZNearInMeters(), pCB->SelRes(),
            pCB->IsOutdooVisible() ? "Out" : "In");
    }

#if defined(INFO_FRAME_COUNTER)
    ++frameCounter;
    DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "Frame #%d", frameCounter);
#endif

    ITimeOfDay* pTimeOfDay = Get3DEngine()->GetTimeOfDay();
    if (GetCVars()->e_TimeOfDayDebug && pTimeOfDay)
    {
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "---------------------------------------");
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "------------ Time of Day  -------------");
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, " ");

        int nVarCount = pTimeOfDay->GetVariableCount();
        for (int v = 0; v < nVarCount; ++v)
        {
            ITimeOfDay::SVariableInfo pVar;
            pTimeOfDay->GetVariableInfo(v, pVar);

            if (pVar.type == ITimeOfDay::TYPE_FLOAT)
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, " %s: %.9f", pVar.displayName, pVar.fValue[0]);
            }
            else
            {
                DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, " %s: %.3f %.3f %.3f", pVar.displayName, pVar.fValue[0], pVar.fValue[1], pVar.fValue[2]);
            }
        }
        DrawTextRightAligned(fTextPosX, fTextPosY += fTextStepY, "---------------------------------------");
    }

#endif
    // We only show memory usage in dev mode.
    if (gEnv->pSystem->IsDevMode())
    {
        if (GetCVars()->e_DisplayMemoryUsageIcon)
        {
            uint64                    nAverageMemoryUsage(0);
            uint64                    nHighMemoryUsage(0);
            uint64                    nCurrentMemoryUsage(0);
            const     uint64  nMegabyte(1024 * 1024);

            // Copied from D3DDriver.cpp, function CD3D9Renderer::RT_EndFrame().
            int                           nIconSize = 16;

            nCurrentMemoryUsage = processMemInfo.TotalPhysicalMemory - processMemInfo.FreePhysicalMemory;

#if defined(_WIN64) || defined(WIN64) || defined(MAC) || defined(LINUX64)
            nAverageMemoryUsage = 3000;
            nHighMemoryUsage = 6000;
            // This is the same value as measured in the editor.
            nCurrentMemoryUsage = processMemInfo.PagefileUsage / nMegabyte;
#elif defined(_WIN32) || defined(LINUX32)
            nAverageMemoryUsage = 800;
            nHighMemoryUsage = 1200;
            // This is the same value as measured in the editor.
            nCurrentMemoryUsage = processMemInfo.PagefileUsage / nMegabyte;
#endif //_WIN32

            ITexture* pRenderTexture(m_ptexIconAverageMemoryUsage);
            if (nCurrentMemoryUsage > nHighMemoryUsage)
            {
                pRenderTexture = m_ptexIconHighMemoryUsage;
            }
            else if (nCurrentMemoryUsage < nAverageMemoryUsage)
            {
                pRenderTexture = m_ptexIconLowMemoryUsage;
            }

            if (pRenderTexture && gEnv->pRenderer)
            {
                float vpWidth = (float)gEnv->pRenderer->GetOverlayWidth(), vpHeight = (float)gEnv->pRenderer->GetOverlayHeight();
                float iconWidth = (float)nIconSize / vpWidth * 800.0f;
                float iconHeight = (float)nIconSize / vpHeight * 600.0f;
                gEnv->pRenderer->Push2dImage((fTextPosX / vpWidth) * 800.0f - iconWidth, ((fTextPosY += nIconSize + 3) / vpHeight) * 600.0f,
                    iconWidth, iconHeight, pRenderTexture->GetTextureID(), 0, 1.0f, 1.0f, 0);
            }
        }
    }
#endif
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
static const float DISPLAY_MEMORY_ROW_MARGIN = 16.0f;
static const float DISPLAY_MEMORY_ROW_HEIGHT = 32.0f;
static const float DISPLAY_MEMORY_ROW_NUMBER_WIDTH = 128.0f;
static const float DISPLAY_MEMORY_ROW_FONT_SCALE = 1.5f;
static const float DISPLAY_MEMORY_COL_LABEL_FONT_SCALE = 1.0f;

static inline void AdjustDisplayMemoryParameters(float& yPos, float& columnInset, float columnWidth, float screenHeight)
{
    int column = (int)(yPos + DISPLAY_MEMORY_ROW_HEIGHT) / (int)screenHeight;
    columnInset += columnWidth * column;
    yPos -= screenHeight * column;
}

static void DisplayMemoryRow(C3DEngine& engine, float columnWidth, float screenHeight, float yPos, float valueA, float valueB, const char* valueBFormat, const ColorF& color, const char* categoryName, const char* subcategoryName = nullptr)
{
    float columnInset = columnWidth - DISPLAY_MEMORY_ROW_MARGIN;
    AdjustDisplayMemoryParameters(yPos, columnInset, columnWidth, screenHeight);
    if (valueA != -1.0f)
    {
        engine.DrawTextRightAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH, yPos, DISPLAY_MEMORY_ROW_FONT_SCALE, color, "%.1fMB", valueA);
    }
    if (valueB != -1.0f)
    {
        engine.DrawTextRightAligned(columnInset, yPos, DISPLAY_MEMORY_ROW_FONT_SCALE, color, valueBFormat, valueB);
    }

    if (subcategoryName)
    {
        static const float MAIN_TEXT_SCALE = 1.5f;
        static const float SUB_TEXT_SCALE = 1.0f;
        static const float SUB_LINE_OFFSET_Y = 16.0f;

        engine.DrawTextLeftAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH * 4, yPos, MAIN_TEXT_SCALE, color, "%s", categoryName);
        engine.DrawTextLeftAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH * 4, yPos + SUB_LINE_OFFSET_Y, SUB_TEXT_SCALE, color, "%s", subcategoryName);
    }
    else
    {
        engine.DrawTextLeftAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH * 4, yPos, DISPLAY_MEMORY_ROW_FONT_SCALE, color, "%s", categoryName);
    }
}

void C3DEngine::DisplayMemoryStatistics()
{
    const ColorF headerColor = ColorF(0.4f, 0.9f, 0.3f, 1.0f);
    const ColorF statisticColor = ColorF(0.4f, 0.9f, 0.9f, 1.0f);
    const ColorF subtotalColor = ColorF(0.4f, 0.3f, 0.9f, 1.0f);
    const ColorF totalColor = ColorF(0.9f, 0.9f, 0.9f, 1.0f);
    const ColorF labelColor = ColorF(0.4f, 0.3f, 0.3f, 1.0f);

    const float screenHeight = (float)m_pRenderer->GetHeight();

    if (GetCVars()->e_MemoryProfiling == 1)
    {
        const float columnWidth = (float)(m_pRenderer->GetWidth() / 2);
        float columnInset = columnWidth - DISPLAY_MEMORY_ROW_MARGIN;

        float memoryYPos = DISPLAY_MEMORY_ROW_HEIGHT;
        float memoryYPosStepSize = DISPLAY_MEMORY_ROW_HEIGHT;

        // Add column labels and header
        this->DrawTextRightAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH, memoryYPos, DISPLAY_MEMORY_COL_LABEL_FONT_SCALE, labelColor, "Allocated");
        this->DrawTextRightAligned(columnInset, memoryYPos, DISPLAY_MEMORY_COL_LABEL_FONT_SCALE, labelColor, "No. Allocations");
        DisplayMemoryRow(*this, columnWidth, screenHeight, memoryYPos, -1.0f, -1.0f, "%.1fMB", headerColor, "VRAM Usage");
        memoryYPos += (memoryYPosStepSize * 0.5f);

        float totalTrackedGPUAlloc = 0.0f;

        // Print the memory usage of each major VRAM category and each subcategory
        for (int category = 0; category < Render::Debug::VRAM_CATEGORY_NUMBER_CATEGORIES; ++category)
        {
            float categorySubTotal = 0.0f;
            AZStd::string categoryName;

            for (int subcategory = 0; subcategory < Render::Debug::VRAM_SUBCATEGORY_NUMBER_SUBCATEGORIES; ++subcategory)
            {
                AZStd::string subcategoryName;
                size_t numberBytesAllocated = 0;
                size_t numberAllocations = 0;
                EBUS_EVENT(Render::Debug::VRAMDrillerBus, GetCurrentVRAMStats, static_cast<Render::Debug::VRAMAllocationCategory>(category),
                    static_cast<Render::Debug::VRAMAllocationSubcategory>(subcategory), categoryName, subcategoryName, numberBytesAllocated, numberAllocations);

                if (numberAllocations != 0)
                {
                    float numMBallocated = numberBytesAllocated / (1024.0f * 1024.0f);
                    DisplayMemoryRow(*this, columnWidth, screenHeight, memoryYPos, numMBallocated, (float)numberAllocations, "%.0f", statisticColor, categoryName.c_str(), subcategoryName.c_str());

                    memoryYPos += memoryYPosStepSize;
                    totalTrackedGPUAlloc += numMBallocated;
                    categorySubTotal += numMBallocated;
                }
            }
            if (categorySubTotal > 0.0f)
            {
                float yPos = memoryYPos;
                AdjustDisplayMemoryParameters(yPos, columnInset, columnWidth, screenHeight);
                DrawTextLeftAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH * 4, yPos, DISPLAY_MEMORY_ROW_FONT_SCALE, subtotalColor, "%s Subtotal", categoryName.c_str());
                DrawTextRightAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH, yPos, DISPLAY_MEMORY_ROW_FONT_SCALE, subtotalColor, "%.1fMB", categorySubTotal);
                memoryYPos += (memoryYPosStepSize * 0.5f);
            }
        }

        float allocatedVideoMemoryMB = -1.0f, reservedVideoMemoryMB = -1.0f;

#if defined(AZ_PLATFORM_PROVO)
        size_t allocatedVideoMemoryBytes = 0, reservedVideoMemoryBytes = 0;
        VirtualAllocator::QueryVideoMemory(allocatedVideoMemoryBytes, reservedVideoMemoryBytes);
        allocatedVideoMemoryMB = static_cast<float>(allocatedVideoMemoryBytes) / (1024.0f * 1024.0f);
        reservedVideoMemoryMB = static_cast<float>(reservedVideoMemoryBytes) / (1024.0f * 1024.0f);
#else
        // Non PROVO platforms just sum up the tracked allocations
        allocatedVideoMemoryMB = totalTrackedGPUAlloc;
#endif

        DrawTextLeftAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH * 4, memoryYPos, DISPLAY_MEMORY_ROW_FONT_SCALE, totalColor, "Total");
        if (reservedVideoMemoryMB != -1.0f)
        {
            DrawTextRightAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH * 1, memoryYPos, DISPLAY_MEMORY_ROW_FONT_SCALE, totalColor, "%.1fMB/%.1fMB", allocatedVideoMemoryMB, reservedVideoMemoryMB);
            memoryYPos += (memoryYPosStepSize * 0.5f);
        }
        else
        {
            DrawTextRightAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH * 1, memoryYPos, DISPLAY_MEMORY_ROW_FONT_SCALE, totalColor, "%.1fMB", allocatedVideoMemoryMB);
            memoryYPos += (memoryYPosStepSize * 0.5f);
        }

        // Spacer
        memoryYPos += (memoryYPosStepSize * 0.5f);

        // Add column labels and header
        this->DrawTextRightAligned(columnInset - DISPLAY_MEMORY_ROW_NUMBER_WIDTH, memoryYPos, DISPLAY_MEMORY_COL_LABEL_FONT_SCALE, labelColor, "Allocated");
        this->DrawTextRightAligned(columnInset, memoryYPos, DISPLAY_MEMORY_COL_LABEL_FONT_SCALE, labelColor, "Capacity");
        DisplayMemoryRow(*this, columnWidth, screenHeight, memoryYPos, -1.0f, -1.0f, "%.1fMB", headerColor, "CPU Memory Usage");
        memoryYPos += (memoryYPosStepSize * 0.5f);

        float totalTrackedCPUAlloc = 0.0f;
        float totalCapacityCPUAlloc = 0.0f;

        AZ::AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();
        const size_t allocatorCount = allocatorManager.GetNumAllocators();
        AZStd::map<AZ::IAllocatorAllocate*, AZ::IAllocator*> existingAllocators;
        AZStd::map<AZ::IAllocatorAllocate*, AZ::IAllocator*> sourcesToAllocators;

        // Build a mapping of original allocator sources to their allocators
        for (int i = 0; i < allocatorCount; ++i)
        {
            AZ::IAllocator* allocator = allocatorManager.GetAllocator(i);
            sourcesToAllocators.emplace(allocator->GetOriginalAllocationSource(), allocator);
        }

        // Group up any allocators under this size
        static float smallAllocatorCapacityMaxMB = 10.0f;
        float smallAllocatorsTotalCapacityMB = 0.0f;
        float smallAllocatorsTotalAllocatedMB = 0.0f;

        for (int i = 0; i < allocatorCount; ++i)
        {
            AZ::IAllocator* allocator = allocatorManager.GetAllocator(i);
            AZ::IAllocatorAllocate* source = allocator->GetAllocationSource();
            AZ::IAllocatorAllocate* originalSource = allocator->GetOriginalAllocationSource();
            AZ::IAllocatorAllocate* schema = allocator->GetSchema();
            AZ::IAllocator* alias = (source != originalSource) ? sourcesToAllocators[source] : nullptr;

            if (schema && !alias)
            {
                // Check to see if this allocator's source maps to another allocator
                // Need to check both the schema and the allocator itself, as either one might be used as the alias depending on how it's implemented
                AZStd::array<AZ::IAllocatorAllocate*, 2> checkAllocators = { { schema, allocator->GetAllocationSource() } };

                for (AZ::IAllocatorAllocate* check : checkAllocators)
                {
                    auto existing = existingAllocators.emplace(check, allocator);

                    if (!existing.second)
                    {
                        alias = existing.first->second;
                        // Do not break out of the loop as we need to add to the map for all entries
                    }
                }
            }

            if (!alias)
            {
                static const AZ::IAllocator* OS_ALLOCATOR = &AZ::AllocatorInstance<AZ::OSAllocator>::GetAllocator();
                float allocatedMB = (float)source->NumAllocatedBytes() / (1024.0f * 1024.0f);
                float capacityMB = (float)source->Capacity() / (1024.0f * 1024.0f);

                totalTrackedCPUAlloc += allocatedMB;
                totalCapacityCPUAlloc += capacityMB;

                // Skip over smaller allocators so the display is readable.
                if (capacityMB < smallAllocatorCapacityMaxMB)
                {
                    smallAllocatorsTotalCapacityMB += capacityMB;
                    smallAllocatorsTotalAllocatedMB += allocatedMB;
                    continue;
                }

                if (allocator == OS_ALLOCATOR)
                {
                    // Need to special case the OS allocator because its capacity is a made-up number. Better to just use the allocated amount, it will hopefully be small anyway.
                    capacityMB = allocatedMB;
                }

                DisplayMemoryRow(*this, columnWidth, screenHeight, memoryYPos, allocatedMB, capacityMB, "%.1fMB", statisticColor, allocator->GetName(), allocator->GetDescription());

                memoryYPos += memoryYPosStepSize;
            }
        }

        if (smallAllocatorCapacityMaxMB > 0.0f)
        {
            AZStd::string subText = AZStd::string::format("Allocators smaller than %.0f MB", smallAllocatorCapacityMaxMB);
            DisplayMemoryRow(*this, columnWidth, screenHeight, memoryYPos, smallAllocatorsTotalAllocatedMB, smallAllocatorsTotalCapacityMB, "%.1fMB", statisticColor, "All Small Allocators", subText.c_str());
            memoryYPos += memoryYPosStepSize;
        }

        DisplayMemoryRow(*this, columnWidth, screenHeight, memoryYPos, totalTrackedCPUAlloc, totalCapacityCPUAlloc, "%.1fMB", totalColor, "Total");
        memoryYPos += (memoryYPosStepSize * 0.5f);
    }
    else if (GetCVars()->e_MemoryProfiling == 2)
    {
        const float columnWidth = (float)(m_pRenderer->GetWidth() / 2);

        float memoryYPos = DISPLAY_MEMORY_ROW_HEIGHT;
        float memoryYPosStepSize = DISPLAY_MEMORY_ROW_HEIGHT;

        AZ::AllocatorManager& allocatorManager = AZ::AllocatorManager::Instance();
        const size_t allocatorCount = allocatorManager.GetNumAllocators();
        AZStd::map<AZ::IAllocatorAllocate*, AZ::IAllocator*> existingAllocators;
        AZStd::map<AZ::IAllocatorAllocate*, AZ::IAllocator*> sourcesToAllocators;

        // Build a mapping of original allocator sources to their allocators
        for (int i = 0; i < allocatorCount; ++i)
        {
            AZ::IAllocator* allocator = allocatorManager.GetAllocator(i);
            sourcesToAllocators.emplace(allocator->GetOriginalAllocationSource(), allocator);
        }

        for (int i = 0; i < allocatorCount; ++i)
        {
            AZ::IAllocator* allocator = allocatorManager.GetAllocator(i);
            AZ::IAllocatorAllocate* source = allocator->GetAllocationSource();
            AZ::IAllocatorAllocate* originalSource = allocator->GetOriginalAllocationSource();
            AZ::IAllocatorAllocate* schema = allocator->GetSchema();
            AZ::IAllocator* alias = (source != originalSource) ? sourcesToAllocators[source] : nullptr;

            if (schema && !alias)
            {
                // Check to see if this allocator's source maps to another allocator
                // Need to check both the schema and the allocator itself, as either one might be used as the alias depending on how it's implemented
                AZStd::array<AZ::IAllocatorAllocate*, 2> checkAllocators = { { schema, allocator->GetAllocationSource() } };

                for (AZ::IAllocatorAllocate* check : checkAllocators)
                {
                    auto existing = existingAllocators.emplace(check, allocator);

                    if (!existing.second)
                    {
                        alias = existing.first->second;
                        // Do not break out of the loop as we need to add to the map for all entries
                    }
                }
            }

            if (alias)
            {
                float columnInset = columnWidth - DISPLAY_MEMORY_ROW_MARGIN;
                float yPos = memoryYPos;
                AdjustDisplayMemoryParameters(yPos, columnInset, columnWidth, screenHeight);
                DrawTextRightAligned(columnInset, yPos, DISPLAY_MEMORY_ROW_FONT_SCALE, statisticColor, "%s => %s", allocator->GetName(), alias->GetName());
                memoryYPos += (memoryYPosStepSize * 0.5f);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void C3DEngine::SetupDistanceFog()
{
    FUNCTION_PROFILER_3DENGINE;

    GetRenderer()->SetFogColor(ColorF(m_vFogColor.x, m_vFogColor.y, m_vFogColor.z, 1.0f));
    GetRenderer()->EnableFog(GetCVars()->e_Fog > 0);
}

void C3DEngine::ScreenShotHighRes([[maybe_unused]] CStitchedImage* pStitchedImage, [[maybe_unused]] const int nRenderFlags, [[maybe_unused]] const SRenderingPassInfo& passInfo, [[maybe_unused]] uint32 SliceCount, [[maybe_unused]] f32 fTransitionSize)
{
#if defined(WIN32) || defined(WIN64) || defined(MAC)

    //If the requested format is TGA we want the framebuffer in BGR format; otherwise we want RGB
    const char* szExtension = GetCVars()->e_ScreenShotFileFormat->GetString();
    bool BGRA = (azstricmp(szExtension, "tga") == 0) ? true : false;

    // finish frame started by system
    GetRenderer()->EndFrame();

    // The occlusion system does not like being restarted mid-frame like this. Disable it for
    // the screenshot system.
    AZ::s32 statObjBufferRenderTasks = GetCVars()->e_StatObjBufferRenderTasks;
    GetCVars()->e_StatObjBufferRenderTasks = 0;

    GetConsole()->SetScrollMax(0);

    const uint32  ScreenWidth =   GetRenderer()->GetWidth();
    const uint32  ScreenHeight    =   GetRenderer()->GetHeight();
    uint32* pImage = new uint32[ScreenWidth * ScreenHeight];
    for (uint32 yy = 0; yy < SliceCount; yy++)
    {
        for (uint32 xx = 0; xx < SliceCount; xx++)
        {
            const int BlendX  =   (xx * 2) / SliceCount;
            const int BlendY  =   (yy * 2) / SliceCount;
            const int x   =   (((xx * 2) % SliceCount) & ~1) + BlendX;
            const int y   =   (((yy * 2) % SliceCount) & ~1) + BlendY;
            const int reverseX = SliceCount - 1 - x;
            const int reverseY = SliceCount - 1 - y;

            const float halfTransitionSize = fTransitionSize * 0.5f;
            const float sliceCountF = static_cast<float>(SliceCount);

            // start new frame and define needed tile
            const f32 ScreenScale = 1.0f / ((1.0f / sliceCountF) * (1.0f + fTransitionSize));

            GetRenderer()->BeginFrame();

            // This has to happen after BeginFrame(), because BeginFrame increments the frame counter, and SRenderingPassInfo
            // pulls from that counter in the constructor. Individual render nodes track the frame they were last rendered with
            // and will bail if the same frame is rendered twice.
            SRenderingPassInfo screenShotPassInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(passInfo.GetCamera());
            PrintMessage("Rendering tile %d of %d ... ", xx + yy * SliceCount + 1, SliceCount * SliceCount);

            const float normalizedX = ((static_cast<f32>(reverseX) - halfTransitionSize) / sliceCountF);
            const float normalizedY = ((static_cast<f32>(reverseY) - halfTransitionSize) / sliceCountF);

                GetRenderer()->SetRenderTile(
                    ScreenScale * normalizedX,
                    ScreenScale * normalizedY,
                    ScreenScale, ScreenScale);

            UpdateRenderingCamera("ScreenShotHighRes", screenShotPassInfo);

            RenderInternal(nRenderFlags, screenShotPassInfo, "ScreenShotHighRes");

            // Make sure we've composited to the final back buffer.
            GetRenderer()->SwitchToNativeResolutionBackbuffer();

            GetRenderer()->EndFrame();

            PrintMessagePlus("reading frame buffer ... ");

            GetRenderer()->ReadFrameBufferFast(pImage, ScreenWidth, ScreenHeight, BGRA);
            pStitchedImage->RasterizeRect(pImage, ScreenWidth, ScreenHeight, x, y, fTransitionSize,
                fTransitionSize > 0.0001f && BlendX,
                fTransitionSize > 0.0001f && BlendY);

            PrintMessagePlus("ok");
        }
    }
    delete[] pImage;

    GetCVars()->e_StatObjBufferRenderTasks = statObjBufferRenderTasks;

    // re-start frame so system can safely finish it
    GetRenderer()->BeginFrame();

    // restore initial state
    GetRenderer()->SetViewport(0, 0, GetRenderer()->GetWidth(), GetRenderer()->GetHeight());
    GetConsole()->SetScrollMax(300);
    GetRenderer()->SetRenderTile();

    PrintMessagePlus(" ok");
#endif // #if defined(WIN32) || defined(WIN64)
}



bool C3DEngine::ScreenShotMap([[maybe_unused]] CStitchedImage* pStitchedImage,
    [[maybe_unused]] const int                         nRenderFlags,
    [[maybe_unused]] const SRenderingPassInfo& passInfo,
    [[maybe_unused]] const uint32                  SliceCount,
    [[maybe_unused]] const f32                         fTransitionSize)
{
#if defined(WIN32) || defined(WIN64) || defined(MAC)

    const f32     fTLX   = GetCVars()->e_ScreenShotMapCenterX - GetCVars()->e_ScreenShotMapSizeX + fTransitionSize * GetRenderer()->GetWidth();
    const f32     fTLY   = GetCVars()->e_ScreenShotMapCenterY - GetCVars()->e_ScreenShotMapSizeY + fTransitionSize * GetRenderer()->GetHeight();
    const f32     fBRX   = GetCVars()->e_ScreenShotMapCenterX + GetCVars()->e_ScreenShotMapSizeX + fTransitionSize * GetRenderer()->GetWidth();
    const f32     fBRY   = GetCVars()->e_ScreenShotMapCenterY + GetCVars()->e_ScreenShotMapSizeY + fTransitionSize * GetRenderer()->GetHeight();
    const f32     Height = GetCVars()->e_ScreenShotMapCamHeight;
    const int     Orient = GetCVars()->e_ScreenShotMapOrientation;

    const char* SettingsFileName = GetLevelFilePath("ScreenshotMap.Settings");

    AZ::IO::HandleType metaFileHandle = gEnv->pCryPak->FOpen(SettingsFileName, "wt");
    if (metaFileHandle != AZ::IO::InvalidHandle)
    {
        char Data[1024 * 8];
        snprintf(Data, sizeof(Data), "<Map CenterX=\"%f\" CenterY=\"%f\" SizeX=\"%f\" SizeY=\"%f\" Height=\"%f\"  Quality=\"%d\" Orientation=\"%d\" />",
            GetCVars()->e_ScreenShotMapCenterX,
            GetCVars()->e_ScreenShotMapCenterY,
            GetCVars()->e_ScreenShotMapSizeX,
            GetCVars()->e_ScreenShotMapSizeY,
            GetCVars()->e_ScreenShotMapCamHeight,
            GetCVars()->e_ScreenShotQuality,
            GetCVars()->e_ScreenShotMapOrientation);
        string data(Data);
        gEnv->pCryPak->FWrite(data.c_str(), data.size(), metaFileHandle);
        gEnv->pCryPak->FClose(metaFileHandle);
    }

    // This bit is necessary because we don't have a way to render the world using an orthographic projection. This is doing
    // a hacky orthographic projection by shifting the camera up to a sufficient height to fake it. To preserve depth range
    // we define a maximum range then then fit the near / far planes to extend [-HeightRangeMax, HeightRangeMax] along Z (which is the up axis).
    const float HeightRangeMax = 4096;
    const float HeightRangeMaxDiv2 = HeightRangeMax / 2.0f;

    const float NearClip = max(Height - HeightRangeMaxDiv2, 1.0f);
    const float FarClip  = max(Height + HeightRangeMaxDiv2, HeightRangeMax);

    CCamera cam = passInfo.GetCamera();
    Matrix34 tmX, tmY;
    float xrot = -gf_PI * 0.5f;
    float yrot = Orient == 0 ? -gf_PI * 0.5f : -0.0f;
    tmX.SetRotationX(xrot);
    tmY.SetRotationY(yrot);
    Matrix34 tm   =   tmX * tmY;
    tm.SetTranslation(Vec3((fTLX + fBRX) * 0.5f, (fTLY + fBRY) * 0.5f, Height));
    cam.SetMatrix(tm);

    const f32 AngleX  =   atanf(((fBRX - fTLX) * 0.5f) / Height);
    const f32 AngleY  =   atanf(((fBRY - fTLY) * 0.5f) / Height);

    ICVar* r_drawnearfov = GetConsole()->GetCVar("r_DrawNearFoV");
    assert(r_drawnearfov);
    const f32 drawnearfov_backup = r_drawnearfov->GetFVal();
    const f32 ViewingSize =   (float)min(cam.GetViewSurfaceX(), cam.GetViewSurfaceZ());
    if (max(AngleX, AngleY) <= 0)
    {
        return false;
    }
    cam.SetFrustum((int)ViewingSize, (int)ViewingSize, max(0.001f, max(AngleX, AngleY) * 2.f), NearClip, FarClip);
    r_drawnearfov->Set(-1);
    ScreenShotHighRes(pStitchedImage, nRenderFlags, SRenderingPassInfo::CreateTempRenderingInfo(cam, passInfo), SliceCount, fTransitionSize);
    r_drawnearfov->Set(drawnearfov_backup);

    return true;
#else       // #if defined(WIN32) || defined(WIN64)
    return false;
#endif  // #if defined(WIN32) || defined(WIN64)
}


bool C3DEngine::ScreenShotPanorama([[maybe_unused]] CStitchedImage* pStitchedImage, [[maybe_unused]] const int nRenderFlags, [[maybe_unused]] const SRenderingPassInfo& passInfo, [[maybe_unused]] uint32 SliceCount, [[maybe_unused]] f32 fTransitionSize)
{
#if defined(WIN32) || defined(WIN64) || defined(MAC)

    //If the requested format is TGA we want the framebuffer in BGR format; otherwise we want RGB
    const char* szExtension = GetCVars()->e_ScreenShotFileFormat->GetString();
    bool BGRA = (azstricmp(szExtension, "tga") == 0) ? true : false;

    // finish frame started by system
    GetRenderer()->EndFrame();

    float r_drawnearfov_backup = -1;
    ICVar* r_drawnearfov = GetConsole()->GetCVar("r_DrawNearFoV");
    assert(r_drawnearfov);

    r_drawnearfov_backup = r_drawnearfov->GetFVal();
    r_drawnearfov->Set(-1);     // means the fov override should be switched off

    // The occlusion system does not like being restarted mid-frame like this. Disable it for
    // the screenshot system.
    AZ::s32 statObjBufferRenderTasks = GetCVars()->e_StatObjBufferRenderTasks;
    GetCVars()->e_StatObjBufferRenderTasks = 0;

    GetTimer()->EnableTimer(false);

    uint32* pImage = new uint32[GetRenderer()->GetWidth() * GetRenderer()->GetHeight()];

    for (int iSlice = SliceCount - 1; iSlice >= 0; --iSlice)
    {
        if (iSlice == 0)                                          // the last one should do eye adaption
        {
            GetTimer()->EnableTimer(true);
        }

        GetRenderer()->BeginFrame();

        Matrix33 rot;
        rot.SetIdentity();

        float fAngle = pStitchedImage->GetSliceAngle(iSlice);

        rot.SetRotationZ(fAngle);

        CCamera cam = passInfo.GetCamera();

        Matrix34 tm = cam.GetMatrix();
        tm = tm * rot;
        tm.SetTranslation(passInfo.GetCamera().GetPosition());
        cam.SetMatrix(tm);

        cam.SetFrustum(cam.GetViewSurfaceX(), cam.GetViewSurfaceZ(), pStitchedImage->m_fPanoramaShotVertFOV, cam.GetNearPlane(), cam.GetFarPlane(), cam.GetPixelAspectRatio());

        SRenderingPassInfo screenShotPassInfo = SRenderingPassInfo::CreateGeneralPassRenderingInfo(cam);

        UpdateRenderingCamera("ScreenShotPanorama", screenShotPassInfo);

        // render scene
        RenderInternal(nRenderFlags, screenShotPassInfo, "ScreenShotPanorama");

        // Make sure we've composited to the final back buffer.
        GetRenderer()->SwitchToNativeResolutionBackbuffer();

        GetRenderer()->ReadFrameBufferFast(pImage, GetRenderer()->GetWidth(), GetRenderer()->GetHeight(), BGRA);

        GetRenderer()->EndFrame();                          // show last frame (from direction)

        const bool bFadeBorders = (iSlice + 1) * 2 <= (int)SliceCount;

        PrintMessage("PanoramaScreenShot %d/%d FadeBorders:%c (id: %d/%d)", iSlice + 1, SliceCount, bFadeBorders ? 't' : 'f', GetRenderer()->GetFrameID(false), GetRenderer()->GetFrameID(true));

        pStitchedImage->RasterizeCylinder(pImage, GetRenderer()->GetWidth(), GetRenderer()->GetHeight(), iSlice + 1, bFadeBorders);

        if (GetCVars()->e_ScreenShotQuality < 0)  // to debug FadeBorders
        {
            if (iSlice * 2 == SliceCount)
            {
                pStitchedImage->Clear();
                PrintMessage("PanoramaScreenShot clear");
            }
        }
    }
    delete [] pImage;

    r_drawnearfov->Set(r_drawnearfov_backup);
    GetCVars()->e_StatObjBufferRenderTasks = statObjBufferRenderTasks;

    // re-start frame so system can safely finish it
    GetRenderer()->BeginFrame();

    return true;
#else       // #if defined(WIN32) || defined(WIN64)
    return false;
#endif  // #if defined(WIN32) || defined(WIN64)
}



void C3DEngine::SetupClearColor()
{
    FUNCTION_PROFILER_3DENGINE;

    bool bCameraInOutdoors = m_pVisAreaManager && !m_pVisAreaManager->m_pCurArea && !(m_pVisAreaManager->m_pCurPortal && m_pVisAreaManager->m_pCurPortal->m_lstConnections.Count() > 1);
    GetRenderer()->SetClearColor(bCameraInOutdoors ? m_vFogColor : Vec3(0, 0, 0));
}

void C3DEngine::FillDebugFPSInfo(SDebugFPSInfo& info)
{
    size_t c = 0;
    float average = 0.0f, min = 0.0f, max = 0.0f;
    const float clampFPS = 200.0f;
    for (size_t i = 0, end = arrFPSforSaveLevelStats.size(); i < end; ++i)
    {
        if (arrFPSforSaveLevelStats[i] > 1.0f && arrFPSforSaveLevelStats[i] < clampFPS)
        {
            ++c;
            average += arrFPSforSaveLevelStats[i];
        }
    }

    if (c)
    {
        average /= (float)c;
    }

    int minc = 0, maxc = 0;
    for (size_t i = 0, end = arrFPSforSaveLevelStats.size(); i < end; ++i)
    {
        if (arrFPSforSaveLevelStats[i] > average && arrFPSforSaveLevelStats[i] < clampFPS)
        {
            ++maxc;
            max += arrFPSforSaveLevelStats[i];
        }

        if (arrFPSforSaveLevelStats[i] < average && arrFPSforSaveLevelStats[i] < clampFPS)
        {
            ++minc;
            min += arrFPSforSaveLevelStats[i];
        }
    }

    if (minc == 0)
    {
        minc = 1;
    }
    if (maxc == 0)
    {
        maxc = 1;
    }

    info.fAverageFPS = average;
    info.fMinFPS = min / (float)minc;
    info.fMaxFPS = max / (float)maxc;
}
