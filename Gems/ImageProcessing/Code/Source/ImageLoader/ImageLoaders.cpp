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

#include <ImageProcessing_precompiled.h>

#include <ImageLoader/ImageLoaders.h>
#include <ImageProcessing/ImageObject.h>
#include <QFileInfo>

namespace ImageProcessing
{
    IImageObject* LoadImageFromFile(const AZStd::string& filename)
    {
        QFileInfo fileInfo(filename.c_str());
        QString ext = fileInfo.suffix();

        if (TIFFLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return TIFFLoader::LoadImageFromTIFF(filename);
        }
        else if (BTLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return BTLoader::LoadImageFromBT(filename);
        }
        else if (QtImageLoader::IsExtensionSupported(ext.toUtf8()))
        {
            return QtImageLoader::LoadImageFromFile(filename);
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
        else if (BTLoader::IsExtensionSupported(extension))
        {
            return true;
        }
        else if (QtImageLoader::IsExtensionSupported(extension))
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

}// namespace ImageProcessing
