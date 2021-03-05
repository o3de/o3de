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
#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ASSETFILEINFO_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ASSETFILEINFO_H
#pragma once

#include <cstring>    // memset()
#include "SimpleString.h"

class CAssetFileInfo
{
public:
    enum
    {
        kMaxCgfLods = 6
    };

    enum EType
    {
        eUnknown,
        eTexture,
        eCGF,
        eCHR,
        eCAF,
        eLUA
    };

    struct TextureInfo
    {
        int w;
        int h;
        bool bAlpha;
        SimpleString format;
        SimpleString type;
        int nNumMips;
        int nDepth;
        int nSides;
    };

    struct GeometryInfo
    {
        int nVertices;
        int nIndices;
        int nIndicesPerLod[kMaxCgfLods];
        int nMeshSizePerLod[kMaxCgfLods];
        int nMeshSize;
        int nPhysProxySize;
        int nPhysTriCount;
        int nPhysProxyCount;
        int nLods;
        int nSubMeshCount;
        int nJoints;
        bool bSplitLods;
    };

    struct SourceControl
    {
        bool bValid;
        SimpleString user;
        SimpleString user_email;
        SimpleString user_fullname;
        SimpleString workspace;
        SimpleString depotFile;
        SimpleString changeDescription;
        int change;
        int revision;
        int time;
    };

public:
    EType         m_type;
    int64         m_SrcFileSize;
    int64         m_DstFileSize;

    SimpleString  m_sInfo;            // separated list of properties (used for excel export)
    SimpleString  m_sSourceFilename;
    SimpleString  m_sDestFilename;
    SimpleString  m_sPreset;
    SimpleString  m_sErrorLog;

    bool          m_bSuccess;
    bool          m_bGetSourceControlInfo;
    bool          m_bReferencedInLevels;

    TextureInfo   m_textureInfo;
    GeometryInfo  m_geomInfo;
    SourceControl m_sc;

public:
    CAssetFileInfo()
    {
        memset(this, 0, sizeof(*this));
        m_type = eUnknown;
        m_bSuccess = true;
        m_bGetSourceControlInfo = true;
    }

    template <class T>
    void SafeStrCopy(T& to, const char* from)
    {
        to = from;
    }
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_ASSETFILEINFO_H
