/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <EditorFastNoiseGradientComponent.h>
#include <FastNoiseTest.h>


class FastNoiseEditorTestApp : public ::testing::Test
{
};

TEST_F(FastNoiseEditorTestApp, FastNoise_EditorCreateGameEntity)
{
    AZStd::unique_ptr<AZ::Entity> noiseEntity(aznew AZ::Entity("editor_noise_entity"));
    ASSERT_TRUE(noiseEntity != nullptr);

    FastNoiseGem::EditorFastNoiseGradientComponent editor;
    auto* editorBase = static_cast<AzToolsFramework::Components::EditorComponentBase*>(&editor);
    editorBase->BuildGameEntity(noiseEntity.get());

    // the new game entity's FastNoise component should look like the default one
    FastNoiseGem::FastNoiseGradientConfig defaultConfig;
    FastNoiseGem::FastNoiseGradientConfig gameComponentConfig;

    FastNoiseGem::FastNoiseGradientComponent* noiseComp = noiseEntity->FindComponent<FastNoiseGem::FastNoiseGradientComponent>();
    ASSERT_TRUE(noiseComp != nullptr);

    // Change a value in the gameComponentConfig just to verify that it got overwritten instead of simply matching the default.
    gameComponentConfig.m_seed++;
    noiseComp->WriteOutConfig(&gameComponentConfig);
    ASSERT_EQ(defaultConfig, gameComponentConfig);
}

// This uses custom test / benchmark hooks so that we can load LmbrCentral and GradientSignal Gems.
AZ_UNIT_TEST_HOOK(new UnitTest::FastNoiseTestEnvironment, UnitTest::FastNoiseBenchmarkEnvironment);
