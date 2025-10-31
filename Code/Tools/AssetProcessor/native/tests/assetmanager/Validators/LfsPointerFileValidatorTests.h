/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#pragma once

#include <native/tests/assetmanager/AssetManagerTestingBase.h>
#include <native/AssetManager/Validators/LfsPointerFileValidator.h>

namespace UnitTests
{
    struct LfsPointerFileValidatorTests
        : AssetManagerTestingBase
    {
        void SetUp() override;
        void TearDown() override;

        bool CreateTestFile(const AZStd::string& filePath, const AZStd::string& content);
        bool RemoveTestFile(const AZStd::string& filePath);

        AZ::IO::Path m_assetRootDir;
        AssetProcessor::LfsPointerFileValidator m_validator;
    };
}


