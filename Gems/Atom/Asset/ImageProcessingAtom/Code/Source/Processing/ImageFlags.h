/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace ImageProcessingAtom
{
    // flags to propagate from the RC to the engine through GetImageFlags()
    // 32bit bitmask, numbers should not change as engine relies on them
    const static AZ::u32 EIF_Cubemap = 0x1;
    const static AZ::u32 EIF_Volumetexture = 0x2;
    const static AZ::u32 EIF_Decal = 0x4;  // this is usually set through the preset
    const static AZ::u32 EIF_Greyscale = 0x8;  // hint for the engine (e.g. greyscale light beams can be applied to shadow mask), can be for DXT1 because compression artfacts don't count as color
    const static AZ::u32 EIF_SupressEngineReduce = 0x10;  // info for the engine: don't reduce texture resolution on this texture
    const static AZ::u32 EIF_UNUSED_BIT = 0x40;  // Free to use
    const static AZ::u32 EIF_AttachedAlpha = 0x400;  // info for the engine: it's a texture with attached alpha channel
    const static AZ::u32 EIF_SRGBRead = 0x800;  // info for the engine: if gamma corrected rendering is on, this texture requires SRGBRead (it's not stored in linear)
    const static AZ::u32 EIF_DontResize = 0x8000; // info for the engine: for dds textures that shouldn't be resized
    const static AZ::u32 EIF_RenormalizedTexture = 0x10000;  // info for the engine: for dds textures that have renormalized color range
    const static AZ::u32 EIF_CafeNative = 0x20000;  // info for the engine: native Cafe texture format
    const static AZ::u32 EIF_RestrictedPlatformONative = 0x40000;  // native tiled texture for restrict platform O
    const static AZ::u32 EIF_Tiled = 0x80000;  // info for the engine: texture has been tiled for the platform
    const static AZ::u32 EIF_RestrictedPlatformDNative = 0x100000;  // native tiled texture for restrict platform D
    const static AZ::u32 EIF_Splitted = 0x200000;  // info for the engine: this texture is splitted
    const static AZ::u32 EIF_Colormodel = 0x7000000;  // info for the engine: bitmask: colormodel used in the texture
    const static AZ::u32 EIF_Colormodel_RGB = 0x0000000;  // info for the engine: colormodel is RGB (default)
    const static AZ::u32 EIF_Colormodel_CIE = 0x1000000;  // info for the engine: colormodel is CIE (used for terrain)
    const static AZ::u32 EIF_Colormodel_YCC = 0x2000000;  // info for the engine: colormodel is Y'CbCr (used for reflectance)
    const static AZ::u32 EIF_Colormodel_YFF = 0x3000000;  // info for the engine: colormodel is Y'FbFr (used for reflectance)
    const static AZ::u32 EIF_Colormodel_IRB = 0x4000000;  // info for the engine: colormodel is IRB (used for reflectance)
}
