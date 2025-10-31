/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ImageLoader/ImageLoaders.h>
#include <Processing/ImageFlags.h>
#include <Atom/ImageProcessing/ImageObject.h>
//  warning C4251: class QT_Type needs to have dll-interface to be used by clients of class 'QT_Type'
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <QFileInfo>
AZ_POP_DISABLE_WARNING

namespace ImageProcessingAtom
{
    IImageObject* LoadImageFromFile(const AZStd::string& filename)
    {
        QFileInfo fileInfo(filename.c_str());
        auto suffix = fileInfo.suffix().toUtf8(); // need to cache the utf8 object so its data can be referenced below
        const char* ext = suffix.data();

        if (!fileInfo.exists())
        {
            return nullptr;
        }

        IImageObject* loadedImage = nullptr;

        if (TIFFLoader::IsExtensionSupported(ext))
        {
            loadedImage = TIFFLoader::LoadImageFromTIFF(filename);
        }
        else if (DdsLoader::IsExtensionSupported(ext))
        {
            loadedImage = DdsLoader::LoadImageFromFile(filename);
        }
        else if (TgaLoader::IsExtensionSupported(ext))
        {
            loadedImage = TgaLoader::LoadImageFromFile(filename);
        }
        else if (QtImageLoader::IsExtensionSupported(ext))
        {
            loadedImage = QtImageLoader::LoadImageFromFile(filename);
        }
        else if (ExrLoader::IsExtensionSupported(ext))
        {
            loadedImage = ExrLoader::LoadImageFromFile(filename);
        }
        else
        {
            AZ_Warning("ImageProcessing", false, "No proper image loader to load file: %s", filename.c_str());
        }

        if (loadedImage)
        {
            if (IsHDRFormat(loadedImage->GetPixelFormat()))
            {
                loadedImage->AddImageFlags(EIF_HDR);
            }
            return loadedImage;
        }
        else
        {
            AZ_Warning("ImageProcessing", false, "Failed to load image file: %s", filename.c_str());
            return nullptr;
        }
    }

    bool IsExtensionSupported(const char* extension)
    {
        return TIFFLoader::IsExtensionSupported(extension)
            || DdsLoader::IsExtensionSupported(extension)
            || TgaLoader::IsExtensionSupported(extension)
            || QtImageLoader::IsExtensionSupported(extension)
            || ExrLoader::IsExtensionSupported(extension);
    }

}// namespace ImageProcessingAtom
