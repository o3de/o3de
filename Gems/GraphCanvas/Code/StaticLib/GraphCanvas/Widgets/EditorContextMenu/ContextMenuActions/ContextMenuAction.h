/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
// qfontmetrics.h(118): warning C4251: 'QFontMetrics::d': class 'QExplicitlySharedDataPointer<QFontPrivate>' needs to have dll-interface to be used by clients of class 'QFontMetrics'
// qwidget.h(858) : warning C4800 : 'uint' : forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QAction>
#include <QObject>
AZ_POP_DISABLE_WARNING

#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Editor/EditorTypes.h>
#endif

namespace GraphCanvas
{
    typedef AZ::Crc32 ActionGroupId;
    
    class ContextMenuAction
        : public QAction
    {
        Q_OBJECT
    protected:
        ContextMenuAction(AZStd::string_view actionName, QObject* parent);
        
    public:
        // This enum dertermines what actions the scene should take once this action is triggered.
        enum class SceneReaction
        {
            Unknown,
            PostUndo,
            Nothing
        };
        
        virtual ~ContextMenuAction() = default;
        
        virtual ActionGroupId GetActionGroupId() const = 0;

        void SetTarget(const GraphId& graphId, const AZ::EntityId& targetId);

        virtual bool IsInSubMenu() const;
        virtual AZStd::string GetSubMenuPath() const;

        virtual SceneReaction TriggerAction(const AZ::Vector2& scenePos)
        {
            return TriggerAction(m_graphId, scenePos);
        }

        // Should trigger the selected action, and return the appropriate reaction for the scene to take.
        virtual SceneReaction TriggerAction(const GraphId& graphId, const AZ::Vector2& scenePos)
        {
            // Temporary fix for the weird recursive step made by deprecating this workflow.
            if (!m_recursionFix)
            {
                m_recursionFix = true;
                m_graphId = graphId;
                SceneReaction reaction = TriggerAction(scenePos);
                m_recursionFix = false;

                return reaction;
            }

            return SceneReaction::Nothing;
        }

    protected:

        const AZ::EntityId& GetTargetId() const;
        const GraphId& GetGraphId() const;
        EditorId GetEditorId() const;

        virtual void RefreshAction();

        // Deprecated function I don't want to actually deprecate just yet
        virtual void RefreshAction(const GraphId& graphId, const AZ::EntityId& targetId);

    private:

        AZ::EntityId m_targetId;
        GraphId      m_graphId;

        bool m_recursionFix = false;
    };
}
