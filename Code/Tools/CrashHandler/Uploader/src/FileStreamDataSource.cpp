/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <FileStreamDataSource.h>
#include <BufferedDataStream.h>

#include <memory>

namespace O3de
{
    FileStreamDataSource::FileStreamDataSource(const base::FilePath& filePath) :
        m_filePath{ filePath }
    {

    }

    std::unique_ptr<crashpad::MinidumpUserExtensionStreamDataSource> FileStreamDataSource::ProduceStreamData(crashpad::ProcessSnapshot* process_snapshot) 
    {
        static constexpr char testBuffer[] = "Test Data From buffer.";

        return std::make_unique<BufferedDataStream>( 0xCAFEBABE, testBuffer, sizeof(testBuffer));
    }
}

