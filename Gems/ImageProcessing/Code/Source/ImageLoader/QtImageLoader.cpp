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
#include "ImageProcessing_precompiled.h"

#include <ImageLoader/ImageLoaders.h>
#include <ImageProcessing/ImageObject.h>

#include <QImage>
#include <QImageReader>

///////////////////////////////////////////////////////////////////////////////////

namespace ImageProcessing
{
    namespace QtImageLoader
    {
        IImageObject* LoadImageFromFile(const AZStd::string& filename)
        {
            //try to open the image
            QImage qimage(filename.c_str());
            if (qimage.isNull())
            {
                return NULL;
            }

            //convert to format which compatiable our pixel format           
            if (qimage.format() != QImage::Format_RGBA8888)
            {
                qimage = qimage.convertToFormat(QImage::Format_RGBA8888);
            }
            
            //create a new image object
            IImageObject *pImage = IImageObject::CreateImage(qimage.width(), qimage.height(), 1, 
                ePixelFormat_R8G8B8A8);

            //get a pointer to the image objects pixel data
            uint8* pDst;
            uint32 dwPitch;
            pImage->GetImagePointer(0, pDst, dwPitch);

            //copy the qImage into the image object
            for (uint32 dwY = 0; dwY < (uint32)qimage.height(); ++dwY)
            {
                uint8* dstLine = &pDst[dwPitch * dwY];
                uchar* srcLine = qimage.scanLine(dwY);
                memcpy(dstLine, srcLine, dwPitch);
            }            
            return pImage;
        }

        bool IsExtensionSupported(const char* extension)
        {
            QList<QByteArray> imgFormats = QImageReader::supportedImageFormats();

            for (int i = 0; i < imgFormats.size(); ++i)
            {
                if (QString::fromUtf8(imgFormats[i]).toLower() == QString(extension).toLower())
                {
                    return true;
                }
            }

            return false;
        }
    }//namespace QtImageLoader
} //namespace ImageProcessing

