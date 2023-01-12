/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>
#include <Editor/Plugins/ColliderWidgets/JointPropertyWidget.h>
#include <Tests/UI/UIFixture.h>
//#include <QTreeView>
#include <Editor/ReselectingTreeView.h>
#include <Tests/Mocks/PhysicsSystem.h>

namespace EMotionFX
{
    class SkeletonOutlinerTestFixture : public UIFixture
    {
    public:
        void CreateSkeletonAndModelIndices()
        {
            // Select the newly created actor
            AZStd::string result;
            EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand("Select -actorID 0", result)) << result.c_str();

            // Get the SkeletonOutlinerPlugin and find its treeview
            EXPECT_TRUE(m_skeletonOutlinerPlugin);
            m_treeView = m_skeletonOutlinerPlugin->GetDockWidget()->findChild<ReselectingTreeView*>(
                "EMFX.SkeletonOutlinerPlugin.SkeletonOutlinerTreeView");

            m_indexList.clear();
            m_treeView->RecursiveGetAllChildren(m_treeView->model()->index(0, 0, m_treeView->model()->index(0, 0)), m_indexList);
        }

        JointPropertyWidget* GetJointPropertyWidget()
        {
            return m_skeletonOutlinerPlugin->m_propertyWidget;
        }

        AddCollidersButton* GetAddCollidersButton()
        {
            auto* button = GetJointPropertyWidget()->findChild<AddCollidersButton*>(
                "EMotionFX.SkeletonOutlinerPlugin.JointPropertyWidget.addCollidersButton");
            return button;
        }

        QTreeView* GetAddCollidersTreeView()
        {
            auto* treeView =
                GetJointPropertyWidget()->findChild<QTreeView*>("EMotionFX.SkeletonOutlinerPlugin.AddCollidersButton.TreeView");
            if (treeView != nullptr)
            {
                return treeView;
            }
            GetAddCollidersButton()->click();
            treeView = GetJointPropertyWidget()->findChild<QTreeView*>("EMotionFX.SkeletonOutlinerPlugin.AddCollidersButton.TreeView");
            EXPECT_TRUE(treeView);
            return treeView;
        }

        // This is useful for hands on testing
        void ShowJointPropertyWidget();
        void AddColliderViaAddComponentButton(QString label, QString subLevelLabel = {""});

        void SetUpPhysics();
        void SetUpSimulatedObject();

    protected:
        QModelIndexList m_indexList;
        ReselectingTreeView* m_treeView;

        Actor* m_actor;
    };

} // namespace EMotionFX
