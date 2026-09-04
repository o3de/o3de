/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzTest/AzTest.h>

#include <Maestro/IMovieSystem.h>
#include <Maestro/Types/AnimNodeType.h>
#include <Maestro/Types/AnimParamType.h>
#include <Maestro/Types/AnimValueType.h>
#include <Maestro/Types/SequenceType.h>

// Maestro compatibility headers
#include <IMovieSystem.h>
#include <CryCommon/Maestro/Bus/EditorSequenceAgentComponentBus.h>
#include <CryCommon/Maestro/Bus/EditorSequenceBus.h>
#include <CryCommon/Maestro/Bus/EditorSequenceComponentBus.h>
#include <CryCommon/Maestro/Bus/SequenceAgentComponentBus.h>
#include <CryCommon/Maestro/Bus/SequenceComponentBus.h>
#include <CryCommon/Maestro/Types/AnimNodeType.h>
#include <CryCommon/Maestro/Types/AnimParamType.h>
#include <CryCommon/Maestro/Types/AnimValueType.h>
#include <CryCommon/Maestro/Types/AssetBlendKey.h>
#include <CryCommon/Maestro/Types/AssetBlends.h>
#include <CryCommon/Maestro/Types/SequenceType.h>

TEST(MaestroApiCompatibilityTests, OutOfLineAnimParamTypeSymbolsLinkFromApiTarget)
{
    const CAnimParamType position(AnimParamType::Position);
    const CAnimParamType positionCopy(AnimParamType::Position);
    const CAnimParamType namedParameter(AZStd::string("API compatibility test"));

    EXPECT_EQ(position, positionCopy);
    EXPECT_NE(position, namedParameter);
    EXPECT_STREQ(namedParameter.GetName(), "API compatibility test");
}

TEST(MaestroApiCompatibilityTests, SerializedValuesAndPublicTypeIdsRemainStable)
{
    static_assert(static_cast<int>(SequenceType::Legacy) == 0);
    static_assert(static_cast<int>(SequenceType::SequenceComponent) == 1);
    static_assert(static_cast<int>(AnimNodeType::Entity) == 0x01);
    static_assert(static_cast<int>(AnimNodeType::Component) == 0x1c);
    static_assert(static_cast<int>(AnimParamType::Position) == 1);
    static_assert(static_cast<int>(AnimParamType::User) == 100000);
    static_assert(static_cast<int>(AnimValueType::AssetBlend) == 22);
    static_assert(CAnimParamType::kParamTypeVersion == 9);
    static_assert(IAnimSequence::kSequenceVersion == 5);

    EXPECT_EQ(azrtti_typeid<CAnimParamType>(), AZ::TypeId("{E2F34955-3B07-4241-8D34-EA3BEF3B33D2}"));
    EXPECT_EQ(azrtti_typeid<IAnimTrack>(), AZ::TypeId("{AA0D5170-FB28-426F-BA13-7EFF6BB3AC67}"));
    EXPECT_EQ(azrtti_typeid<IAnimNode>(), AZ::TypeId("{0A096354-7F26-4B18-B8C0-8F10A3E0440A}"));
    EXPECT_EQ(azrtti_typeid<IAnimSequence>(), AZ::TypeId("{A60F95F5-5A4A-47DB-B3BB-525BBC0BC8DB}"));
    EXPECT_EQ(azrtti_typeid<IMovieSystem>(), AZ::TypeId("{D8E6D6E9-830D-40DC-87F3-E9A069FBEB69}"));
}

TEST(MaestroApiCompatibilityTests, PublicOutOfLineReflectionSymbolsRegisterTheirTypes)
{
    AZ::SerializeContext serializeContext;
    IAnimTrack::Reflect(&serializeContext);
    IAnimNode::Reflect(&serializeContext);
    IAnimSequence::Reflect(&serializeContext);

    EXPECT_NE(serializeContext.FindClassData(azrtti_typeid<IAnimTrack>()), nullptr);
    EXPECT_NE(serializeContext.FindClassData(azrtti_typeid<IAnimNode>()), nullptr);
    EXPECT_NE(serializeContext.FindClassData(azrtti_typeid<IAnimSequence>()), nullptr);
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
