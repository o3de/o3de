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

