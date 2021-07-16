/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/SystemComponentFixture.h>

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>

namespace EMotionFX
{
    // Developer code and APIs with exclusionary terms will be deprecated as we introduce replacements across this project's related
    // codebases and APIs. Please note, some instances have been retained in the current version to provide backward compatibility
    // for assets/materials created prior to the change. These will be deprecated in the future.
    // Those tests validate the conversion of blendspace1d and blendspace2d node.

    TEST_F(SystemComponentFixture, TestLeaderFollowerConversionBlendSpace1D)
    {
        const AZStd::string buffer =
            R"(<ObjectStream version="3">
                <Class name="AnimGraph" version="1" type="{BD543125-CFEE-426C-B0AC-129F2A4C6BC8}">
                    <Class name="GroupParameter" field="rootGroupParameter" version="1" type="{6B42666E-82D7-431E-807E-DA789C53AF05}">
                        <Class name="Parameter" field="BaseClass1" version="1" type="{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
                            <Class name="AZStd::string" field="name" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                            <Class name="AZStd::string" field="description" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                        </Class>
                        <Class name="AZStd::vector" field="childParameters" type="{496E6454-2F91-5CDC-9771-3B589F3F8FEB}"/>
                    </Class>
                    <Class name="AnimGraphStateMachine" field="rootStateMachine" version="1" type="{272E90D2-8A18-46AF-AD82-6A8B7EC508ED}">
                        <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                            <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                            <Class name="AZ::u64" field="id" value="5624789762880256302" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                            <Class name="AZStd::string" field="name" value="Root" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                            <Class name="int" field="posX" value="0" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                            <Class name="int" field="posY" value="0" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                            <Class name="Color" field="visualizeColor" value="0.1254902 0.6470588 0.8549020 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                            <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                            <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}">
                                <Class name="BlendTree" field="element" version="1" type="{A8B5BB1E-5BA9-4B0A-88E9-21BB7A199ED2}">
                                    <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                                        <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                                        <Class name="AZ::u64" field="id" value="12099635065983060864" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                        <Class name="AZStd::string" field="name" value="BlendTree0" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                        <Class name="int" field="posX" value="530" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                        <Class name="int" field="posY" value="360" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                        <Class name="Color" field="visualizeColor" value="0.6784314 0.8705883 1.0000000 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                                        <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                        <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                        <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                        <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}">
                                            <Class name="BlendTreeFinalNode" field="element" version="1" type="{1A755218-AD9D-48EA-86FC-D571C11ECA4D}">
                                                <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                                                    <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                                                    <Class name="AZ::u64" field="id" value="6320198947116385307" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                    <Class name="AZStd::string" field="name" value="FinalNode0" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                    <Class name="int" field="posX" value="-39" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                    <Class name="int" field="posY" value="10" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                    <Class name="Color" field="visualizeColor" value="0.8235295 0.9803922 0.9803922 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                                                    <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                    <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                    <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                    <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}"/>
                                                    <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}">
                                                        <Class name="BlendTreeConnection" field="element" version="2" type="{B48FFEDB-87FB-4085-AE54-0302AC49373A}">
                                                            <Class name="AZ::u64" field="id" value="7087270156229006524" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                            <Class name="AZ::u64" field="sourceNodeId" value="4148000174371361859" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                            <Class name="unsigned short" field="sourcePortNr" value="0" type="{ECA0B403-C4F8-4B86-95FC-81688D046E40}"/>
                                                            <Class name="unsigned short" field="targetPortNr" value="0" type="{ECA0B403-C4F8-4B86-95FC-81688D046E40}"/>
                                                        </Class>
                                                    </Class>
                                                    <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                                        <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                                                    </Class>
                                                </Class>
                                            </Class>)"
            R"(                                 <Class name="BlendSpace1DNode" field="element" version="1" type="{E41B443C-8423-4764-97F0-6C57997C2E5B}">
                                                <Class name="BlendSpaceNode" field="BaseClass1" version="2" type="{11EC99C4-6A25-4610-86FD-B01F2E53007E}">
                                                    <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                                                        <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                                                        <Class name="AZ::u64" field="id" value="4148000174371361859" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                        <Class name="AZStd::string" field="name" value="BlendSpace1D0" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="int" field="posX" value="-250" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                        <Class name="int" field="posY" value="10" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                        <Class name="Color" field="visualizeColor" value="1.0000000 0.0000000 1.0000000 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                                                        <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                        <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                        <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                        <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}"/>
                                                        <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}"/>
                                                        <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                                            <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                                                        </Class>
                                                    </Class>
                                                    <Class name="bool" field="retarget" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                    <Class name="bool" field="inPlace" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                </Class>
                                                <Class name="unsigned char" field="calculationMethod" value="0" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                <Class name="AZ::Uuid" field="evaluatorType" value="{17D8679E-5760-481C-9411-5A97D22BC745}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                                                <Class name="unsigned char" field="syncMode" value="1" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                <Class name="AZStd::string" field="syncMasterMotionId" value="rin_walk_kick_04" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                <Class name="unsigned char" field="eventFilterMode" value="0" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                <Class name="AZStd::vector" field="motions" type="{2C5B7168-99DB-5908-8EDB-029D3793CA0A}">
                                                    <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                        <Class name="AZStd::string" field="motionId" value="rin_run" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                        <Class name="unsigned char" field="typeFlags" value="2" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    </Class>
                                                    <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                        <Class name="AZStd::string" field="motionId" value="rin_walk_kick_04" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                        <Class name="unsigned char" field="typeFlags" value="2" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    </Class>
                                                    <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                        <Class name="AZStd::string" field="motionId" value="rin_turn_180_clockwise" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                        <Class name="unsigned char" field="typeFlags" value="2" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    </Class>
                                                    <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                        <Class name="AZStd::string" field="motionId" value="rin_stand_kick_punch_02" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                        <Class name="unsigned char" field="typeFlags" value="2" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    </Class>
                                                    <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                        <Class name="AZStd::string" field="motionId" value="rin_stand_kick_punch_05" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                        <Class name="unsigned char" field="typeFlags" value="2" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    </Class>
                                                    <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                        <Class name="AZStd::string" field="motionId" value="rin_shuffle_turn_right" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                        <Class name="unsigned char" field="typeFlags" value="2" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    </Class>)"
            R"(                                         <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                        <Class name="AZStd::string" field="motionId" value="rin_idle" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                        <Class name="unsigned char" field="typeFlags" value="2" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    </Class>
                                                </Class>
                                            </Class>
                                        </Class>
                                        <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}"/>
                                        <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                            <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                                        </Class>
                                    </Class>
                                    <Class name="AZ::u64" field="finalNodeId" value="6320198947116385307" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                </Class>
                            </Class>
                            <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}"/>
                            <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                            </Class>
                        </Class>
                        <Class name="AZ::u64" field="entryStateId" value="12099635065983060864" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                        <Class name="AZStd::vector" field="transitions" type="{7BE4AF47-DDD5-5B41-9BE0-6739C8A2694E}"/>
                        <Class name="bool" field="alwaysStartInEntryState" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                    </Class>
                    <Class name="AZStd::vector" field="nodeGroups" type="{9A7148E6-1DCA-5BDA-80D7-5FF35D43170E}"/>
                    <Class name="AnimGraphGameControllerSettings" field="gameControllerSettings" version="1" type="{05DF1B3B-2073-4E6D-B5B6-7B87F46CCCB7}">
                        <Class name="unsigned int" field="activePresetIndex" value="4294967295" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
                        <Class name="AZStd::vector" field="presets" type="{7EF271EC-720F-5899-B522-39F46AEC02F1}"/>
                    </Class>
                    <Class name="bool" field="retarget" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                </Class>
            </ObjectStream>
        )";

        // Verify if the property got converted correctly.
        AnimGraph* graph = AZ::Utils::LoadObjectFromBuffer<AnimGraph>(buffer.c_str(), buffer.size(), GetSerializeContext());
        ASSERT_TRUE(graph != nullptr);
        AnimGraphNode* node = graph->RecursiveFindNodeByName("BlendSpace1D0");
        ASSERT_TRUE(node != nullptr);
        BlendSpace1DNode* blendSpaceNode = azrtti_cast<BlendSpace1DNode*>(node);
        ASSERT_TRUE(blendSpaceNode != nullptr);
        EXPECT_STREQ(blendSpaceNode->GetSyncLeaderMotionId().c_str(), "rin_walk_kick_04");
        delete graph;
    }

    TEST_F(SystemComponentFixture, TestLeaderFollowerConversionBlendSpace2D)
    {
        const AZStd::string buffer =
            R"(<ObjectStream version="3">
                    <Class name="AnimGraph" version="1" type="{BD543125-CFEE-426C-B0AC-129F2A4C6BC8}">
                        <Class name="GroupParameter" field="rootGroupParameter" version="1" type="{6B42666E-82D7-431E-807E-DA789C53AF05}">
                            <Class name="Parameter" field="BaseClass1" version="1" type="{4AF0BAFC-98F8-4EA3-8946-4AD87D7F2A6C}">
                                <Class name="AZStd::string" field="name" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                <Class name="AZStd::string" field="description" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                            </Class>
                            <Class name="AZStd::vector" field="childParameters" type="{496E6454-2F91-5CDC-9771-3B589F3F8FEB}"/>
                        </Class>
                        <Class name="AnimGraphStateMachine" field="rootStateMachine" version="1" type="{272E90D2-8A18-46AF-AD82-6A8B7EC508ED}">
                            <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                                <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                                <Class name="AZ::u64" field="id" value="5624789762880256302" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                <Class name="AZStd::string" field="name" value="Root" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                <Class name="int" field="posX" value="0" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                <Class name="int" field="posY" value="0" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                <Class name="Color" field="visualizeColor" value="0.1254902 0.6470588 0.8549020 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                                <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}">
                                    <Class name="BlendTree" field="element" version="1" type="{A8B5BB1E-5BA9-4B0A-88E9-21BB7A199ED2}">
                                        <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                                            <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                                            <Class name="AZ::u64" field="id" value="12099635065983060864" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                            <Class name="AZStd::string" field="name" value="BlendTree0" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                            <Class name="int" field="posX" value="530" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                            <Class name="int" field="posY" value="360" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                            <Class name="Color" field="visualizeColor" value="0.6784314 0.8705883 1.0000000 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                                            <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                            <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                            <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                            <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}">
                                                <Class name="BlendTreeFinalNode" field="element" version="1" type="{1A755218-AD9D-48EA-86FC-D571C11ECA4D}">
                                                    <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                                                        <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                                                        <Class name="AZ::u64" field="id" value="6320198947116385307" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                        <Class name="AZStd::string" field="name" value="FinalNode0" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                        <Class name="int" field="posX" value="-39" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                        <Class name="int" field="posY" value="10" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                        <Class name="Color" field="visualizeColor" value="0.8235295 0.9803922 0.9803922 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                                                        <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                        <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                        <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                        <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}"/>
                                                        <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}">
                                                            <Class name="BlendTreeConnection" field="element" version="2" type="{B48FFEDB-87FB-4085-AE54-0302AC49373A}">
                                                                <Class name="AZ::u64" field="id" value="15639148820508181089" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                                <Class name="AZ::u64" field="sourceNodeId" value="5538230721523229091" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                                <Class name="unsigned short" field="sourcePortNr" value="0" type="{ECA0B403-C4F8-4B86-95FC-81688D046E40}"/>
                                                                <Class name="unsigned short" field="targetPortNr" value="0" type="{ECA0B403-C4F8-4B86-95FC-81688D046E40}"/>
                                                            </Class>
                                                        </Class>
                                                        <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                                            <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                                                        </Class>
                                                    </Class>
                                                </Class>
                                                <Class name="BlendSpace2DNode" field="element" version="1" type="{5C0DADA2-FE74-468F-A755-55AEBE579C45}">
                                                    <Class name="BlendSpaceNode" field="BaseClass1" version="2" type="{11EC99C4-6A25-4610-86FD-B01F2E53007E}">
                                                        <Class name="AnimGraphNode" field="BaseClass1" version="2" type="{7F1C0E1D-4D32-4A6D-963C-20193EA28F95}">
                                                            <Class name="AnimGraphObject" field="BaseClass1" version="1" type="{532F5328-9AE3-4793-A7AA-8DEB0BAC9A9E}"/>
                                                            <Class name="AZ::u64" field="id" value="5538230721523229091" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                                            <Class name="AZStd::string" field="name" value="BlendSpace2D0" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="int" field="posX" value="-380" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                            <Class name="int" field="posY" value="0" type="{72039442-EB38-4D42-A1AD-CB68F7E0EEF6}"/>
                                                            <Class name="Color" field="visualizeColor" value="0.9803922 1.0000000 0.9607844 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
                                                            <Class name="bool" field="isDisabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                            <Class name="bool" field="isCollapsed" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                            <Class name="bool" field="isVisEnabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                            <Class name="AZStd::vector" field="childNodes" type="{E952D946-FF2C-5624-B72A-E02718B8CB09}"/>
                                                            <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}"/>
                                                            <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                                                <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                                                            </Class>
                                                        </Class>
                                                        <Class name="bool" field="retarget" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                        <Class name="bool" field="inPlace" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                                                    </Class>
                                                    <Class name="unsigned char" field="calculationMethodX" value="0" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    <Class name="AZ::Uuid" field="evaluatorTypeX" value="{17D8679E-5760-481C-9411-5A97D22BC745}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                                                    <Class name="unsigned char" field="calculationMethodY" value="0" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    <Class name="AZ::Uuid" field="evaluatorTypeY" value="{17D8679E-5760-481C-9411-5A97D22BC745}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
                                                    <Class name="unsigned char" field="syncMode" value="1" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    <Class name="AZStd::string" field="syncMasterMotionId" value="rin_walk_kick_03" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                    <Class name="unsigned char" field="eventFilterMode" value="0" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                    <Class name="AZStd::vector" field="motions" type="{2C5B7168-99DB-5908-8EDB-029D3793CA0A}">
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_readyattack_idle" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_run" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_walk_kick_03" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_stand_kick_punch_04" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_shuffle_turn_left" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>)"
            R"(                                             <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_stand_kick_punch_03" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_walk_kick_04" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_turn_180_clockwise" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                        <Class name="BlendSpaceMotion" field="element" version="1" type="{4D624F75-2179-47E4-80EE-6E5E8B9B2CA0}">
                                                            <Class name="AZStd::string" field="motionId" value="rin_stand_kick_punch_02" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
                                                            <Class name="Vector2" field="coordinates" value="0.0000000 0.0000000" type="{3D80F623-C85C-4741-90D0-E4E66164E6BF}"/>
                                                            <Class name="unsigned char" field="typeFlags" value="4" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
                                                        </Class>
                                                    </Class>
                                                </Class>
                                            </Class>
                                            <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}"/>
                                            <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                                <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                                            </Class>
                                        </Class>
                                        <Class name="AZ::u64" field="finalNodeId" value="6320198947116385307" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                                    </Class>
                                </Class>
                                <Class name="AZStd::vector" field="connections" type="{79810215-6320-5ED4-BA2C-CD3D5F2BFFE4}"/>
                                <Class name="TriggerActionSetup" field="actionSetup" version="1" type="{7B4E270C-2C7F-41C4-BFA5-FE6104B789BF}">
                                    <Class name="AZStd::vector" field="actions" type="{51C7563D-C7C7-5975-9359-21A4869DE695}"/>
                                </Class>
                            </Class>
                            <Class name="AZ::u64" field="entryStateId" value="12099635065983060864" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
                            <Class name="AZStd::vector" field="transitions" type="{7BE4AF47-DDD5-5B41-9BE0-6739C8A2694E}"/>
                            <Class name="bool" field="alwaysStartInEntryState" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                        </Class>
                        <Class name="AZStd::vector" field="nodeGroups" type="{9A7148E6-1DCA-5BDA-80D7-5FF35D43170E}"/>
                        <Class name="AnimGraphGameControllerSettings" field="gameControllerSettings" version="1" type="{05DF1B3B-2073-4E6D-B5B6-7B87F46CCCB7}">
                            <Class name="unsigned int" field="activePresetIndex" value="4294967295" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
                            <Class name="AZStd::vector" field="presets" type="{7EF271EC-720F-5899-B522-39F46AEC02F1}"/>
                        </Class>
                        <Class name="bool" field="retarget" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
                    </Class>
                </ObjectStream>
        )";

        // Verify if the property got converted correctly.
        AnimGraph* graph = AZ::Utils::LoadObjectFromBuffer<AnimGraph>(buffer.c_str(), buffer.size(), GetSerializeContext());
        ASSERT_TRUE(graph != nullptr);
        AnimGraphNode* node = graph->RecursiveFindNodeByName("BlendSpace2D0");
        ASSERT_TRUE(node != nullptr);
        BlendSpace2DNode* blendSpaceNode = azrtti_cast<BlendSpace2DNode*>(node);
        ASSERT_TRUE(blendSpaceNode != nullptr);
        EXPECT_STREQ(blendSpaceNode->GetSyncLeaderMotionId().c_str(), "rin_walk_kick_03");
        delete graph;
    }
} // end namespace EMotionFX
