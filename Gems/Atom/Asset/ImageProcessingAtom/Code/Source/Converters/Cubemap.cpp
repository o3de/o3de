/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Processing/ImageObjectImpl.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageConvert.h>
#include <Processing/ImageFlags.h>

#include <Compressors/Compressor.h>
#include <Converters/PixelOperation.h>

#include <Converters/Cubemap.h>
#include <CCubeMapProcessor.h>

namespace ImageProcessingAtom
{
    CubemapLayoutInfo CubemapLayout::s_layoutList[CubemapLayoutTypeCount];

    template <class TInteger>
    inline bool IsPowerOfTwo(TInteger x)
    {
        return (x & (x - 1)) == 0;
    }

    CubemapLayoutInfo::CubemapLayoutInfo()
        : m_type(CubemapLayoutNone)
        , m_rows(0)
        , m_columns(0)
    {
    }

    void CubemapLayoutInfo::SetFaceInfo(CubemapFace face, AZ::u8 row, AZ::u8 col, CubemapFaceDirection dir)
    {
        m_faceInfos[face].row = row;
        m_faceInfos[face].column = col;
        m_faceInfos[face].direction = dir;
    }

    void CubemapLayout::InitCubemapLayoutInfos()
    {
        //CubemapLayoutHorizontal
        //left , right, front, back, top, bottom;
        //NOTE: this layout is widely used in game projects by Jan 2018 since other layouts weren't supported correctly
        //but the faces in one has unusual directions compare to other format.
        //The direction matters when using it as input for Cubemap generation filter.
        //Left: rotated left 90 degree. Right: rotated right 90 degree
        //Front:  rotated 180 degree. Back: no rotation
        //Top: rotate 180 degree. Bottom: no rotation
        CubemapLayoutInfo* info = &s_layoutList[CubemapLayoutHorizontal];
        info->m_rows = 1;
        info->m_columns = 6;
        info->m_type = CubemapLayoutHorizontal;
        info->SetFaceInfo(FaceLeft, 0, 0, CubemapFaceDirection::DirRotateLeft90);
        info->SetFaceInfo(FaceRight, 0, 1, CubemapFaceDirection::DirRotateRight90);
        info->SetFaceInfo(FaceFront, 0, 2, CubemapFaceDirection::DirRotate180);
        info->SetFaceInfo(FaceBack, 0, 3, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceTop, 0, 4, CubemapFaceDirection::DirRotate180);
        info->SetFaceInfo(FaceBottom, 0, 5, CubemapFaceDirection::DirNoRotation);

        //CubemapLayoutHorizontalCross
        //       top
        //  left front  right back
        //       bottom
        info = &s_layoutList[CubemapLayoutHorizontalCross];
        info->m_rows = 3;
        info->m_columns = 4;
        info->m_type = CubemapLayoutHorizontalCross;
        info->SetFaceInfo(FaceLeft, 1, 0, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceRight, 1, 2, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceFront, 1, 1, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceBack, 1, 3, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceTop, 0, 1, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceBottom, 2, 1, CubemapFaceDirection::DirNoRotation);

        //CubemapLayoutVerticalCross
        //       top
        //  left front  right
        //       bottom
        //       back
        info = &s_layoutList[CubemapLayoutVerticalCross];
        info->m_rows = 4;
        info->m_columns = 3;
        info->m_type = CubemapLayoutVerticalCross;
        info->SetFaceInfo(FaceLeft, 1, 0, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceRight, 1, 2, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceFront, 1, 1, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceBack, 3, 1, CubemapFaceDirection::DirRotate180);
        info->SetFaceInfo(FaceTop, 0, 1, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceBottom, 2, 1, CubemapFaceDirection::DirNoRotation);

        //CubemapLayoutVertical
        //       left
        //       right
        //       front
        //       back
        //       top
        //       bottom
        info = &s_layoutList[CubemapLayoutVertical];
        info->m_rows = 6;
        info->m_columns = 1;
        info->m_type = CubemapLayoutVertical;
        info->SetFaceInfo(FaceLeft, 0, 0, CubemapFaceDirection::DirRotateLeft90);
        info->SetFaceInfo(FaceRight, 1, 0, CubemapFaceDirection::DirRotateRight90);
        info->SetFaceInfo(FaceFront, 2, 0, CubemapFaceDirection::DirRotate180);
        info->SetFaceInfo(FaceBack, 3, 0, CubemapFaceDirection::DirNoRotation);
        info->SetFaceInfo(FaceTop, 4, 0, CubemapFaceDirection::DirRotate180);
        info->SetFaceInfo(FaceBottom, 5, 0, CubemapFaceDirection::DirNoRotation);

        //make sure all types were initialized
        for (int i = 0; i < CubemapLayoutTypeCount; i++)
        {
            AZ_Assert(s_layoutList[i].m_type == i, "layout %d is not initialized", i);
        }
    }

