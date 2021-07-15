/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "FileEnum.h"

CFileEnum::CFileEnum()
    : m_hEnumFile(0)
{
}

CFileEnum::~CFileEnum()
{
    if (m_hEnumFile)
    {
        delete m_hEnumFile;
        m_hEnumFile = 0;
    }
}

bool CFileEnum::StartEnumeration(
    const QString& szEnumPath,
    const QString& szEnumPattern,
    QFileInfo* pFile)
{
    //////////////////////////////////////////////////////////////////////
    // Take path and search pattern as separate arguments
    //////////////////////////////////////////////////////////////////////

    // Build enumeration path
    QString szPath = szEnumPath;

    if (!szPath.endsWith('\\') && !szPath.endsWith('/'))
    {
        szPath += QDir::separator();
    }

    szPath += szEnumPattern;

    return StartEnumeration(szPath, pFile);
}

bool CFileEnum::StartEnumeration(const QString& szEnumPathAndPattern, QFileInfo* pFile)
{
    // End any previous enumeration
    if (m_hEnumFile)
    {
        delete m_hEnumFile;
        m_hEnumFile = 0;
    }

    QStringList parts = szEnumPathAndPattern.split(QRegularExpression(R"([\\/])"));
    QString pattern = parts.takeLast();
    QString path = parts.join(QDir::separator());

    // Start the enumeration
    m_hEnumFile = new QDirIterator(path, QStringList(pattern));
    if (!m_hEnumFile->hasNext())
    {
        // No files found
        delete m_hEnumFile;
        m_hEnumFile = 0;

        return false;
    }

    m_hEnumFile->next();
    *pFile = m_hEnumFile->fileInfo();

    return true;
}

bool CFileEnum::GetNextFile(QFileInfo* pFile)
{
    // Fill file strcuture
    if (!m_hEnumFile->hasNext())
    {
        // No more files left
        delete m_hEnumFile;
        m_hEnumFile = 0;

        return false;
    }

    m_hEnumFile->next();
    *pFile = m_hEnumFile->fileInfo();

    // At least one file left
    return true;
}

inline bool ScanDirectoryRecursive(
    const QString& root,
    const QString& path,
    const QString& file,
    QStringList& files,
    bool bRecursive)
{
    bool bFoundAny = false;

    QDirIterator hFile(root + path, QStringList(file));

    if (hFile.hasNext())
    {

        // Find the rest of the files.
        do
        {
            hFile.next();
            QFileInfo foundFile = hFile.fileInfo();

            bFoundAny = true;
            files.push_back(path + foundFile.fileName());
        }   while (hFile.hasNext());
    }

    if (bRecursive)
    {
        QDirIterator hFile2(root + path, QStringList("*.*"));

        if (hFile2.hasNext())
        {
            // Find directories.
            do
            {
                hFile2.next();
                QFileInfo foundFile = hFile2.fileInfo();

                if (foundFile.isDir())
                {
                    // If recursive.
                    if (!foundFile.fileName().startsWith('.'))
                    {
                        if (ScanDirectoryRecursive(
                                root,
                                path + foundFile.fileName() + QDir::separator(),
                                file,
                                files,
                                bRecursive))
                        {
                            bFoundAny = true;
                        }
                    }
                }
            }   while (hFile2.hasNext());
        }
    }

    return bFoundAny;
}

bool CFileEnum::ScanDirectory(
    const QString& path,
    const QString& file,
    QStringList& files,
    bool bRecursive,
    bool /* bSkipPaks - unused, left for backwards API compatibility */)
{
    return ScanDirectoryRecursive(path, "", file, files, bRecursive);
}
