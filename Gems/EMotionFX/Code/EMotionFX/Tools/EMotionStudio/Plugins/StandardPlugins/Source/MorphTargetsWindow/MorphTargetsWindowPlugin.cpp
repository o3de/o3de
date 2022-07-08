/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MorphTargetsWindowPlugin.h"
#include <AzCore/Casting/numeric_cast.h>
#include <QPushButton>
#include <QLabel>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"

namespace EMStudio
{
    MorphTargetsWindowPlugin::MorphTargetsWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_dialogStack = nullptr;
        m_currentActorInstance = nullptr;
        m_staticTextWidget = nullptr;

        EMotionFX::ActorInstanceNotificationBus::Handler::BusConnect();
    }

    MorphTargetsWindowPlugin::~MorphTargetsWindowPlugin()
    {
        EMotionFX::ActorInstanceNotificationBus::Handler::BusDisconnect();

        // unregister the command callbacks and get rid of the memory
        for (auto callback : m_callbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, true);
        }
        m_callbacks.clear();

        Clear();

        // delete the dialog stack
        delete m_dialogStack;
    }

    // init after the parent dock window has been created
    bool MorphTargetsWindowPlugin::Init()
    {
        // create the static text layout
        m_staticTextWidget = new QWidget();
        m_staticTextLayout = new QVBoxLayout();
        m_staticTextWidget->setLayout(m_staticTextLayout);
        QLabel* label = new QLabel("No morph targets to show.");
        m_staticTextLayout->addWidget(label);
        m_staticTextLayout->setAlignment(label, Qt::AlignCenter);

        // create the dialog stack
        assert(m_dialogStack == nullptr);
        m_dialogStack = new MysticQt::DialogStack();
        m_dock->setMinimumWidth(300);
        m_dock->setMinimumHeight(100);
        m_dock->setWidget(m_staticTextWidget);

        GetCommandManager()->RegisterCommandCallback<CommandSelectCallback>("Select", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandUnselectCallback>("Unselect", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandClearSelectionCallback>("ClearSelection", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandAdjustMorphTargetCallback>("AdjustMorphTarget", m_callbacks, false);
        GetCommandManager()->RegisterCommandCallback<CommandAdjustActorInstanceCallback>("AdjustActorInstance", m_callbacks, false);

        // reinit the dialog
        ReInit();

        // connect the window activation signal to refresh if reactivated
        connect(m_dock, &QDockWidget::visibilityChanged, this, &MorphTargetsWindowPlugin::WindowReInit);

        // done
        return true;
    }


    // clear the morph target window
    void MorphTargetsWindowPlugin::Clear()
    {
        if (m_dock)
        {
            m_dock->setWidget(m_staticTextWidget);
        }

        // clear the dialog stack
        if (m_dialogStack)
        {
            m_dialogStack->Clear();
        }

        m_morphTargetGroups.clear();
    }

    // reinit the morph target dialog, e.g. if selection changes
    void MorphTargetsWindowPlugin::ReInit(bool forceReInit)
    {
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        EMotionFX::ActorInstance* actorInstance = selection.GetSingleActorInstance();
        ReInit(actorInstance, forceReInit);
    }

    void MorphTargetsWindowPlugin::ReInit(EMotionFX::ActorInstance* actorInstance, bool forceReInit)
    {
        // show hint if no/multiple actor instances is/are selected
        if (actorInstance == nullptr)
        {
            // set the dock contents
            m_dock->setWidget(m_staticTextWidget);

            // clear dialog and reset the current actor instance as we cleared the window
            if (m_currentActorInstance)
            {
                Clear();
                m_currentActorInstance = nullptr;
            }

            // done
            return;
        }

        // only reinit the morph targets if actor instance changed
        if (m_currentActorInstance != actorInstance || forceReInit)
        {
            // set the current actor instance in any case
            m_currentActorInstance = actorInstance;

            // arrays for the default morph targets and the phonemes
            AZStd::vector<EMotionFX::MorphTarget*>                       phonemes;
            AZStd::vector<EMotionFX::MorphTarget*>                       defaultMorphTargets;
            AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>   phonemeInstances;
            AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>   defaultMorphTargetInstances;

            EMotionFX::Actor* actor = actorInstance->GetActor();
            EMotionFX::MorphSetup* morphSetup = actor->GetMorphSetup(actorInstance->GetLODLevel());
            if (morphSetup == nullptr)
            {
                m_dock->setWidget(m_staticTextWidget);
                return;
            }

            // get the corresponding morph setup instance from the actor instance, this holds the weight values
            EMotionFX::MorphSetupInstance* morphSetupInstance = actorInstance->GetMorphSetupInstance();
            if (morphSetupInstance == nullptr)
            {
                m_dock->setWidget(m_staticTextWidget);
                return;
            }

            // get the number of morph targets
            const size_t numMorphTargets = aznumeric_caster(morphSetup->GetNumMorphTargets());

            // preinit array size
            phonemes.reserve(numMorphTargets);
            phonemeInstances.reserve(numMorphTargets);
            defaultMorphTargets.reserve(numMorphTargets);
            defaultMorphTargetInstances.reserve(numMorphTargets);

            // iterate trough all morph targets
            for (size_t i = 0; i < numMorphTargets; ++i)
            {
                // get the current morph target
                EMotionFX::MorphTarget* morphTarget = morphSetup->GetMorphTarget(aznumeric_caster(i));

                // get the corresponding morph target instance
                // this contains the weight value and some other settings that are unique for the morph target inside this actor instance
                EMotionFX::MorphSetupInstance::MorphTarget* morphTargetInstance = morphSetupInstance->FindMorphTargetByID(morphTarget->GetID());
                if (morphTargetInstance == nullptr)
                {
                    AZ_Error("EMotionFX", false, "No corresponding morph target instance found for morph target '%s'.", morphTarget->GetName());
                    continue;
                }

                // don't add phoneme morph targets (they are used for lipsync)
                if (morphTarget->GetIsPhoneme())
                {
                    phonemes.push_back(morphTarget);
                    phonemeInstances.push_back(morphTargetInstance);
                }
                else
                {
                    defaultMorphTargets.push_back(morphTarget);
                    defaultMorphTargetInstances.push_back(morphTargetInstance);
                }
            }

            // clear the window in case we have already had any widgets in it
            Clear();

            // create group for the default morph targets
            if (!defaultMorphTargets.empty())
            {
                CreateGroup("Default", defaultMorphTargets, defaultMorphTargetInstances);
            }

            // create group for the phonemes
            if (!phonemes.empty())
            {
                CreateGroup("Phonemes", phonemes, phonemeInstances);
            }

            // create static text if no morph targets are available
            if (defaultMorphTargets.empty() && phonemes.empty())
            {
                m_dock->setWidget(m_staticTextWidget);
            }
            else
            {
                m_dock->setWidget(m_dialogStack);
            }

            // adjust the slider values to the correct weights of the selected actor instance
            UpdateInterface();
        }
    }


    void MorphTargetsWindowPlugin::CreateGroup(const char* name, const AZStd::vector<EMotionFX::MorphTarget*>& morphTargets, const AZStd::vector<EMotionFX::MorphSetupInstance::MorphTarget*>& morphTargetInstances)
    {
        if (morphTargets.empty() || morphTargetInstances.empty())
        {
            return;
        }

        MorphTargetGroupWidget* morphTargetGroup = new MorphTargetGroupWidget(name, m_currentActorInstance, morphTargets, morphTargetInstances, m_dialogStack);
        morphTargetGroup->setObjectName("EMFX.MorphTargetsWindowPlugin.MorphTargetGroupWidget");
        m_morphTargetGroups.push_back(morphTargetGroup);

        m_dialogStack->Add(morphTargetGroup, name);
    }


    // reinit the window when it gets activated
    void MorphTargetsWindowPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            ReInit(true);
        }
    }


    // update the interface
    void MorphTargetsWindowPlugin::UpdateInterface()
    {
        for (MorphTargetGroupWidget* group : m_morphTargetGroups)
        {
            group->UpdateInterface();
        }
    }


    // update the morph target
    void MorphTargetsWindowPlugin::UpdateMorphTarget(const char* name)
    {
        for (MorphTargetGroupWidget* group : m_morphTargetGroups)
        {
            group->UpdateMorphTarget(name);
        }
    }

    void MorphTargetsWindowPlugin::OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance)
    {
        if (m_currentActorInstance == actorInstance)
        {
            ReInit(/*actorInstance=*/nullptr);
        }
    }

    //-----------------------------------------------------------------------------------------
    // Command callbacks
    //-----------------------------------------------------------------------------------------


    bool ReInitMorphTargetsWindowPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MorphTargetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MorphTargetsWindowPlugin* morphTargetsWindow = (MorphTargetsWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (GetManager()->GetIgnoreVisibility() || !morphTargetsWindow->GetDockWidget()->visibleRegion().isEmpty())
        {
            morphTargetsWindow->ReInit(true);
        }

        return true;
    }


    bool UpdateMorphTargetsWindowPluginInterface(const char* name)
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MorphTargetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MorphTargetsWindowPlugin* morphTargetsWindow = (MorphTargetsWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (GetManager()->GetIgnoreVisibility() || !morphTargetsWindow->GetDockWidget()->visibleRegion().isEmpty())
        {
            morphTargetsWindow->UpdateMorphTarget(name);
        }

        return true;
    }


    bool MorphTargetsWindowPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return ReInitMorphTargetsWindowPlugin();
    }
    bool MorphTargetsWindowPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMorphTargetsWindowPlugin(); }
    bool MorphTargetsWindowPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)          { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMorphTargetsWindowPlugin(); }


    bool MorphTargetsWindowPlugin::CommandAdjustMorphTargetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string name;
        commandLine.GetValue("name", command, &name);
        return UpdateMorphTargetsWindowPluginInterface(name.c_str());
    }


    bool MorphTargetsWindowPlugin::CommandAdjustMorphTargetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        AZStd::string name;
        commandLine.GetValue("name", command, &name);
        return UpdateMorphTargetsWindowPluginInterface(name.c_str());
    }


    bool MorphTargetsWindowPlugin::CommandAdjustActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("lodLevel"))
        {
            return ReInitMorphTargetsWindowPlugin();
        }

        return true;
    }


    bool MorphTargetsWindowPlugin::CommandAdjustActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (commandLine.CheckIfHasParameter("lodLevel"))
        {
            return ReInitMorphTargetsWindowPlugin();
        }

        return true;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MorphTargetsWindow/moc_MorphTargetsWindowPlugin.cpp>
