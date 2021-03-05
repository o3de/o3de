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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PAKHELPERS_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PAKHELPERS_H
#pragma once

#include <map>                 // stl multimap<>

#include "RcFile.h"

namespace PakHelpers
{
    enum ESortType
    {
        eSortType_NoSort,               // don't sort files
        eSortType_Size,                 // sort files by size
        eSortType_Streaming,            // sort files by extension+type+name+size
        eSortType_Suffix,               // sort files by suffix+name+extension
        eSortType_Alphabetically,       // sort files by full path
    };

    enum ESplitType
    {
        eSplitType_Original,            // don't create paks differing from the original configuration (XML or command line)
        eSplitType_Basedir,             // split paks by base directory name
        eSplitType_ExtensionMipmap,     // split paks by extension and mipmap level (high/low)
        eSplitType_Suffix,              // split paks by suffix
    };

    enum ETextureType
    {
        eTextureType_Diffuse,
        eTextureType_Normal,
        eTextureType_Specular,
        eTextureType_Detail,
        eTextureType_Mask,
        eTextureType_SubSurfaceScattering,
        eTextureType_Cubemap,
        eTextureType_Colorchart,
        eTextureType_Displacement,
        eTextureType_Undefined,
    };

    struct PakEntry
    {
        PakEntry()
            : m_sourceFileSize(-1)
            , m_bIsLastMip(false)
        {
        }

        RcFile m_rcFile;
        int64 m_sourceFileSize;
        bool m_bIsLastMip;

        string m_streamingSuffix;
        string m_extension;
        ETextureType m_textureType;
        string m_baseName;
        string m_innerDir;

        static bool MakeSortableStreamingSuffix(const string& suffix, string* sortable = NULL, int nDigits = 2, int nIncrement = 0);

        string GetRealFilename() const;
        string GetStreamingSuffix() const;
        string GetExtension() const;
        string GetNameWithoutExtension(bool bFilenameOnly) const;
        string GetDirnameWithoutFile(bool bRootdirOnly) const;

        ETextureType GetTextureType() const;
    };

    size_t CreatePakEntryList(
        const std::vector<RcFile>& files,
        std::map<string, std::vector<PakEntry> >& pakEntries,
        ESortType eSortType,
        ESplitType eSplitType,
        const string& sPakName);
}

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_PAKHELPERS_H
