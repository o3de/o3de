/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FFontXML_Internal.h>

#include <shlobj_core.h>

namespace AtomFontInternal
{
    void XmlFontShader::FoundElementImpl()
    {
        TCHAR sysFontPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(0, CSIDL_FONTS, 0, SHGFP_TYPE_DEFAULT, sysFontPath)))
        {
            const char* fontPath = m_strFontPath.c_str();
            const char* fontName = CryStringUtils::FindFileNameInPath(fontPath);

            string newFontPath(sysFontPath);
            newFontPath += "/";
            newFontPath += fontName;
            m_font->Load(newFontPath, m_FontTexSize.x, m_FontTexSize.y, m_slotSizes.x, m_slotSizes.y, CreateTTFFontFlag(m_FontSmoothMethod, m_FontSmoothAmount), m_SizeRatio);
        }
    }
}
