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

            const SourceHandle& GetSource() const;
            void Save(const SourceHandle& source);

        private:
            AZStd::mutex m_mutex;

            bool m_sourceFileReleased = false;
            SourceHandle m_source;
            AZStd::function<void(const FileSaveResult& result)> m_onComplete;
            AZStd::function<bool()> m_onReadOnlyFile;

            void OnSourceFileReleased(const SourceHandle& source);
            
            void PerformMove
                ( AZStd::string source
                , AZStd::string target
                , size_t remainingAttempts);

            AZStd::string RemoveTempFile(AZStd::string_view tempFile);
        };
    }
}
