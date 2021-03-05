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

#ifndef CRYINCLUDE_EDITOR_UTIL_FILEENUM_H
#define CRYINCLUDE_EDITOR_UTIL_FILEENUM_H
#pragma once

#include <QDirIterator>

class QFileInfo;
class QString;
class QStringList;

class CFileEnum
{
public:
    CFileEnum();
    virtual ~CFileEnum();
    bool GetNextFile(QFileInfo* pFile);
    bool StartEnumeration(const QString& szEnumPathAndPattern, QFileInfo* pFile);
    bool StartEnumeration(const QString& szEnumPath, const QString& szEnumPattern, QFileInfo* pFile);
    static bool ScanDirectory(
        const QString& path,
        const QString& file,
        QStringList& files,
        bool bRecursive = true,
        bool bSkipPaks = false);

protected:
    QDirIterator* m_hEnumFile;
};
#endif // CRYINCLUDE_EDITOR_UTIL_FILEENUM_H
