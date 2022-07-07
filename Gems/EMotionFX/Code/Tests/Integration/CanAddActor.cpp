/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>

#include <QPushButton>
#include <QAction>
#include <QtTest>

#include <Tests/UI/UIFixture.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>

namespace EMotionFX
{
    TEST_F(UIFixture, CanAddActor)
    {
        /*
        Test Case: C14459474
        Can Add Actor
        Imports an Actor to ensure that importing an actor is poosible.
        */

        RecordProperty("test_case_id", "C14459474");

        // Ensure that the number of actors loaded is 0
        ASSERT_EQ(GetActorManager().GetNumActors(), 0);

        // Load an Actor
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<JackNoMeshesActor>(actorAssetId, "Jack");

        // Ensure the Actor is correct
        ASSERT_TRUE(GetActorManager().FindActorByName("Jack"));
        EXPECT_EQ(GetActorManager().GetNumActors(), 1);
    }
} // namespace EMotionFX
