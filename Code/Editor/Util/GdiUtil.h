/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Utilitarian classes for double buffer GDI rendering and 32bit bitmaps


#ifndef CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
#define CRYINCLUDE_EDITOR_UTIL_GDIUTIL_H
#pragma once

QColor ScaleColor(const QColor& coor, float aScale);

//! This class loads alpha-channel bitmaps and holds a DC for use with AlphaBlend function
class CRYEDIT_API CAlphaBitmap
{
public:

    CAlphaBitmap();
    ~CAlphaBitmap();

    //! creates the bitmap from raw 32bpp data
    //! \param pData the 32bpp raw image data, RGBA, can be nullptr and it would create just an empty bitmap
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
