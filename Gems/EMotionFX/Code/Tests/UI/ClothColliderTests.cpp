/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Editor/ColliderHelpers.h"
#include <gtest/gtest.h>

#include <QtTest>
#include <QTreeView>

#include <Editor/Plugins/ColliderWidgets/ClothOutlinerNotificationHandler.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/ReselectingTreeView.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/UI/UIFixture.h>

namespace EMotionFX
{
    // Add so that ClothJointInspectorPlugin::IsNvClothGemAvailable() will return the correct value
    class SystemComponent
        : public AZ::Component
        //        , public SystemRequestBus::Handler
        //        , protected CrySystemEventBus::Handler
    {
    public:
        AZ_COMPONENT(SystemComponent, "{89DF5C48-64AC-4B8E-9E61-0D4C7A7B5491}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SystemComponent, AZ::Component>()
                    ->Version(0);
            }
        }

    protected:
        void Activate() override {}
        void Deactivate() override {}
    };


    class ClothColliderTestsFixture : public UIFixture
    {
    public:
        void TearDown() override
        {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
            UIFixture::TearDown();
        }

        void CreateSkeletonAndModelIndices()
        {
            // Select the newly created actor
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorID 0", result)) << result.c_str();

            // Change the Editor mode to Physics
            EMStudio::GetMainWindow()->ApplicationModeChanged("Physics");

            // Get the SkeletonOutlinerPlugin and find its treeview
            m_skeletonOutliner = static_cast<EMotionFX::SkeletonOutlinerPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SkeletonOutlinerPlugin::CLASS_ID));
            EXPECT_TRUE(m_skeletonOutliner);
            m_treeView = m_skeletonOutliner->GetDockWidget()->findChild<ReselectingTreeView*>("EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");

            m_indexList.clear();
            m_treeView->RecursiveGetAllChildren(m_treeView->model()->index(0, 0, m_treeView->model()->index(0, 0)), m_indexList);
        }

    protected:
        bool ShouldReflectPhysicSystem() override { return true; }

        void ReflectMockedSystems() override
        {
            UIFixture::ReflectMockedSystems();

            // Reflect the mocked version of the cloth system.
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            SystemComponent::Reflect(serializeContext);
        }

        QModelIndexList m_indexList;
        ReselectingTreeView* m_treeView;
        EMotionFX::SkeletonOutlinerPlugin* m_skeletonOutliner;

    };

    TEST_F(ClothColliderTestsFixture, RemoveClothColliders)
    {
        const int numJoints = 8;
        const int firstIndex = 3;
        const int lastIndex = 6;
        RecordProperty("test_case_id", "C18970351");

        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZ::Data::Asset<Integration::ActorAsset> actorAsset =
            TestActorAssets::CreateActorAssetAndRegister<SimpleJointChainActor>(actorAssetId, numJoints, "RagdollEditTestsActor");

        CreateSkeletonAndModelIndices();
        EXPECT_EQ(m_indexList.size(), numJoints);

        // Select four Indexes
        SelectIndexes(m_indexList, m_treeView, firstIndex, lastIndex);

        // Bring up the contextMenu
        const QRect rect = m_treeView->visualRect(m_indexList[5]);
        EXPECT_TRUE(rect.isValid());
        BringUpContextMenu(m_treeView, rect);

        QMenu* contextMenu = m_skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        ASSERT_TRUE(contextMenu);
        QAction* clothAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(clothAction, contextMenu, "Cloth"));
        QMenu* clothMenu = clothAction->menu();
        ASSERT_TRUE(clothMenu);
        QAction* colliderAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(colliderAction, clothMenu, "Add collider"));
        QMenu* colliderMenu = colliderAction->menu();
        ASSERT_TRUE(colliderMenu);
        QAction* addSphereAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(addSphereAction, colliderMenu, "Add sphere"));
        addSphereAction->trigger();

        // Check colliders are in model
        for (int i = firstIndex; i <= lastIndex; ++i)
        {
            EXPECT_TRUE(ColliderHelpers::NodeHasClothCollider(m_indexList[i]));
        }

        // Remove context menu as it is rebuild below
        delete contextMenu;

        const QRect rect2 = m_treeView->visualRect(m_indexList[4]);
        EXPECT_TRUE(rect2.isValid());
        BringUpContextMenu(m_treeView, rect2);

        // Find the "Remove colliders" entry and trigger it.
        contextMenu = m_skeletonOutliner->GetDockWidget()->findChild<QMenu*>("EMFX.SkeletonOutlinerPlugin.ContextMenu");
        ASSERT_TRUE(contextMenu);
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(clothAction, contextMenu, "Cloth"));
        clothMenu = clothAction->menu();
        ASSERT_TRUE(clothMenu);
        QAction* removeClothAction;
        EXPECT_TRUE(UIFixture::GetActionFromContextMenu(removeClothAction, clothMenu, "Remove colliders"));
        removeClothAction->trigger();

        // Check colliders have been removed
        for (int i = firstIndex; i <= lastIndex; ++i)
        {
            EXPECT_FALSE(ColliderHelpers::NodeHasClothCollider(m_indexList[i]));
        }
    }
}
