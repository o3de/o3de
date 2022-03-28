/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>
#include <Atom/ImageProcessing/ImageObject.h>


namespace ImageProcessingAtom
{
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
    };// namespace TIFFLoader

    // Image loader through Qt's QImage with image formats supported native and through plugins
    namespace QtImageLoader
    {
        bool IsExtensionSupported(const char* extension);
        // Load image file which supported by QtImage to an image object
        IImageObject* LoadImageFromFile(const AZStd::string& filename);
    };// namespace QtImageLoader

    // Load dds files to ImageObject. The QtImageLoader can load dds file but it only can load dds with non-compressed formats.
    namespace DdsLoader
    {
        bool IsExtensionSupported(const char* extension);
        IImageObject* LoadImageFromFile(const AZStd::string& filename);
    };// namespace DdsLoader

    // Load .exr files to an image object
    namespace ExrLoader
    {
        bool IsExtensionSupported(const char* extension);
        IImageObject* LoadImageFromFile(const AZStd::string& filename);
    };// namespace ExrLoader
}// namespace ImageProcessingAtom
