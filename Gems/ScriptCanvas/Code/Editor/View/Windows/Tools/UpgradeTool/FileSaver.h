/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Core.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        struct FileSaveResult
        {
            AZStd::string fileSaveError;
            AZStd::string tempFileRemovalError;
        };

        class FileSaver
        {
        public:
            AZ_CLASS_ALLOCATOR(FileSaver, AZ::SystemAllocator, 0);

            FileSaver
                ( AZStd::function<bool()> onReadOnlyFile
                , AZStd::function<void(const FileSaveResult& result)> onComplete);

            void Save(AZ::Data::Asset<AZ::Data::AssetData> asset);

        private:
            AZStd::function<void(const FileSaveResult& result)> m_onComplete;
            AZStd::function<bool()> m_onReadOnlyFile;

            void OnSourceFileReleased(AZ::Data::Asset<AZ::Data::AssetData> asset);

            void PerformMove
                ( AZStd::string source
                , AZStd::string target
                , size_t remainingAttempts);

            AZStd::string RemoveTempFile(AZStd::string_view tempFile);
        };
    }
}
