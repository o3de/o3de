/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageProcessing_precompiled.h>

#include <ImageLoader/ImageLoaders.h>
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
        QString ext = fileInfo.suffix();

        if (!fileInfo.exists())
        {
            return nullptr;
        }

        if (TIFFLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return TIFFLoader::LoadImageFromTIFF(filename);
        }
        else if (DdsLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return DdsLoader::LoadImageFromFile(filename);
        }
        else if (QtImageLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return QtImageLoader::LoadImageFromFile(filename);
        }
        else if (ExrLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return ExrLoader::LoadImageFromFile(filename);
        }

        AZ_Warning("ImageProcessing", false, "No proper image loader to load file: %s", filename.c_str());
        return nullptr;
    }

    bool IsExtensionSupported(const char* extension)
    {
        if (TIFFLoader::IsExtensionSupported(extension))
        {
            return true;
        }
        else if (DdsLoader::IsExtensionSupported(extension))
        {
            return true;
        }
        else if (QtImageLoader::IsExtensionSupported(extension))
        {
            return true;
        }
        else if (ExrLoader::IsExtensionSupported(extension))
        {
            return true;
        }

        return false;
    }

    const AZStd::string LoadEmbeddedSettingFromFile(const AZStd::string& filename)
    {
        QFileInfo fileInfo(filename.c_str());
        QString ext = fileInfo.suffix();

        if (TIFFLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return TIFFLoader::LoadSettingFromTIFF(filename);
        }
        return "";
    }
}// namespace ImageProcessingAtom
