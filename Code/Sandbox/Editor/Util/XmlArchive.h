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

// Description : Stores XML in MFC archive.


#ifndef CRYINCLUDE_EDITOR_UTIL_XMLARCHIVE_H
#define CRYINCLUDE_EDITOR_UTIL_XMLARCHIVE_H
#pragma once


#include "NamedData.h"
#include "EditorUtils.h"

class CPakFile;
/*!
 *  CXmlArcive used to stores XML in MFC archive.
 */
class CXmlArchive
{
public:
    XmlNodeRef  root;
    CNamedData* pNamedData;
    bool                bLoading;
    bool                bOwnNamedData;

    CXmlArchive()
    {
        bLoading = false;
        bOwnNamedData = true;
        pNamedData = new CNamedData;
    };
    explicit CXmlArchive(const QString& xmlRoot)
    {
        bLoading = false;
        bOwnNamedData = true;
        pNamedData = new CNamedData;
        root = XmlHelpers::CreateXmlNode(xmlRoot.toUtf8().data());
    };
    ~CXmlArchive()
    {
        if (bOwnNamedData)
        {
            delete pNamedData;
        }
    };
    CXmlArchive(const CXmlArchive& ar) { *this = ar; }
    CXmlArchive& operator=(const CXmlArchive& ar)
    {
        root = ar.root;
        pNamedData = ar.pNamedData;
        bLoading = ar.bLoading;
        bOwnNamedData = false;
        return *this;
    }

    bool Load(const QString& file);
    void Save(const QString& file);

    //! Save XML Archive to pak file.
    //! @return true if saved.
    bool SaveToPak(const QString& levelPath, CPakFile& pakFile);
    bool LoadFromPak(const QString& levelPath, CPakFile& pakFile);
};


#endif // CRYINCLUDE_EDITOR_UTIL_XMLARCHIVE_H
