/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/UI/CommandRunnerFixture.h>

namespace EMotionFX
{
    INSTANTIATE_TEST_SUITE_P(CanAdjustGroupParameter, CommandRunnerFixture,
        ::testing::Values(std::vector<std::string> {
            R"str(CreateAnimGraph -animGraphID 100)str",
            R"str(AnimGraphAddGroupParameter -animGraphID 100 -name Group0)str",
            R"str(AnimGraphAdjustParameter -animGraphID 100 -name Group0 -newName Group01 -type {6B42666E-82D7-431E-807E-DA789C53AF05} -contents <ObjectStream version="3">
                    <Class name="GroupParameter" version="1" type="{6B42666E-82D7-431E-807E-DA789C53AF05}">
                            <Class name="Parameter" field="BaseClass1" version="1" type="{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
                                    <Class name="AZStd::string" field="name" value="Group01" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                    <Class name="AZStd::string" field="description" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                            </Class>
                            <Class name="AZStd::vector" field="childParameters" type="{496E6454-2F91-5CDC-9771-3B589F3F8FEB}"/>
                    </Class>
            </ObjectStream>)str",
        }
    ));
} // end namespace EMotionFX
