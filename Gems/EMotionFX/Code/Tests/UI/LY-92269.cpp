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

#include <Tests/UI/CommandRunnerFixture.h>

namespace EMotionFX
{
    INSTANTIATE_TEST_CASE_P(LY92269, CommandRunnerFixture,
        ::testing::Values(std::vector<std::string> {
            R"str(CreateAnimGraph)str",
            R"str(AnimGraphCreateNode -animGraphID 0 -type {A8B5BB1E-5BA9-4B0A-88E9-21BB7A199ED2} -parentName Root -xPos 240 -yPos 230 -name GENERATE -namePrefix BlendTree)str",
            R"str(AnimGraphCreateNode -animGraphID 0 -type {1A755218-AD9D-48EA-86FC-D571C11ECA4D} -parentName BlendTree0 -xPos 0 -yPos 0 -name GENERATE -namePrefix FinalNode)str",
            R"str(AnimGraphCreateNode -animGraphID 0 -type {4510529A-323F-40F6-B773-9FA8FC4DE53D} -parentName BlendTree0 -xPos -120 -yPos 30 -name GENERATE -namePrefix Parameters)str",
            R"str(AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name Parameter0 -contents <ObjectStream version="3">
                <Class name="FloatSliderParameter" version="1" type="{2ED6BBAF-5C82-4EAA-8678-B220667254F2}">
                    <Class name="FloatParameter" field="BaseClass1" version="1" type="{0F0B8531-0B07-4D9B-A8AC-3A32D15E8762}">
                        <Class name="(RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt;" field="BaseClass1" version="1" type="{01CABBF8-9500-5ABB-96BD-9989198146C2}">
                            <Class name="(DefaultValueParameter&lt;ValueType, Derived&gt;)&lt;float (RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt; &gt;" field="BaseClass1" version="1" type="{3221F118-9372-5BA3-BD8B-E88267CB356B}">
                                <Class name="ValueParameter" field="BaseClass1" version="1" type="{46549C79-6B4C-4DDE-A5E3-E5FBEC455816}">
                                    <Class name="Parameter" field="BaseClass1" version="1" type="{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
                                        <Class name="AZStd::string" field="name" value="Parameter0" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                        <Class name="AZStd::string" field="description" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                    </Class>
                                </Class>
                                <Class name="float" field="defaultValue" value="0.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            </Class>
                            <Class name="bool" field="hasMinValue" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="float" field="minValue" value="0.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="bool" field="hasMaxValue" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="float" field="maxValue" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                        </Class>
                    </Class>
                </Class>
            </ObjectStream>)str",
            R"str(AnimGraphCreateParameter -animGraphID 0 -type {2ED6BBAF-5C82-4EAA-8678-B220667254F2} -name Parameter1 -contents <ObjectStream version="3">
                <Class name="FloatSliderParameter" version="1" type="{2ED6BBAF-5C82-4EAA-8678-B220667254F2}">
                    <Class name="FloatParameter" field="BaseClass1" version="1" type="{0F0B8531-0B07-4D9B-A8AC-3A32D15E8762}">
                        <Class name="(RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt;" field="BaseClass1" version="1" type="{01CABBF8-9500-5ABB-96BD-9989198146C2}">
                            <Class name="(DefaultValueParameter&lt;ValueType, Derived&gt;)&lt;float (RangedValueParameter&lt;ValueType, Derived&gt;)&lt;float FloatParameter &gt; &gt;" field="BaseClass1" version="1" type="{3221F118-9372-5BA3-BD8B-E88267CB356B}">
                                <Class name="ValueParameter" field="BaseClass1" version="1" type="{46549C79-6B4C-4DDE-A5E3-E5FBEC455816}">
                                    <Class name="Parameter" field="BaseClass1" version="1" type="{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
                                        <Class name="AZStd::string" field="name" value="Parameter1" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                        <Class name="AZStd::string" field="description" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                    </Class>
                                </Class>
                                <Class name="float" field="defaultValue" value="0.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            </Class>
                            <Class name="bool" field="hasMinValue" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="float" field="minValue" value="0.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                            <Class name="bool" field="hasMaxValue" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="float" field="maxValue" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
                        </Class>
                    </Class>
                </Class>
            </ObjectStream>)str",
            R"str(AnimGraphRemoveParameter -animGraphID 0 -name Parameter0)str",
            R"str(UNDO)str",
            R"str(AnimGraphAdjustNode -animGraphID 0 -name Parameters0 -attributesString -parameterNames {<ObjectStream version="3">
                <Class name="AZStd::vector" type="{99DAD0BC-740E-5E82-826B-8FC7968CC02C}">
                    <Class name="AZStd::string" field="element" value="Parameter1" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                </Class>
            </ObjectStream>
            })str",
            R"str(AnimGraphRemoveParameter -animGraphID 0 -name Parameter1)str"
        }
    ));
} // end namespace EMotionFX
