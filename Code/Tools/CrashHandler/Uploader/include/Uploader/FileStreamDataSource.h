/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
