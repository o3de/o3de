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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Utilitarian classes for double buffer GDI rendering and 32bit bitmaps


#ifndef CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
#define CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
#pragma once

//! function used to compute thumbs per row and spacing, used in asset browser and other tools where thumb layout is needed and maybe GDI canvas used
//! \param aContainerWidth the thumbs' container width
//! \param aThumbWidth the thumb image width
//! \param aMargin the thumb default minimum horizontal margin
//! \param aThumbCount the thumb count
//! \param rThumbsPerRow returned thumb count per single row
//! \param rNewMargin returned new computed margin between thumbs
//! \note The margin between thumbs will grow/shrink dynamically to keep up with the thumb count per row
bool ComputeThumbsLayoutInfo(float aContainerWidth, float aThumbWidth, float aMargin, UINT aThumbCount, UINT& rThumbsPerRow, float& rNewMargin);

QColor ScaleColor(const QColor& coor, float aScale);

//! This class loads alpha-channel bitmaps and holds a DC for use with AlphaBlend function
class CRYEDIT_API CAlphaBitmap
{
public:

    CAlphaBitmap();
    ~CAlphaBitmap();

    //! creates the bitmap from raw 32bpp data
    //! \param pData the 32bpp raw image data, RGBA, can be NULL and it would create just an empty bitmap
    //! \param aWidth the bitmap width
    //! \param aHeight the bitmap height
    bool            Create(void* pData, UINT aWidth, UINT aHeight, bool bVerticalFlip = false, bool bPremultiplyAlpha = false);
    //! \return the actual bitmap
    QImage&    GetBitmap();
    //! free the bitmap and DC
    void            Free();
    //! \return bitmap width
    UINT            GetWidth();
    //! \return bitmap height
    UINT            GetHeight();

protected:

    QImage m_bmp;
    UINT        m_width, m_height;
};

//! Fill a rectangle with a checkerboard pattern.
//! \param pGraphics The Graphics object used for drawing
//! \param rRect The rectangle to be filled
//! \param checkDiameter the diameter of the check squares
//! \param aColor1 the color that starts in the top left corner check square
//! \param aColor2 the second color used for check squares
void CheckerboardFillRect(QPainter* pGraphics, const QRect& rRect, int checkDiameter, const QColor& aColor1, const QColor& aColor2);
#endif // CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
