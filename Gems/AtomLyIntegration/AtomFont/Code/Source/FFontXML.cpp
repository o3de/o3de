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

// Description : XML parsing to load a font.


#include <AtomLyIntegration/AtomFont/AtomFont_precompiled.h>

#if !defined(USE_NULLFONT_ALWAYS)

#include "FFontXML_Internal.h"

#include <AtomLyIntegration/AtomFont/FFont.h>
#include <AtomLyIntegration/AtomFont/FontTexture.h>
#include <CryCommon/Cry_Math.h>
#include <CryCommon/CryPath.h>
#include <AzCore/PlatformIncl.h>

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
    xmlfs.ScanXmlNodesRecursively(root);

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

        // parse the font effects file, adding to this font object
        AtomFontInternal::XmlFontShader xmlfsEffect(this);
        xmlfsEffect.ScanXmlNodesRecursively(fontEffectRoot);
    }

    return true;
}

#endif

