/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Tests/UI/UIFixture.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    class CommandRunnerFixtureBase
        : public UIFixture
    {
    public:
        void TearDown() override;

        void ExecuteCommands(std::vector<std::string> commands);

        const AZStd::vector<AZStd::string>& GetResults();
    private:
        AZStd::vector<AZStd::string> m_results;
    };

    class CommandRunnerFixture
        : public CommandRunnerFixtureBase
        , public ::testing::WithParamInterface<std::vector<std::string>>
    {
    };
} // end namespace EMotionFX
