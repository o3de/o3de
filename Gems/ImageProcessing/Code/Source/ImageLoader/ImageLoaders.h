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

#pragma once

#include <AzCore/std/string/string.h>

namespace ImageProcessing
{
    class IImageObject;

    IImageObject* LoadImageFromFile(const AZStd::string& filename);
    bool IsExtensionSupported(const char* extension);
    const AZStd::string LoadEmbeddedSettingFromFile(const AZStd::string& filename);

    // Tiff loader. The loader support uncompressed tiff with with 1~4 channels and 8bit and 16bit uint or 16bits and 32bits float per channel
    // QImage also support tiff (tiff plugin), but it only supports 8bits uint
    namespace TIFFLoader
    {
        bool IsExtensionSupported(const char* extension);
        // Load a tiff file to an image object. 
        IImageObject* LoadImageFromTIFF(const AZStd::string& filename);
        // Load embedded .exportsettings string from tiff which was exported by deprecated feature of CryTif plugin.
        const AZStd::string LoadSettingFromTIFF(const AZStd::string& filename);
    };// namespace ImageTIFF

    namespace BTLoader
    {
        bool IsExtensionSupported(const char* extension);
        // Load a BT file to an image object.
        IImageObject* LoadImageFromBT(const AZStd::string& fileName);
    }// namespace BTLoader

    // Image loader through Qt's QImage with image formats supported native and through plugins
    namespace QtImageLoader
    {
        bool IsExtensionSupported(const char* extension);
        // Load image file which supported by QtImage to an image object
        IImageObject* LoadImageFromFile(const AZStd::string& filename);
    };// namespace QtImageLoader

}// namespace ImageProcessing
