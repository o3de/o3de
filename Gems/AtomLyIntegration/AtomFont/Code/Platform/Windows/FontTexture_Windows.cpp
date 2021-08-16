/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#if !defined(USE_NULLFONT_ALWAYS)
#include <AtomLyIntegration/AtomFont/FontTexture.h>

//-------------------------------------------------------------------------------------------------
int AZ::FontTexture::WriteToFile(const AZStd::string& fileName)
{
    AZ::IO::FileIOStream outputFile(fileName.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);

    if (!outputFile.IsOpen())
    {
        return 0;
    }

    BITMAPFILEHEADER pHeader;
    BITMAPINFOHEADER pInfoHeader;

    memset(&pHeader, 0, sizeof(BITMAPFILEHEADER));
    memset(&pInfoHeader, 0, sizeof(BITMAPINFOHEADER));

    pHeader.bfType = 0x4D42;
    pHeader.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + m_width * m_height * 3;
    pHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    pInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
    pInfoHeader.biWidth = m_width;
    pInfoHeader.biHeight = m_height;
    pInfoHeader.biPlanes = 1;
    pInfoHeader.biBitCount = 24;
    pInfoHeader.biCompression = 0;
    pInfoHeader.biSizeImage = m_width * m_height * 3;

    outputFile.Write(sizeof(BITMAPFILEHEADER), &pHeader);
    outputFile.Write(sizeof(BITMAPINFOHEADER), &pInfoHeader);

    unsigned char cRGB[3];

    for (int i = m_height - 1; i >= 0; i--)
    {
        for (int j = 0; j < m_width; j++)
        {
            cRGB[0] = m_buffer[(i * m_width) + j];
            cRGB[1] = *cRGB;

            cRGB[2] = *cRGB;

            outputFile.Write(3, cRGB);
        }
    }

    return 1;
}

#endif
