/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : XML parsing to load a font.



#if !defined(USE_NULLFONT_ALWAYS)

#include "FFontXML_Internal.h"

#include <AtomLyIntegration/AtomFont/FFont.h>
#include <AtomLyIntegration/AtomFont/FontTexture.h>
#include <CryCommon/Cry_Math.h>
#include <CryCommon/CryPath.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Serialization/Locale.h>

//////////////////////////////////////////////////////////////////////////
// Main loading function
bool AZ::FFont::Load(const char* xmlFile)
{
    m_curPath = "";
    if (xmlFile)
    {
        m_curPath = PathUtil::GetPath(xmlFile);
    }

    XmlNodeRef root = GetISystem()->LoadXmlFromFile(xmlFile);
    if (!root)
    {
        return false;
    }

    
    AtomFontInternal::XmlFontShader xmlfs(this);

    {
        // use the invariant culture so that if the user has a machine that has comma as the decimal separator,
        // the font file will still be parsed correctly.
        AZ::Locale::ScopedSerializationLocale scopedLocale;

        xmlfs.ScanXmlNodesRecursively(root);
    }
    // if this was not a valid font XML file then return false
    if (!m_fontTexture || !m_fontBuffer)
    {
        return false;
    }

    // if there was a font effect file then parse that for effects
    if (!xmlfs.m_strFontEffectPath.empty())
    {
        XmlNodeRef fontEffectRoot = GetISystem()->LoadXmlFromFile(xmlfs.m_strFontEffectPath.c_str());
        if (!fontEffectRoot)
        {
            AZ_Warning("Font", false, "Error parsing font file %s, 'effectfile' pathname %s could not be found.",
                xmlFile, xmlfs.m_strFontEffectPath.c_str());
            return false;
        }

        if (m_effects.size() > 1 || (m_effects.size() == 1 && (m_effects[0].m_name != "default" || m_effects[0].m_passes.size() > 1)))
        {
            AZ_Warning("Font", false, "Error parsing font file %s, 'effectfile' and 'effect' cannot both be used in the same font file.",
                xmlFile);
            m_effects.clear();
        }

        AZ::Locale::ScopedSerializationLocale scopedLocale;

        // parse the font effects file, adding to this font object
        AtomFontInternal::XmlFontShader xmlfsEffect(this);
        xmlfsEffect.ScanXmlNodesRecursively(fontEffectRoot);
    }

    return true;
}

#endif

