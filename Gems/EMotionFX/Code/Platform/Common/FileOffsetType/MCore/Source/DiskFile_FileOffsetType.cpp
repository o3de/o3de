/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <MCore/Source/DiskFile.h>
#include <AzCore/PlatformDef.h>

namespace MCore
{
    // returns the position (offset) in the file in bytes
    size_t DiskFile::GetPos() const
    {
        MCORE_ASSERT(m_file);

        return ftello(m_file);
    }


    // seek a given number of bytes ahead from it's current position
    bool DiskFile::Forward(size_t numBytes)
    {
        MCORE_ASSERT(m_file);

        if (fseeko(m_file, numBytes, SEEK_CUR) != 0)
        {
            return false;
        }

        return true;
    }

    // seek to an absolute position in the file (offset in bytes)
    bool DiskFile::Seek(size_t offset)
    {
        MCORE_ASSERT(m_file);

        if (fseeko(m_file, offset, SEEK_SET) != 0)
        {
            return false;
        }

        return true;
    }

    // returns the filesize in bytes
    size_t DiskFile::GetFileSize() const
    {
        MCORE_ASSERT(m_file);
        if (m_file == nullptr)
        {
            return 0;
        }

        // get the current file position
        size_t curPos = GetPos();

        // seek to the end of the file
        fseeko(m_file, 0, SEEK_END);

        // get the position, whis is the size of the file
        size_t fileSize = GetPos();

        // seek back to the original position
        fseeko(m_file, curPos, SEEK_SET);

        // return the size of the file
        return fileSize;
    }

}
