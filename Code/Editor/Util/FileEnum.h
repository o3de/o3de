/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
