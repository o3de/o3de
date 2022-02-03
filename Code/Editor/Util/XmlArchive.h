/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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

    //! Save XML Archive to pak file.
    //! @return true if saved.
    bool SaveToPak(const QString& levelPath, CPakFile& pakFile);
    bool LoadFromPak(const QString& levelPath, CPakFile& pakFile);
};


#endif // CRYINCLUDE_EDITOR_UTIL_XMLARCHIVE_H