    const float* GetTransformMatrix(CubemapFaceDirection dir, bool isInvert)
    {
        switch (dir)
        {
        case CubemapFaceDirection::DirNoRotation:
        {
            static const float mat[] = { 1, 0, 0, 1 };
            return mat;
        }
        case CubemapFaceDirection::DirRotateLeft90:
        {
            //thelta = 90 degree
            //{cos, -sin, sin, cos}
            if (isInvert)
            {
                return GetTransformMatrix(CubemapFaceDirection::DirRotateRight90, false);
            }
            static const float mat[] = { 0, -1, 1, 0 };
            return mat;
        }
        case CubemapFaceDirection::DirRotateRight90:
        {
            //thelta = -90 degree
            if (isInvert)
            {
                return GetTransformMatrix(CubemapFaceDirection::DirRotateLeft90, false);
            }
            static const float mat[] = { 0, 1, -1, 0 };
            return mat;
        }
        case CubemapFaceDirection::DirRotate180:
        {
            //thelta = 180 degree
            static const float mat[] = { -1, 0, 0, -1 };
            return mat;
        }
        case CubemapFaceDirection::DirMirrorHorizontal:
        {
            static const float mat[] = { 1, 0, 0, -1 };
            return mat;
        }
        default:
        {
            AZ_Assert(false, "unimplemented direction matrix");
            static const float mat[] = { 1, 0, 0, 1 };
            return mat;
        }
        }
    }

    void TransformImage(CubemapFaceDirection srcDir, CubemapFaceDirection dstDir, const AZ::u8* srcImageBuf,
        AZ::u8* dstImageBuf, AZ::u8 bytePerPixel, AZ::u32 rectSize)
    {
        //get final matrix to transform dst back to src
        const float* m1 = GetTransformMatrix(dstDir, true);
        const float* m2 = GetTransformMatrix(srcDir, false);
        float mtx[4];
        mtx[0] = m1[0] * m2[0] + m1[1] * m2[2];
        mtx[1] = m1[0] * m2[1] + m1[1] * m2[3];
        mtx[2] = m1[2] * m2[0] + m1[3] * m2[2];
        mtx[3] = m1[2] * m2[1] + m1[3] * m2[3];

        const float* noRotate = GetTransformMatrix(CubemapFaceDirection::DirNoRotation, false);

        if (memcmp(noRotate, mtx, 4 * sizeof(float)) == 0)
        {
            memcpy(dstImageBuf, srcImageBuf, rectSize * rectSize * bytePerPixel);
            return;
        }

        //for each pixel in dst image, find it's location in src and copy the data from there
        float halfSize = static_cast<float>(rectSize / 2);
        for (AZ::u32 row = 0; row < rectSize; row++)
        {
            for (AZ::u32 col = 0; col < rectSize; col++)
            {
                //coordinate in image center as origin and right as positive X, up as positive Y
                float dstX = col + 0.5f - halfSize;
                float dstY = halfSize - row - 0.5f;
                float srcX = dstX * mtx[0] + dstY * mtx[1];
                float srcY = dstX * mtx[2] + dstY * mtx[3];
                AZ::u32 srcCol = static_cast<AZ::u32>(srcX + halfSize);
                AZ::u32 srcRow = static_cast<AZ::u32>(halfSize - srcY);

                memcpy(&dstImageBuf[(row * rectSize + col) * bytePerPixel],
                    &srcImageBuf[(srcRow * rectSize + srcCol) * bytePerPixel], bytePerPixel);
            }
        }
    }

    CubemapLayout::CubemapLayout()
        : m_info(nullptr)
        , m_image(nullptr)
        , m_faceSize(256)
    {
    }

