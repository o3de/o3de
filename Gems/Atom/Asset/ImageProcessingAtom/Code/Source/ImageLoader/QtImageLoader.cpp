/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/ImageProcessing/ImageObject.h>

#include <ImageLoader/ImageLoaders.h>
#include <ImageBuilderBaseType.h>

//  warning C4251: class QT_Type needs to have dll-interface to be used by clients of class 'QT_Type'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QImage>
#include <QImageReader>
AZ_POP_DISABLE_WARNING

///////////////////////////////////////////////////////////////////////////////////

namespace ImageProcessingAtom
{
    namespace QtImageLoader
    {
        IImageObject* LoadImageFromFile(const AZStd::string& filename)
        {
            //try to open the image
            QImage qimage(filename.c_str());
            if (qimage.isNull())
            {
                AZ_Warning("ImageProcessing", false, "Failed to load [%s] via QImage", filename.c_str());
                return nullptr;
            }

            //convert to format which compatiable our pixel format
            if (qimage.format() != QImage::Format_RGBA8888)
            {
                qimage = qimage.convertToFormat(QImage::Format_RGBA8888);
            }

            //create a new image object
            IImageObject* pImage = IImageObject::CreateImage(qimage.width(), qimage.height(), 1,
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
                if (imgFormats[i].toLower().toStdString() == QString(extension).toLower().toStdString())
                {
                    return true;
                }
            }
            return false;
        }
    }//namespace QtImageLoader
} //namespace ImageProcessingAtom
