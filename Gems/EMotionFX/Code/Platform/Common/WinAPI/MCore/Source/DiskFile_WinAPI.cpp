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

#include <MCore/Source/DiskFile.h>
#include <AzCore/PlatformDef.h>

namespace MCore
{
    // returns the position (offset) in the file in bytes
    size_t DiskFile::GetPos() const
    {
        MCORE_ASSERT(mFile);

        return _ftelli64(mFile);
    }


    // seek a given number of bytes ahead from it's current position
    bool DiskFile::Forward(size_t numBytes)
    {
        MCORE_ASSERT(mFile);

        if (_fseeki64(mFile, numBytes, SEEK_CUR) != 0)
        {
            return false;
        }

        return true;
    }

    // seek to an absolute position in the file (offset in bytes)
    bool DiskFile::Seek(size_t offset)
    {
        MCORE_ASSERT(mFile);

        if (_fseeki64(mFile, offset, SEEK_SET) != 0)
        {
            return false;
        }

        return true;
    }

    // returns the filesize in bytes
    size_t DiskFile::GetFileSize() const
    {
        MCORE_ASSERT(mFile);
        if (mFile == nullptr)
        {
            return 0;
        }

        // get the current file position
        size_t curPos = GetPos();

        // seek to the end of the file
        _fseeki64(mFile, 0, SEEK_END);

        // get the position, whis is the size of the file
        size_t fileSize = GetPos();

        // seek back to the original position
        _fseeki64(mFile, curPos, SEEK_SET);

        // return the size of the file
        return fileSize;
    }

}