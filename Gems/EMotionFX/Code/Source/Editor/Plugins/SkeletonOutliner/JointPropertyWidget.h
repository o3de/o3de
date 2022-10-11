/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <Editor/Plugins/Cloth/ClothJointWidget.h>
#include <Editor/Plugins/HitDetection/HitDetectionJointWidget.h>
#include <UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerBus.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <Editor/Plugins/Ragdoll/RagdollNodeWidget.h>
#include <QPushButton>
#endif

namespace EMStudio
{
    class ActorInfo;
    class NodeInfo;
}

namespace EMotionFX
{
    //! A Widget in the Inspector Pane displaying Attributes of selected Nodes in a Skeleton
    class JointPropertyWidget
            : public QWidget
    {
        Q_OBJECT

    public:
        JointPropertyWidget(QWidget* parent = nullptr);
        ~JointPropertyWidget();

    public slots:
        void Reset();
    private:
        AzToolsFramework::ReflectedPropertyEditor* m_propertyWidget{nullptr};
        AZStd::unique_ptr<EMStudio::ActorInfo> m_actorInfo;
        AZStd::unique_ptr<EMStudio::NodeInfo> m_nodeInfo;
    };

} // ns EMotionFX
