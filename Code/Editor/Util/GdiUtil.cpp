/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "GdiUtil.h"

// Qt
#include <QPainter>
#include <QMessageBox>

QColor ScaleColor(const QColor& c, float aScale)
{
    QColor aColor = c;
    if (!aColor.isValid())
    {
        // help out scaling, by starting at very low black
        aColor = QColor(1, 1, 1);
    }

    const float r = static_cast<float>(aColor.red()) * aScale;
    const float g = static_cast<float>(aColor.green()) * aScale;
    const float b = static_cast<float>(aColor.blue()) * aScale;

    return QColor(AZStd::clamp(static_cast<int>(r), 0, 255), AZStd::clamp(static_cast<int>(g), 0, 255), AZStd::clamp(static_cast<int>(b), 0, 255));
}

CAlphaBitmap::CAlphaBitmap()
{
    m_width = m_height = 0;
}

CAlphaBitmap::~CAlphaBitmap()
{
    Free();
}

bool CAlphaBitmap::Create(void* pData, UINT aWidth, UINT aHeight, bool bVerticalFlip, bool bPremultiplyAlpha)
{
    if (!aWidth || !aHeight)
    {
        return false;
    }

    m_bmp = QImage(aWidth, aHeight, QImage::Format_RGBA8888);
    if (m_bmp.isNull())
    {
        return false;
    }

    std::vector<UINT> vBuffer;

    if (pData)
    {
        // copy over the raw 32bpp data
        bVerticalFlip = !bVerticalFlip; // in Qt, the flip is not required. Still, keep the API behaving the same
        if (bVerticalFlip)
        {
            UINT nBufLen = aWidth * aHeight;
            vBuffer.resize(nBufLen);

            if (IsBadReadPtr(pData, nBufLen * 4))
            {
                //TODO: remove after testing alot the browser, it doesnt happen anymore
                QMessageBox::critical(QApplication::activeWindow(), QString(), QObject::tr("Bad image data ptr!"));
                Free();
                return false;
            }

            assert(!vBuffer.empty());

            if (vBuffer.empty())
            {
                Free();
                return false;
            }

            UINT scanlineSize = aWidth * 4;

            for (UINT i = 0, iCount = aHeight; i < iCount; ++i)
            {
                // top scanline position
                UINT* pTopScanPos = (UINT*)&vBuffer[0] + i * aWidth;
                // bottom scanline position
                UINT* pBottomScanPos = (UINT*)pData + (aHeight - i - 1) * aWidth;

                // save a scanline from top
                memcpy(pTopScanPos, pBottomScanPos, scanlineSize);
            }

            pData = &vBuffer[0];
        }

        // premultiply alpha, AlphaBlend GDI expects it
        if (bPremultiplyAlpha)
        {
            for (UINT y = 0; y < aHeight; ++y)
            {
                BYTE* pPixel = (BYTE*) pData + aWidth * 4 * y;

                for (UINT x = 0; x < aWidth; ++x)
                {
                    pPixel[0] = ((int)pPixel[0] * pPixel[3] + 127) >> 8;
                    pPixel[1] = ((int)pPixel[1] * pPixel[3] + 127) >> 8;
                    pPixel[2] = ((int)pPixel[2] * pPixel[3] + 127) >> 8;
                    pPixel += 4;
                }
            }
        }

        memcpy(m_bmp.bits(), pData, aWidth * aHeight * 4);

        if (m_bmp.isNull())
        {
            return false;
        }
    }
    else
    {
        m_bmp.fill(Qt::transparent);
    }

    // we dont need this screen DC anymore
    m_width = aWidth;
    m_height = aHeight;

    return true;
}

QImage& CAlphaBitmap::GetBitmap()
{
    return m_bmp;
}

void CAlphaBitmap::Free()
{

}

UINT CAlphaBitmap::GetWidth()
{
    return m_width;
}

UINT CAlphaBitmap::GetHeight()
{
    return m_height;
}

void CheckerboardFillRect(QPainter* pGraphics, const QRect& rRect, int checkDiameter, const QColor& aColor1, const QColor& aColor2)
{
    pGraphics->save();
    pGraphics->setClipRect(rRect);
    // Create a checkerboard background for easier readability
    pGraphics->fillRect(rRect, aColor1);
    QBrush lightBrush(aColor2);

    // QRect bottom/right methods are short one unit for legacy reasons. Compute bottomr/right of the rectange ourselves to get the full size.
    const int rectRight = rRect.x() + rRect.width();
    const int rectBottom = rRect.y() + rRect.height();

    for (int i = rRect.left(); i < rectRight; i += checkDiameter)
    {
        for (int j = rRect.top(); j < rectBottom; j += checkDiameter)
        {
            if ((i / checkDiameter) % 2 ^ (j / checkDiameter) % 2)
            {
                pGraphics->fillRect(QRect(i, j, checkDiameter, checkDiameter), lightBrush);
            }
        }
    }
    pGraphics->restore();
}
