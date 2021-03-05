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

#include "RenderDll_precompiled.h"
#include "CImage.h"
#include <CryPath.h>
#include "DDSImage.h"
#include <AzFramework/Asset/AssetSystemBus.h>
#include "IResourceCompilerHelper.h"

CImageFile::CImageFile(const string& filename)
    : m_FileName(filename)
{
    m_nRefCount = 0;
    m_pByteImage[0] = NULL;
    m_pByteImage[1] = NULL;
    m_pByteImage[2] = NULL;
    m_pByteImage[3] = NULL;
    m_pByteImage[4] = NULL;
    m_pByteImage[5] = NULL;

    m_eError = eIFE_OK;
    m_eFormat = eTF_Unknown;
    m_eTileMode = eTM_None;
    m_NumMips = 0;
    m_NumPersistentMips = 0;
    m_Flags = 0;
    m_ImgSize = 0;
    m_Depth = 1;
    m_nStartSeek = 0;
    m_Sides = 1;

    m_pStreamState = NULL;
}

CImageFile::~CImageFile()
{
    mfAbortStreaming();

    for (int i = 0; i < 6; i++)
    {
        mfFree_image(i);
    }
}

void CImageFile::mfSet_dimensions(const int w, const int h)
{
    m_Width = w;
    m_Height = h;
}

void CImageFile::mfSet_error(const EImFileError error, const char* detail)
{
    m_eError = error;
    if (detail)
    {
        TextureWarning(m_FileName.c_str(), detail);
    }
}

namespace
{
    struct DDSCallback
        : public IImageFileStreamCallback
    {
        DDSCallback()
            : ok(false)
        {
        }
        virtual void OnImageFileStreamComplete(CImageFile* pImFile)
        {
            ok = pImFile != NULL;
            waitEvent.Set();
        };
        CryEvent waitEvent;
        volatile bool ok;
    };

    //This function return the appropriate replacement texture based on whether the source asset is missing,compiling etc and also
    //takes into account whether the source asset was a cubemap,alpha texture etc.
    const AZStd::string GetMissingTextureFileName(AZStd::string& inputFile, AzFramework::AssetSystem::AssetStatus status)
    {
        using namespace AzFramework::AssetSystem;

        CRY_ASSERT(status != AssetStatus_Compiled);
        AZStd::string sFileToLoad = "EngineAssets/TextureMsg/";
        AZStd::string prefixName = "";
        //Checking status of the source asset
        if (status == AssetStatus_Missing)
        {
            prefixName = "NotFound";
        }
        else if (status == AssetStatus_Compiling || status == AssetStatus_Queued)
        {
            prefixName = "TextureCompiling";
        }
        else if (status == AssetStatus_Failed)
        {
            prefixName = "RCError";
        }
        else // fallback case.
        {
            prefixName = "NotFound";
        }

        if (inputFile.find("_ddna.") != string::npos)
        {
            sFileToLoad.append(prefixName + "_ddna.dds");
        }
        else if (inputFile.find("_ddn.") != string::npos)
        {
            sFileToLoad.append(prefixName + "_ddn.dds");
        }
        else if (inputFile.find("_a.") != string::npos)
        {
            sFileToLoad.append(prefixName + "_a.dds");
        }
        else if (inputFile.find("_cm.") != string::npos)
        {
            sFileToLoad.append(prefixName + "_cm.dds");
        }
        else if (inputFile.find("_cm_diff.") != string::npos)
        {
            sFileToLoad.append(prefixName + "_cm_diff.dds");
        }
        else
        {
            sFileToLoad.append(prefixName + ".dds");
        }

        if (!gEnv->pCryPak->IsFileExist(sFileToLoad.c_str()))
        {
            //if missing texture is not present than we send a request to asset processor to process it
            AssetStatus assetStatus = AssetStatus_Unknown;
            EBUS_EVENT_RESULT(assetStatus, AzFramework::AssetSystemRequestBus, CompileAssetSync, sFileToLoad);

            if (assetStatus != AssetStatus_Compiled)
            {
                //if we are here ,it means we were not able to compile both the source asset as well the replacement asset
                return AZStd::string();
            }
        }

        return sFileToLoad;
    }


    //Textures in texturemsg,code coverage and editor folder need to be compiled immediately
    bool DoesTextureRequireImmediateCompilation(const AZStd::string& fileName)
    {
        if (fileName.find("/texturemsg/") != string::npos || fileName.find("/codecoverage/") != string::npos || fileName.find("/editor/") != string::npos)
        {
            return true;
        }

        return false;
    }
}

