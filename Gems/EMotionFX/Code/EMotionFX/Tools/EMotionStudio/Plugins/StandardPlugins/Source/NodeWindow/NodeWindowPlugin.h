/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <EMotionFX/Source/ActorBus.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/StandardPluginsConfig.h>
#include <MysticQt/Source/DialogStack.h>
#endif

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace EMStudio
{
    // forward declaration
    class ActorInfo;
    class NodeHierarchyWidget;
    class NodeInfo;

    class NodeWindowPlugin
        : public EMStudio::DockWidgetPlugin
        , EMotionFX::ActorNotificationBus::Handler
    {
        Q_OBJECT // AUTOMOC
        MCORE_MEMORYOBJECTCATEGORY(NodeWindowPlugin, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        enum
        {
            CLASS_ID = 0x00000357
        };

        NodeWindowPlugin();
        ~NodeWindowPlugin();

        // overloaded
        const char* GetCompileDate() const override         { return MCORE_DATE; }
        const char* GetName() const override                { return "Joint outliner"; }
        uint32 GetClassID() const override                  { return CLASS_ID; }
        const char* GetCreatorName() const override         { return "O3DE"; }
        float GetVersion() const override                   { return 1.0f;  }
        bool GetIsClosable() const override                 { return true;  }
        bool GetIsFloatable() const override                { return true;  }
        bool GetIsVertical() const override                 { return false; }

        // overloaded main init function
        void Reflect(AZ::ReflectContext* context) override;
        bool Init() override;
        EMStudioPlugin* Clone() override;
        void ReInit();

        void ProcessFrame(float timePassedInSeconds) override;

    public slots:
        void OnNodeChanged();
        void VisibilityChanged(bool isVisible);
        void OnTextFilterChanged(const QString& text);

        void UpdateVisibleNodeIndices();

    private:
        // ActorNotificationBus
        void OnActorReady(EMotionFX::Actor* actor) override;

        // declare the callbacks
        MCORE_DEFINECOMMANDCALLBACK(UpdateCallback);

        AZStd::vector<UpdateCallback*> m_callbacks;

        MysticQt::DialogStack*              m_dialogStack;
        NodeHierarchyWidget*                m_hierarchyWidget;
        AzToolsFramework::ReflectedPropertyEditor* m_propertyWidget;

        AZStd::string                       m_string;
        AZStd::string                       m_tempGroupName;
        AZStd::unordered_set<size_t> m_visibleNodeIndices;
        AZStd::unordered_set<size_t> m_selectedNodeIndices;

        AZStd::unique_ptr<ActorInfo>        m_actorInfo;
        AZStd::unique_ptr<NodeInfo>         m_nodeInfo;

        // Use this flag to defer the reinit function to main thread.
        bool m_reinitRequested = false;
    };
} // namespace EMStudio
