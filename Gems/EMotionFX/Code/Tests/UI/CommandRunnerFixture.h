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