_smart_ptr<CImageFile> CImageFile::mfLoad_file(const string& filename, const uint32 nFlags)
{
    AZStd::string sFileToLoad;
    {
        char buffer[512];
        IResourceCompilerHelper::GetOutputFilename(filename, buffer, sizeof(buffer));   // change filename: e.g. instead of TIF, pass DDS
        sFileToLoad = buffer;
    }
    AZStd::string originalFile(sFileToLoad);

    bool fileIsMissing = false;
    if (gEnv->pCryPak)
    {
        if (!gEnv->pCryPak->IsFileExist(sFileToLoad.c_str()))
        {
            fileIsMissing = true;
        }
    }
    else
    {
        if (!gEnv->pFileIO->Exists(sFileToLoad.c_str()))
        {
            fileIsMissing = true;
        }
    }

    if (fileIsMissing)
    {
        using namespace AzFramework::AssetSystem;
        AssetStatus status = AssetStatus_Unknown;
        // if the texture has been requested with no fallback support then we must try our best to compile it asap, blocking
        if (CRenderer::CV_r_texBlockOnLoad || (nFlags & FIM_NOFALLBACKS) || DoesTextureRequireImmediateCompilation(originalFile))
        {
            EBUS_EVENT_RESULT(status, AzFramework::AssetSystemRequestBus, CompileAssetSync, originalFile);
        }
        else
        {
            EBUS_EVENT_RESULT(status, AzFramework::AssetSystemRequestBus, GetAssetStatus, originalFile);
        }


        if (status != AssetStatus_Compiled)
        {
            if (nFlags & FIM_NOFALLBACKS) // do not return a fallback image.
            {
                return _smart_ptr<CImageFile>();
            }
            if (status == AssetStatus_Missing)
            {
                TextureWarning(originalFile.c_str(), "Texture file is missing: '%s'", originalFile.c_str());
            }
            else if (status == AssetStatus_Failed)
            {
                TextureWarning(originalFile.c_str(), "Failed to compile texture: '%s'", originalFile.c_str());
            }
            sFileToLoad = GetMissingTextureFileName(originalFile, status);
            if (sFileToLoad.empty())
            {
                //We were not able to replace the input texture file with a substitute texture file
                CryLog("No Substitute Texture found for the file : %s", originalFile.c_str());
            }
        }
    }

    const char* ext = PathUtil::GetExt(sFileToLoad.c_str());
    _smart_ptr<CImageFile> pImageFile;

    // Try DDS first
    if (!strcmp(ext, "dds"))
    {
        pImageFile = new CImageDDSFile(sFileToLoad.c_str(), nFlags);

        if (fileIsMissing)
        {
            // masquerade the file as the original one so if the original changes, we reload.
            pImageFile->mfChange_Filename(originalFile.c_str());
            pImageFile->m_bIsImageMissing = true;
        }
    }
    else
    {
#ifndef _RELEASE
        ITextureLoadHandler* pTextureHandler = nullptr;

        //suppress warning if a texture handler will be able to load this image later
        if (I3DEngine* p3DEngine = gEnv->p3DEngine)
        {
            pTextureHandler = p3DEngine->GetTextureLoadHandlerForImage(filename.c_str());
        }

        if (!pTextureHandler)
        {
            TextureWarning(sFileToLoad.c_str(), "Unsupported texture extension '%s': '%s'", ext, filename.c_str());
        }
#endif
    }

    if (pImageFile && pImageFile->mfGet_error() != eIFE_OK)
    {
        return _smart_ptr<CImageFile>();
    }

    return pImageFile;
}

_smart_ptr<CImageFile> CImageFile::mfLoad_mem(const string& filename, void* data, int width, int height, ETEX_Format format, int numMips, const uint32 nFlags)
{
    _smart_ptr<CImageFile> pImageFile = new CImageFile(filename);

    pImageFile->m_Width = width;
    pImageFile->m_Height = height;
    pImageFile->m_NumMips = numMips;
    pImageFile->m_eFormat = format;
    pImageFile->m_FileName = filename;
    pImageFile->m_Flags = nFlags;
    pImageFile->m_ImgSize =
        CTexture::TextureDataSize(width, height, pImageFile->m_Depth, numMips, 1, format, pImageFile->m_eTileMode);

    pImageFile->m_pByteImage[0] = static_cast<byte*>(data);

    if (pImageFile && pImageFile->mfGet_error() != eIFE_OK)
    {
        return _smart_ptr<CImageFile>();
    }

    return pImageFile;
}

_smart_ptr<CImageFile> CImageFile::mfStream_File(const string& filename, const uint32 nFlags, IImageFileStreamCallback* pCallback)
{
    string sFileToLoad;
    {
        char buffer[512];
        IResourceCompilerHelper::GetOutputFilename(filename, buffer, sizeof(buffer));   // change filename: e.g. instead of TIF, pass DDS
        sFileToLoad = buffer;
    }

    const char* ext = PathUtil::GetExt(sFileToLoad);
    _smart_ptr<CImageFile> pImageFile;

    // Try DDS first
    if (!strcmp(ext, "dds"))
    {
        CImageDDSFile* pDDS = new CImageDDSFile(sFileToLoad);
        pImageFile = pDDS;
        pDDS->Stream(nFlags, pCallback);
    }
    else
    {
        TextureWarning(sFileToLoad.c_str(), "Unsupported texture extension '%s'", ext);
    }

    return pImageFile;
}

void CImageFile::mfFree_image(const int nSide)
{
    SAFE_DELETE_ARRAY(m_pByteImage[nSide]);
}

byte* CImageFile::mfGet_image(const int nSide)
{
    if (!m_pByteImage[nSide] && m_ImgSize)
    {
        m_pByteImage[nSide] = new byte[m_ImgSize];
    }
    return m_pByteImage[nSide];
}

void CImageFile::mfAbortStreaming()
{
    if (m_pStreamState)
    {
        for (int i = 0; i < m_pStreamState->MaxStreams; ++i)
        {
            if (m_pStreamState->m_pStreams[i])
            {
                m_pStreamState->m_pStreams[i]->Abort();
            }
        }
        delete m_pStreamState;
        m_pStreamState = NULL;
    }
}

DDSSplitted::DDSDesc CImageFile::mfGet_DDSDesc() const
{
    return DDSSplitted::DDSDesc();
}
