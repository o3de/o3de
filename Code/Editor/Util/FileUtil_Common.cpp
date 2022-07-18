/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "FileUtil_Common.h"

namespace Common
{
    bool Exists(const QString& strPath, bool boDirectory, IFileUtil::FileDesc* pDesc)
    {
        auto pIPak = GetIEditor()->GetSystem()->GetIPak();
        bool boIsDirectory(false);

        AZ::IO::ArchiveFileIterator nFindHandle = pIPak->FindFirst(strPath.toUtf8().data());
        // If it found nothing, no matter if it is a file or directory, it was not found.
        if (!nFindHandle)
        {
            return false;
        }
        pIPak->FindClose(nFindHandle);

        if ((nFindHandle.m_fileDesc.nAttrib & AZ::IO::FileDesc::Attribute::Subdirectory) == AZ::IO::FileDesc::Attribute::Subdirectory)
        {
            boIsDirectory = true;
        }
        else if (pDesc)
        {
            pDesc->filename = strPath;
            pDesc->attrib = static_cast<unsigned int>(nFindHandle.m_fileDesc.nAttrib);
            pDesc->size = nFindHandle.m_fileDesc.nSize;
            pDesc->time_access = nFindHandle.m_fileDesc.tAccess;
            pDesc->time_create = nFindHandle.m_fileDesc.tCreate;
            pDesc->time_write = nFindHandle.m_fileDesc.tWrite;
        }

        // If we are seeking directories...
        if (boDirectory)
        {
            // The return value will tell us if the found element is a directory.
            return boIsDirectory;
        }
        else
        {
            // If we are not seeking directories...
            // We return true if the found element is not a directory.
            return !boIsDirectory;
        }
    }

    bool PathExists(const QString& strPath)
    {
        return Exists(strPath, true);
    }
}
