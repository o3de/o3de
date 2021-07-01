/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_UTIL_IMAGEPAINTER_H
#define CRYINCLUDE_EDITOR_UTIL_IMAGEPAINTER_H
#pragma once

#include "Util/Image.h"

struct LayerWeight;

// Brush structure used for painting.
struct SANDBOX_API SEditorPaintBrush
{
    // constructor
    SEditorPaintBrush(class CHeightmap& rHeightmap, class CLayer& rLayer,
                const bool bMaskByLayerSettings, const uint32 dwLayerIdMask, const bool bFlood);

    CHeightmap&            m_rHeightmap;        // for mask support
    unsigned char           color;                  // Painting color
    float                           fRadius;                // outer radius (0..1 for the whole terrain size)
    float                           hardness;               // 0-1 hardness of brush
    bool                            bBlended;               // true=shades of the value are stores, false=the value is either stored or not
    bool                            m_bFlood;                   // true=fills square area without attenuation, false=fills circle area with attenuation
    uint32                      m_dwLayerIdMask;// reference Value for the mask, 0xffffffff if not used
    CLayer&                    m_rLayer;                // layer we paint with
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    ColorF                      m_cFilterColor; // (1,1,1) if not used, multiplied with brightness
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

    // Arguments:
    //   fX - 0..1 in the whole terrain
    //   fY - 0..1 in the whole terrain
    // Return:
    //   0=paint there 0% .. 1=paint there 100%
    float GetMask(const float fX, const float fY) const;

protected: // --------------------------------------------------------------------------

    float                           m_fMinSlope;                // in m per m
    float                           m_fMaxSlope;                // in m per me
    float                           m_fMinAltitude;         // in m
    float                           m_fMaxAltitude;         // in m
};

// Contains image painting functions.
class CImagePainter
{
public:

    // Paint spot on image at position px,py with specified paint brush parameters (to a layer)
    // Arguments:
    //   fpx - 0..1 in the whole terrain (used for the mask)
    //   fpy - 0..1 in the whole terrain (used for the mask)
    SANDBOX_API void PaintBrush(const float fpx, const float fpy, TImage<LayerWeight>& image, const SEditorPaintBrush& brush);

    // Paint spot with pattern (to an RGB image)
    // real spot is drawn to (fpx-dwOffsetX,fpy-dwOffsetY) - to get the pattern working we need this info split up like this
    // Arguments:
    //   fpx - 0..1 in the whole terrain (used for the mask)
    //   fpy - 0..1 in the whole terrain (used for the mask)
    void PaintBrushWithPattern(const float fpx, const float fpy, CImageEx& outImage, const uint32 dwOffsetX, const uint32 dwOffsetY,
        const float fScaleX, const float fScaleY, const SEditorPaintBrush& brush, const CImageEx& imgPattern);

    //
    void FillWithPattern(CImageEx& outImage, const uint32 dwOffsetX, const uint32 dwOffsetY, const CImageEx& imgPattern);
};


#endif // CRYINCLUDE_EDITOR_UTIL_IMAGEPAINTER_H
