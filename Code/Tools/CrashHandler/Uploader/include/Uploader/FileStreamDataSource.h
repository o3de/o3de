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


#pragma once

#include <handler/user_stream_data_source.h>
#include "base/files/file_path.h"

namespace O3de
{
    class FileStreamDataSource : public crashpad::UserStreamDataSource
    {
    public:
        FileStreamDataSource(const base::FilePath& filePath);

        std::unique_ptr<crashpad::MinidumpUserExtensionStreamDataSource> ProduceStreamData(crashpad::ProcessSnapshot* process_snapshot) override;

    private:
        FileStreamDataSource(const FileStreamDataSource& rhs) = delete;
        FileStreamDataSource& operator=(const FileStreamDataSource& rhs) = delete;

        base::FilePath m_filePath;
    };
}
