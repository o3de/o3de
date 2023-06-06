/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzTest/GemTestEnvironment.h>

class ArchiveEditorTestEnvironment
    : public AZ::Test::GemTestEnvironment
{
public:
    void AddGemsAndComponents() override
    {
        // Load the (lib)Compression.Editor.(dll|so|dylib) file
        // in the Archive Test code
        // The AzTestRunner explicitly loads this module: (lib)Archive.Editor.Tests.(dll|so|dylib)
        // via its first command line argument
        AddDynamicModulePaths({ "Compression.Editor" });
        // Set both this Gem "Archive" and the Compression Gem has active
        AddActiveGems(AZStd::to_array<AZStd::string_view>({ "Archive", "Compression" }));
    }
};

AZ_UNIT_TEST_HOOK(new ArchiveEditorTestEnvironment);
