/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <Editor/ActorJointBrowseEdit.h>
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#endif

QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace EMStudio
{
    // forward declaration
    class SceneManagerPlugin;
    class MirrorSetupWindow;

    class ActorPropertiesWindow
        : public QWidget
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(ActorPropertiesWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000005
        };

        ActorPropertiesWindow(QWidget* parent, SceneManagerPlugin* plugin);
        ~ActorPropertiesWindow() = default;

        void Init();

        // helper functions
        static void GetNodeName(const AZStd::vector<SelectionItem>& joints, AZStd::string* outNodeName, uint32* outActorID);

    public slots:
        void UpdateInterface();

        // actor name
        void NameEditChanged();

        void OnMotionExtractionJointSelected(const AZStd::vector<SelectionItem>& selectedJoints);
        void OnFindBestMatchingNode();
        void OnRetargetRootJointSelected(const AZStd::vector<SelectionItem>& selectedJoints);
        void OnMirrorSetup();

        // nodes excluded from bounding volume calculations
        void OnExcludedJointsFromBoundsSelectionDone(const AZStd::vector<SelectionItem>& selectedJoints);
        void OnExcludedJointsFromBoundsSelectionChanged(const AZStd::vector<SelectionItem>& selectedJoints);

    private:
        ActorJointBrowseEdit* m_motionExtractionJointBrowseEdit = nullptr;
        QPushButton* m_findBestMatchButton = nullptr;

        ActorJointBrowseEdit* m_retargetRootJointBrowseEdit = nullptr;
        ActorJointBrowseEdit* m_excludeFromBoundsBrowseEdit = nullptr;

        AzQtComponents::BrowseEdit*     m_mirrorSetupLink = nullptr;
        MirrorSetupWindow*              m_mirrorSetupWindow = nullptr;

        // actor name
        QLineEdit*                      m_nameEdit = nullptr;

        SceneManagerPlugin*             m_plugin = nullptr;
        EMotionFX::Actor*               m_actor = nullptr;
        EMotionFX::ActorInstance*       m_actorInstance = nullptr;
    };
} // namespace EMStudio
