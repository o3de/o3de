/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FFontXML_Internal.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/conversions.h>

#include <shlobj_core.h>

namespace AtomFontInternal
{
    void XmlFontShader::FoundElementImpl()
    {
        wchar_t sysFontPathW[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(0, CSIDL_FONTS, 0, SHGFP_TYPE_DEFAULT, sysFontPathW)))
        {
            const AZ::IO::PathView fontName = AZ::IO::PathView(m_strFontPath.c_str()).Filename();

            AZStd::string sysFontPath;
            AZStd::to_string(sysFontPath, sysFontPathW);
            AZ::IO::Path newFontPath(sysFontPath);
            newFontPath /= fontName;
            m_font->Load(newFontPath.c_str(), m_FontTexSize.x, m_FontTexSize.y, m_slotSizes.x, m_slotSizes.y, CreateTTFFontFlag(m_FontSmoothMethod, m_FontSmoothAmount), m_SizeRatio);
        }
    }
}
