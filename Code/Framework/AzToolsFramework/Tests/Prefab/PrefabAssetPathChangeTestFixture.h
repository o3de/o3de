/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabTestFixture.h>

namespace UnitTest
{
    using namespace AzToolsFramework::Prefab;

    //! Fixture for testing prefab source asset file and path changes
    class PrefabAssetPathChangeTestFixture
        : public PrefabTestFixture
    {
    protected:
        void SetUpEditorFixtureImpl() override;
        InstanceOptionalReference CreatePrefabInstance(AZStd::string_view folderPath, AZStd::string_view fileName);
        void ChangePrefabFolderPath(AZStd::string_view fromFolderPath, AZStd::string_view toFolderPath);
        void ChangePrefabFileName(
            AZStd::string_view folderPath,
            AZStd::string_view fromFileName,
            AZStd::string_view toFileName);

        AZ::IO::Path GetPrefabFilePathForSerialization(AZStd::string_view folderPath, AZStd::string_view fileName);

    private:
        void SendFolderPathNameChangeEvent(AZStd::string_view fromPath, AZStd::string_view toPath);
        void SendFilePathNameChangeEvent(AZStd::string_view fromPath, AZStd::string_view toPath);

        AZ::IO::Path GetAbsoluteFilePathName(AZStd::string_view folderPath, AZStd::string_view fileName);

        // Private members
        AZStd::string m_projectPath;
    };
}