    CubemapLayout* CubemapLayout::CreateCubemapLayout(IImageObjectPtr image)
    {
        //only support uncompressed format.
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(image->GetPixelFormat()))
        {
            AZ_Assert(false, "CubemapLayout only support uncompressed image");
            return nullptr;
        }

        CubemapLayout* layout = nullptr;
        CubemapLayoutInfo* info = GetCubemapLayoutInfo(image);
        if (info)
        {
            layout = new CubemapLayout();
            layout->m_info = GetCubemapLayoutInfo(image);
            layout->m_image = image;
            layout->m_faceSize = image->GetWidth(0) / layout->m_info->m_columns;
        }
        return layout;
    }


    CubemapLayoutInfo* CubemapLayout::GetCubemapLayoutInfo(CubemapLayoutType type)
    {
        if (type == CubemapLayoutNone)
        {
            return nullptr;
        }

        //if it's never initialized
        if (s_layoutList[0].m_type == CubemapLayoutNone)
        {
            InitCubemapLayoutInfos();
        }

        return &s_layoutList[type];
    }

    CubemapLayoutInfo* CubemapLayout::GetCubemapLayoutInfo(IImageObjectPtr image)
    {
        //if it's never initialized
        if (s_layoutList[0].m_type == CubemapLayoutNone)
        {
            InitCubemapLayoutInfos();
        }

        if (image == nullptr)
        {
            return nullptr;
        }

        uint32 width, height;
        width = image->GetWidth(0);
        height = image->GetHeight(0);
        CubemapLayoutInfo* info = nullptr;

        for (int i = 0; i < CubemapLayoutTypeCount; i++)
        {
            if (width * s_layoutList[i].m_rows == height * s_layoutList[i].m_columns)
            {
                info = &s_layoutList[i];

                //we require the face size need to be power of two
                if (IsPowerOfTwo(width / info->m_columns))
                {
                    return info;
                }
                else
                {
                    return nullptr;
                }
            }
        }
        return nullptr;
    }

    //public functions to get faces information for associated image
    AZ::u32 CubemapLayout::GetFaceSize()
    {
        return m_faceSize;
    }

    CubemapLayoutInfo* CubemapLayout::GetLayoutInfo()
    {
        return m_info;
    }

    CubemapFaceDirection CubemapLayout::GetFaceDirection(CubemapFace face)
    {
        return m_info->m_faceInfos[face].direction;
    }

    void CubemapLayout::GetFaceData(CubemapFace face, void* outBuffer, AZ::u32& outSize)
    {
        //only valid for uncompressed
        AZ::u32 sizePerPixel = CPixelFormats::GetInstance().GetPixelFormatInfo(m_image->GetPixelFormat())->bitsPerBlock / 8;

        AZ::u8* imageBuf;
        AZ::u32 dwPitch;
        m_image->GetImagePointer(0, imageBuf, dwPitch);
        AZ::u8* dstBuf = (AZ::u8*)outBuffer;

        AZ::u32 startX = m_info->m_faceInfos[face].column * m_faceSize;
        AZ::u32 startY = m_info->m_faceInfos[face].row * m_faceSize;

        //face size is same as rows for uncompressed format
        for (AZ::u32 y = 0; y < m_faceSize; y++)
        {
            AZ::u32 scanlineSize = m_faceSize * sizePerPixel;
            AZ::u8* srcBuf = &imageBuf[(startY + y) * dwPitch + startX * sizePerPixel];
            memcpy(dstBuf, srcBuf, scanlineSize);
            dstBuf += scanlineSize;
        }

        outSize = m_faceSize * m_faceSize * sizePerPixel;
    }

    void CubemapLayout::SetFaceData(CubemapFace face, void* dataBuffer, [[maybe_unused]] AZ::u32 dataSize)
    {
        //only valid for uncompressed
        AZ::u32 sizePerPixel = CPixelFormats::GetInstance().GetPixelFormatInfo(m_image->GetPixelFormat())->bitsPerBlock / 8;

        AZ::u8* imageBuf;
        AZ::u32 dwPitch;
        m_image->GetImagePointer(0, imageBuf, dwPitch);
        AZ::u8* srcBuf = (AZ::u8*)dataBuffer;

        AZ::u32 startX = m_info->m_faceInfos[face].column * m_faceSize;
        AZ::u32 startY = m_info->m_faceInfos[face].row * m_faceSize;

        //face size is same as rows for uncompressed format
        for (AZ::u32 y = 0; y < m_faceSize; y++)
        {
            AZ::u32 scanlineSize = m_faceSize * sizePerPixel;
            AZ::u8* dstBuf = &imageBuf[(startY + y) * dwPitch + startX * sizePerPixel];
            memcpy(dstBuf, srcBuf, scanlineSize);
            srcBuf += scanlineSize;
        }
    }

    void* CubemapLayout::GetFaceMemBuffer(AZ::u32 mip, CubemapFace face, AZ::u32& outPitch)
    {
        if (CubemapLayoutVertical != m_info->m_type)
        {
            AZ_Assert(false, "this should only be used for CubemapLayoutVertical which has continuous memory for each face");
            return nullptr;
        }

        AZ::u32 faceSize = m_faceSize >> mip;
        AZ::u8* imageBuf;
        m_image->GetImagePointer(mip, imageBuf, outPitch);
        AZ::u32 startY = m_info->m_faceInfos[face].row * faceSize;

        //use startY is same as rows from m_image since the pixel format is uncompressed
        return &imageBuf[startY * outPitch];
    }

    void CubemapLayout::SetToFaceMemBuffer(AZ::u32 mip, CubemapFace face, void* dataBuffer)
    {
        if (CubemapLayoutVertical != m_info->m_type)
        {
            AZ_Assert(false, "this should only be used for CubemapLayoutVertical which has continuous memory for each face");
            return;
        }

        AZ::u32 faceSize = m_faceSize >> mip;
        AZ::u32 pitch;
        AZ::u8* imageBuf;
        m_image->GetImagePointer(mip, imageBuf, pitch);
        AZ::u32 startY = m_info->m_faceInfos[face].row * faceSize;

        //use startY is same as rows from m_image since the pixel format is uncompressed
        memcpy(&imageBuf[startY * pitch], dataBuffer, faceSize * pitch);
    }

    void CubemapLayout::GetRectForFace(AZ::u32 mip, CubemapFace face, QRect& outRect)
    {
        AZ::u32 faceSize = m_faceSize >> mip;
        AZ::u32 startY = m_info->m_faceInfos[face].row * faceSize;
        AZ::u32 startX = m_info->m_faceInfos[face].column * faceSize;

        outRect.setRect(startX, startY, faceSize, faceSize);
    }

    bool ImageToProcess::ConvertCubemapLayout(CubemapLayoutType dstLayoutType)
    {
        const EPixelFormat srcPixelFormat = m_img->GetPixelFormat();

        //it need to be uncompressed format
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(srcPixelFormat))
        {
            AZ_Assert(false, "Please convert the image to uncompressed pixel format before calling ConvertCubemapLayout");
            return false;
        }

        // If it's a latitude longitude map, convert it to cube map with vertical layout first.
        if (IsValidLatLongMap(m_img))
        {
            m_img = ConvertLatLongMapToCubemap(m_img);
        }

        //check if it's valid cubemap size
        CubemapLayoutInfo* layoutInfo = CubemapLayout::GetCubemapLayoutInfo(m_img);
        if (layoutInfo == nullptr)
        {
            AZ_Error("Image Processing", false, "The original image doesn't have a valid size (layout) as cubemap");
            return false;
        }

        //if the source is same as output layout, return directly
        if (layoutInfo->m_type == dstLayoutType)
        {
            return true;
        }

        CubemapLayoutInfo* dstLayoutInfo = CubemapLayout::GetCubemapLayoutInfo(dstLayoutType);

        //create cubemap layout for source image for later operation.
        CubemapLayout* srcCubemap = CubemapLayout::CreateCubemapLayout(m_img);
        AZ::u32 faceSize = srcCubemap->GetFaceSize();

        //create new image with same pixel format and copy properties from source image
        IImageObjectPtr newImage(IImageObject::CreateImage(faceSize* dstLayoutInfo->m_columns,
            faceSize* dstLayoutInfo->m_rows, 1, srcPixelFormat));
        CubemapLayout* dstCubemap = CubemapLayout::CreateCubemapLayout(newImage);
        newImage->CopyPropertiesFrom(newImage);

        //copy data from src cube to dst cube for each face
        //temp buf for copy over data
        AZ::u32 sizePerPixel = CPixelFormats::GetInstance().GetPixelFormatInfo(srcPixelFormat)->bitsPerBlock / 8; //only valid for uncompressed
        AZ::u8* buf = new AZ::u8[faceSize * faceSize * sizePerPixel];
        AZ::u8* tempBuf = new AZ::u8[faceSize * faceSize * sizePerPixel];

        for (AZ::u32 faceIdx = 0; faceIdx < FaceCount; faceIdx++)
        {
            AZ::u32 outSize = 0;
            CubemapFace face = (CubemapFace)faceIdx;
            srcCubemap->GetFaceData(face, buf, outSize);
            CubemapFaceDirection srcDir = srcCubemap->GetFaceDirection(face);
            CubemapFaceDirection dstDir = dstCubemap->GetFaceDirection(face);
            if (srcDir == dstDir)
            {
                dstCubemap->SetFaceData(face, buf, outSize);
            }
            else
            {
                //transform the image
                TransformImage(srcDir, dstDir, buf, tempBuf, static_cast<AZ::u8>(sizePerPixel), faceSize);
                dstCubemap->SetFaceData(face, tempBuf, outSize);
            }
        }

        //clean up
        delete[] buf;
        delete[] tempBuf;
        delete srcCubemap;
        delete dstCubemap;

        newImage->AddImageFlags(EIF_Cubemap);
        m_img = newImage;
        return true;
    }

    bool ImageConvertProcess::FillCubemapMipmaps()
    {
        //this function only works with pixel format rgba32f
        const EPixelFormat srcPixelFormat = m_image->Get()->GetPixelFormat();
        if (srcPixelFormat != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "%s only works with pixel format rgba32f", __FUNCTION__);
            return false;
        }

        //only if the src image has one mip
        if (m_image->Get()->GetMipCount() != 1)
        {
            AZ_Assert(false, "%s called for a mipmapped image. ", __FUNCTION__);
            return false;
        }

        const PresetSettings& preset = m_input->m_presetSetting;

        CubemapLayout* srcCubemap = CubemapLayout::CreateCubemapLayout(m_image->Get());

        uint32 outWidth;
        uint32 outHeight;
        uint32 outReduce = 0;
        AZ::u32 srcFaceSize = srcCubemap->GetFaceSize();

        //get output face size
        AZ::u32 outFaceSize, outFaceHeight;
        GetOutputExtent(srcFaceSize, srcFaceSize, outFaceSize, outFaceHeight, outReduce, &m_input->m_textureSetting, &preset);

        //get final cubemap image size
        outWidth = outFaceSize * srcCubemap->GetLayoutInfo()->m_columns;
        outHeight = outFaceSize * srcCubemap->GetLayoutInfo()->m_rows;

        //max mipmap count
        uint32 maxMipCount;
        if (preset.m_mipmapSetting == nullptr || !m_input->m_textureSetting.m_enableMipmap)
        {
            maxMipCount = 1;
        }
        else
        {
            //calculate based on face size, and use final export format which may save some low level mip calculation
            maxMipCount = CPixelFormats::GetInstance().ComputeMaxMipCount(preset.m_pixelFormat, outFaceSize, outFaceSize);

            //the FilterImage function won't do well with rect size 1. avoiding cubemap with face size 1
            if (srcFaceSize >> maxMipCount == 1 && maxMipCount > 1)
            {
                maxMipCount -= 1;
            }
        }

        if (preset.m_cubemapSetting->m_filter == CubemapFilterType::ggx)
        {
            //the PBR shader currently requires 6 mip levels (i.e., [0..5])
            //[GFX TODO][ATOM-2482] make this data driven per reflection cubemap
            static const uint32 ShaderMipCount = 6;
            if (maxMipCount < ShaderMipCount)
            {
                AZ_Assert(false, "Filter type GGX requires a texture size capable of at least %d mip levels", ShaderMipCount);
                return false;
            }

            maxMipCount = ShaderMipCount;
        }

        //generate box filtered source image mip chain
        IImageObjectPtr mippedSourceImage(IImageObject::CreateImage(outWidth, outHeight, maxMipCount, ePixelFormat_R32G32B32A32F));
        mippedSourceImage->CopyPropertiesFrom(m_image->Get());

        for (int iSide = 0; iSide < 6; ++iSide)
        {
            for (int iMip = 0; iMip < (int)maxMipCount; iMip++)
            {
                QRect srcRect;
                QRect dstRect;

                srcRect.setLeft(0);
                srcRect.setRight(srcFaceSize);
                srcRect.setTop(iSide * srcFaceSize);
                srcRect.setBottom((iSide + 1) * srcFaceSize);

                AZ::u32 mipFaceSize = outFaceSize >> iMip;

                dstRect.setLeft(0);
                dstRect.setRight(mipFaceSize);
                dstRect.setTop(iSide * mipFaceSize);
                dstRect.setBottom((iSide + 1) * mipFaceSize);

                MipGenType mipGenType = (iMip == 0 ? MipGenType::point : MipGenType::box);
                FilterImage(mipGenType, MipGenEvalType::sum, 0, 0, m_image->Get(), 0, mippedSourceImage, iMip, &srcRect, &dstRect);
            }
        }

        //replace the source cubemap with the mipped version
        delete srcCubemap;
        srcCubemap = CubemapLayout::CreateCubemapLayout(mippedSourceImage);

        //create new new output image with proper face
        IImageObjectPtr outImage(IImageObject::CreateImage(outWidth, outHeight, maxMipCount, srcPixelFormat));
        outImage->CopyPropertiesFrom(m_image->Get());
        CubemapLayout* dstCubemap = CubemapLayout::CreateCubemapLayout(outImage);
        AZ::u32 dstMipCount = outImage->GetMipCount();

        //filter mip 0 from source to destination
        for (int iSide = 0; iSide < 6; ++iSide)
        {
            QRect srcRect;
            QRect dstRect;

            srcRect.setLeft(0);
            srcRect.setRight(srcFaceSize);
            srcRect.setTop(iSide * srcFaceSize);
            srcRect.setBottom((iSide + 1) * srcFaceSize);

            dstRect.setLeft(0);
            dstRect.setRight(outFaceSize);
            dstRect.setTop(iSide * outFaceSize);
            dstRect.setBottom((iSide + 1) * outFaceSize);

            FilterImage(m_input->m_textureSetting.m_mipGenType, m_input->m_textureSetting.m_mipGenEval, 0, 0, m_image->Get(), 0,
                outImage, 0, &srcRect, &dstRect);
        }

        CCubeMapProcessor  atiCubemanGen;
        //ATI's cubemap generator to filter the image edges to avoid seam problem
        // https://gpuopen.com/archive/gamescgi/cubemapgen/

        //the thread support was done with windows thread function so it's removed for multi-dev platform support
        atiCubemanGen.m_NumFilterThreads = 0;

        //input and output cubemap set to have save dimensions
        atiCubemanGen.Init(outFaceSize, outFaceSize, dstMipCount, 4);

        //load the 6 faces of the input cubemap for each mip level into the cubemap processor
        void* pMem;
        uint32 nPitch;

        for (int iFace = 0; iFace < 6; ++iFace)
        {
            for (int iMip = 0; iMip < (int)maxMipCount; ++iMip)
            {
                pMem = srcCubemap->GetFaceMemBuffer(iMip, (CubemapFace)iFace, nPitch);

                atiCubemanGen.SetInputFaceData(
                    iFace,                      // FaceIdx,
                    iMip,                       // MipIdx
                    CP_VAL_FLOAT32,             // SrcType,
                    4,                          // SrcNumChannels,
                    nPitch,                     // SrcPitch,
                    pMem,                       // SrcDataPtr,
                    1000000.0f,                 // MaxClamp,
                    1.0f,                       // Degamma,
                    1.0f);                      // Scale
            }
        }

        //number of rays to use for the GGX importance sampling
        //note: more rays reduces artifacts but increases processing time
        //[GFX TODO][ATOM-2956] add a sample quality option to the reflection volume to control this per reflection
        static const uint32 SampleCountGGX = 256;

        //filter cubemap
        atiCubemanGen.InitiateFiltering(
            preset.m_cubemapSetting->m_angle,              //BaseFilterAngle,
            preset.m_cubemapSetting->m_mipAngle,           //InitialMipAngle,
            preset.m_cubemapSetting->m_mipSlope,           //MipAnglePerLevelScale,
            (int)preset.m_cubemapSetting->m_filter,        //FilterType, CP_FILTER_TYPE_COSINE for diffuse cube
            preset.m_cubemapSetting->m_edgeFixup > 0 ? CP_FIXUP_PULL_LINEAR : CP_FIXUP_NONE,             //FixupType, CP_FIXUP_PULL_LINEAR if FixupWidth> 0
            static_cast<int32>(preset.m_cubemapSetting->m_edgeFixup),          //FixupWidth,
            true,               //bUseSolidAngle,
            16,                 //GlossScale,
            0,                  //GlossBias
            SampleCountGGX);

        //copy the convolved cubemap data for each face and mip into the output image
        for (int iFace = 0; iFace < 6; ++iFace)
        {
            for (unsigned int dstMip = 0; dstMip < dstMipCount; ++dstMip)
            {
                pMem = dstCubemap->GetFaceMemBuffer(dstMip, (CubemapFace)iFace, nPitch);
                atiCubemanGen.GetOutputFaceData(iFace, dstMip, CP_VAL_FLOAT32, 4, nPitch, pMem, 1.0f, 1.0f);
            }
        }

        delete srcCubemap;
        delete dstCubemap;

        //set back to image
        m_image->Set(outImage);
        return true;
    }

    // Convert from direction into (u,v) coordinates latitude-longitude map
    void NormalToLatLongUV(const AZ::Vector3& dir, float& outU, float& outV)
    {
        // The normal we calculate from cubemap is Y up. +z is forward
        float r = sqrt(dir.GetX() * dir.GetX() + dir.GetY() * dir.GetY());
        float latitude = (r < abs(dir.GetZ())) ? acos(r) * (dir.GetZ() >= 0 ? 1 : -1) : asin(dir.GetZ());
        float longitude = (dir.GetY() == 0.0f && dir.GetX() == 0.0f) ? 0.0f : atan2(dir.GetX(), dir.GetY());

        outU = 1.0f - (longitude * 0.159154943f + 0.5f); // Longitude [-Pi, Pi] -> [0, 1]
        outV = 0.5f - latitude * 0.318309886f; // Latitude [Pi/2, -Pi/2] -> [0, 1]

        AZ_Assert(outU <= 1 && outU >= 0, "wrong pixel position");
        AZ_Assert(outV <= 1 && outV >= 0, "wrong pixel position");
    }

    // Get normal from a 2d vector and cubemap face index
    AZ::Vector3 GetNormalForVerticalLayout(CubemapFace faceIdx, float x, float y)
    {
        AZ::Vector3 normal;
        switch (faceIdx)
        {
        case FaceLeft:
            normal = AZ::Vector3(-1, -x, y);
            break;
        case FaceRight:
            normal = AZ::Vector3(1, x, y);
            break;
        case FaceFront:
            normal = AZ::Vector3(-x, -y, 1);
            break;
        case FaceBack:
            normal = AZ::Vector3(-x, y, -1);
            break;
        case FaceTop:
            normal = AZ::Vector3(-x, 1, y);
            break;
        case FaceBottom:
            normal = AZ::Vector3(x, -1, y);
            break;
        }
        normal.Normalize();
        return normal;
    }

    bool IsValidLatLongMap(IImageObjectPtr latitudeMap)
    {
        AZ::u32 srcWidth = latitudeMap->GetWidth(0);
        AZ::u32 srcHeight = latitudeMap->GetHeight(0);
        return (srcWidth == srcHeight * 2 && srcWidth % 4 == 0);
    }

    IImageObjectPtr ConvertLatLongMapToCubemap(IImageObjectPtr latitudeMap)
    {
        const EPixelFormat srcPixelFormat = latitudeMap->GetPixelFormat();

        // The map need to be uncompressed format
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(srcPixelFormat))
        {
            AZ_Assert(false, "The input image should have uncompressed pixel format.");
            return nullptr;
        }

        AZ_Assert(latitudeMap->GetMipCount() == 1, "The mipmap won't be converted");


        AZ::u32 srcWidth = latitudeMap->GetWidth(0);
        AZ::u32 srcHeight = latitudeMap->GetHeight(0);
        AZ::u8* srcBuf;
        AZ::u32 outPitch;
        latitudeMap->GetImagePointer(0, srcBuf, outPitch);

        if (!IsValidLatLongMap(latitudeMap))
        {
            AZ_Error("Image Processing", false, "Invalid latitude-longitude map resolution [%dx%d]."
                "The aspect ratio should be 2:1 and the width should be dividable by 4 ", srcWidth, srcHeight);
            return nullptr;
        }

        AZ::u32 srcfaceSize = srcWidth / 4;

        // Convert face size to power of 2 since CreateCubemapLayout doesn't support non-power of 2 face size.
        // Find the highest power of 2 less than or equal to source face size
        srcfaceSize >>= 1;
        AZ::u32 faceSize = 1;
        while (srcfaceSize > 0)
        {
            faceSize <<= 1;
            srcfaceSize >>= 1;
        }
        AZ_Assert(faceSize <= srcWidth / 4, "wrong conversion");

        // Create output image
        IImageObjectPtr outImage(IImageObject::CreateImage(faceSize, faceSize * FaceCount, 1, srcPixelFormat));
        outImage->CopyPropertiesFrom(latitudeMap);
        outImage->AddImageFlags(EIF_Cubemap);
        CubemapLayout* dstCubemap = CubemapLayout::CreateCubemapLayout(outImage);

        IPixelOperationPtr pixelOp = CreatePixelOperation(srcPixelFormat);

        AZ::u32 sizePerPixel = CPixelFormats::GetInstance().GetPixelFormatInfo(srcPixelFormat)->bitsPerBlock / 8; //only valid for uncompressed
        AZ::u32 facePixelCount = faceSize * faceSize;
        float radius = faceSize / 2.0f;
        for (AZ::u32 faceIdx = 0; faceIdx < FaceCount; faceIdx++)
        {
            CubemapFace face = (CubemapFace)faceIdx;
            AZ::u8* buf = (AZ::u8*)dstCubemap->GetFaceMemBuffer(0, face, outPitch);

            // Get value from the original map and assign it to each pixel on this face
            for (AZ::u32 pixelIdx = 0; pixelIdx < facePixelCount; pixelIdx++)
            {
                float x =  ((pixelIdx % faceSize) - radius) / radius;
                float y = -((pixelIdx / faceSize) - radius) / radius;
                AZ::Vector3 normal = GetNormalForVerticalLayout(face, x, y);
                float srcU, srcV;
                NormalToLatLongUV(normal, srcU, srcV);

                // Get 4 corner pixels' color and use them to interpolate the final color of destination pixel
                float px = srcU * (float)(srcWidth - 1);
                float py = srcV * (float)(srcHeight - 1);
                AZ::u32 px1 = (AZ::u32) px;
                AZ::u32 px2 = ((AZ::u32) px + 1) % srcWidth;
                AZ::u32 py1 = (AZ::u32) py;
                AZ::u32 py2 = ((AZ::u32) py + 1) % srcHeight;
                float t1 = px - px1;
                float t2 = py - py1;

                float p1[4], p2[4], p3[4], p4[4];
                pixelOp->GetRGBA(&srcBuf[(px1 + py1 * srcWidth) * sizePerPixel], p1[0], p1[1], p1[2], p1[3]);
                pixelOp->GetRGBA(&srcBuf[(px1 + py2 * srcWidth) * sizePerPixel], p2[0], p2[1], p2[2], p2[3]);
                pixelOp->GetRGBA(&srcBuf[(px2 + py1 * srcWidth) * sizePerPixel], p3[0], p3[1], p3[2], p3[3]);
                pixelOp->GetRGBA(&srcBuf[(px2 + py2 * srcWidth) * sizePerPixel], p4[0], p4[1], p4[2], p4[3]);

                float dstP[4];
                dstP[0] = (1 - t2) * ((1 - t1) * p1[0] + t1 * p3[0]) + t2 * ((1 - t1) * p2[0] + t1 * p4[0]);
                dstP[1] = (1 - t2) * ((1 - t1) * p1[1] + t1 * p3[1]) + t2 * ((1 - t1) * p2[1] + t1 * p4[1]);
                dstP[2] = (1 - t2) * ((1 - t1) * p1[2] + t1 * p3[2]) + t2 * ((1 - t1) * p2[2] + t1 * p4[2]);
                dstP[3] = (1 - t2) * ((1 - t1) * p1[3] + t1 * p3[3]) + t2 * ((1 - t1) * p2[3] + t1 * p4[3]);

                pixelOp->SetRGBA(&buf[pixelIdx * sizePerPixel], dstP[0], dstP[1], dstP[2], dstP[3]);
            }
        }

        delete dstCubemap;

        return outImage;
    }
} // namespace ImageProcessingAtom
