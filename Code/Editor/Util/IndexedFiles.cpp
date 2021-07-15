/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Tagged files database for 'SmartFileOpen' dialog

#include "EditorDefs.h"

#include "IndexedFiles.h"

volatile TIntAtomic CIndexedFiles::s_bIndexingDone;
CIndexedFiles* CIndexedFiles::s_pIndexedFiles = NULL;

bool CIndexedFiles::m_startedFileIndexing = false;

void CIndexedFiles::Initialize(const QString& path, IFileUtil::ScanDirectoryUpdateCallBack updateCB)
{
    m_files.clear();
    m_pathToIndex.clear();
    m_tags.clear();
    m_rootPath = path;

    bool anyFiles = CFileUtil::ScanDirectory(path, "*.*", m_files, true, true, updateCB);

    if (anyFiles == false)
    {
        m_files.clear();
        return;
    }

    if (updateCB)
    {
        updateCB("Parsing & tagging...");
    }

    for (int i = 0; i < m_files.size(); ++i)
    {
        m_pathToIndex[m_files[i].filename] = i;
    }

    PrepareTagTable();

    InvokeUpdateCallbacks();
}

void CIndexedFiles::AddFile(const IFileUtil::FileDesc& path)
{
    assert(m_pathToIndex.find(path.filename) == m_pathToIndex.end());
    m_files.push_back(path);
    m_pathToIndex[path.filename] = m_files.size() - 1;
    QStringList tags;
    GetTags(tags, path.filename);
    for (int k = 0; k < tags.size(); ++k)
    {
        m_tags[tags[k]].insert(m_files.size() - 1);
    }
}

void CIndexedFiles::RemoveFile(const QString& path)
{
    if (m_pathToIndex.find(path) == m_pathToIndex.end())
    {
        return;
    }
    std::map<QString, int>::iterator itr = m_pathToIndex.find(path);
    int index = itr->second;
    m_pathToIndex.erase(itr);
    m_files.erase(m_files.begin() + index);
    QStringList tags;
    GetTags(tags, path);
    for (int k = 0; k < tags.size(); ++k)
    {
        m_tags[tags[k]].erase(index);
    }
}

void CIndexedFiles::Refresh(const QString& path, bool recursive)
{
    IFileUtil::FileArray files;
    bool anyFiles = CFileUtil::ScanDirectory(m_rootPath, Path::Make(path, "*.*"), files, recursive, recursive ? true : false);

    if (anyFiles == false)
    {
        return;
    }

    for (int i = 0; i < files.size(); ++i)
    {
        if (m_pathToIndex.find(files[i].filename) == m_pathToIndex.end())
        {
            AddFile(files[i]);
        }
    }

    InvokeUpdateCallbacks();
}

void CIndexedFiles::GetFilesWithTags(IFileUtil::FileArray& files, const QStringList& tags) const
{
    files.clear();
    if (tags.empty())
    {
        return;
    }
    int_set candidates;
    TagTable::const_iterator i;
    // Gets candidate files from the first tag.
    for (i = m_tags.begin(); i != m_tags.end(); ++i)
    {
        if (i->first.startsWith(tags[0]))
        {
            candidates.insert(i->second.begin(), i->second.end());
        }
    }
    // Reduces the candidates further using additional tags, if any.
    for (int k = 1; k < tags.size(); ++k)
    {
        // Gathers the filter set.
        int_set filter;
        for (i = m_tags.begin(); i != m_tags.end(); ++i)
        {
            if (i->first.startsWith(tags[k]))
            {
                filter.insert(i->second.begin(), i->second.end());
            }
        }

        // Filters the candidates using it.
        for (int_set::iterator m = candidates.begin(); m != candidates.end(); )
        {
            if (filter.find(*m) == filter.end())
            {
                int_set::iterator target = m;
                ++m;
                candidates.erase(target);
            }
            else
            {
                ++m;
            }
        }
    }
    // Outputs the result.
    files.reserve(candidates.size());
    for (int_set::const_iterator m = candidates.begin(); m != candidates.end(); ++m)
    {
        files.push_back(m_files[*m]);
    }
}

void CIndexedFiles::GetTags(QStringList& tags, const QString& path) const
{
    tags = path.split(QRegularExpression(QStringLiteral(R"([\\/.])")), Qt::SkipEmptyParts);
}

void CIndexedFiles::GetTagsOfPrefix(QStringList& tags, const QString& prefix) const
{
    tags.clear();
    TagTable::const_iterator i;
    for (i = m_tags.begin(); i != m_tags.end(); ++i)
    {
        if (i->first.startsWith(prefix))
        {
            tags.push_back(i->first);
        }
    }
}

void CIndexedFiles::PrepareTagTable()
{
    QStringList tags;
    for (int i = 0; i < m_files.size(); ++i)
    {
        GetTags(tags, m_files[i].filename);
        for (int k = 0; k < tags.size(); ++k)
        {
            m_tags[tags[k]].insert(i);
        }
    }
}

void CIndexedFiles::AddUpdateCallback(std::function<void()> updateCallback)
{
    CryAutoLock<CryMutex> lock(m_updateCallbackMutex);

    m_updateCallbacks.push_back(updateCallback);
}

void CIndexedFiles::InvokeUpdateCallbacks()
{
    CryAutoLock<CryMutex> lock(m_updateCallbackMutex);

    for (auto updateCallback : m_updateCallbacks)
    {
        updateCallback();
    }
}
