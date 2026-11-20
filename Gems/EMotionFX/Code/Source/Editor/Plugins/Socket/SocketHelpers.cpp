/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/SocketCommands.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SocketSetup.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/DockWidgetPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/PluginManager.h>
#include <Editor/Plugins/Socket/SocketHelpers.h>
#include <Editor/Plugins/Socket/SocketWidget.h>
#include <Editor/SkeletonModel.h>
#include <QRect>

namespace EMotionFX
{
    namespace SocketHelpers
    {
        void OpenSocketPanel()
        {
            EMStudio::PluginManager* pluginManager = EMStudio::GetPluginManager();
            if (pluginManager->FindActivePlugin(SocketWidget::CLASS_ID))
            {
                return;
            }

            EMStudio::EMStudioPlugin* plugin = pluginManager->CreateWindowOfType(SocketWidget::PluginName);
            if (plugin && plugin->GetPluginType() == EMStudio::EMStudioPlugin::PLUGINTYPE_WINDOW)
            {
                EMStudio::DockWidgetPlugin* dockPlugin = static_cast<EMStudio::DockWidgetPlugin*>(plugin);
                EMStudio::MainWindow* mainWindow = EMStudio::GetMainWindow();
                QRect dockRect;
                dockRect.setSize(dockPlugin->GetInitialWindowSize());
                dockRect.moveCenter(mainWindow->geometry().center());
                mainWindow->GetFancyDockingManager()->makeDockWidgetFloating(dockPlugin->GetDockWidget(), dockRect);
            }
        }

        bool JointHasSockets(const Actor& actor, const Node& joint)
        {
            for (const ActorSocket& socket : actor.GetSocketSetup()->GetSockets())
            {
                if (AzFramework::StringFunc::Equal(socket.GetJointName().c_str(), joint.GetName(), /*bCaseSensitive=*/false))
                {
                    return true;
                }
            }
            return false;
        }

        bool AddSocketsToJoints(const QModelIndexList& selectedRowIndices)
        {
            if (selectedRowIndices.empty())
            {
                return false;
            }

            OpenSocketPanel();

            // All the selected rows belong to the same actor.
            const Actor* actor = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            if (!actor)
            {
                return false;
            }

            MCore::CommandGroup commandGroup(AZStd::string::format("Add socket%s", selectedRowIndices.size() > 1 ? "s" : ""));
            for (const QModelIndex& index : selectedRowIndices)
            {
                const Node* joint = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();
                if (joint)
                {
                    CommandSocketHelpers::AddSocket(
                        actor->GetID(), AZStd::string(), joint->GetNameString(), AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(),
                        &commandGroup);
                }
            }

            AZStd::string result;
            if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
                return false;
            }
            return true;
        }

        bool RemoveSocketsFromJoints(const QModelIndexList& selectedRowIndices)
        {
            if (selectedRowIndices.empty())
            {
                return false;
            }

            const Actor* actor = selectedRowIndices[0].data(SkeletonModel::ROLE_ACTOR_POINTER).value<Actor*>();
            if (!actor)
            {
                return false;
            }

            MCore::CommandGroup commandGroup("Remove sockets");
            for (const ActorSocket& socket : actor->GetSocketSetup()->GetSockets())
            {
                for (const QModelIndex& index : selectedRowIndices)
                {
                    const Node* joint = index.data(SkeletonModel::ROLE_POINTER).value<Node*>();
                    if (joint && AzFramework::StringFunc::Equal(socket.GetJointName().c_str(), joint->GetName(), /*bCaseSensitive=*/false))
                    {
                        CommandSocketHelpers::RemoveSocket(actor->GetID(), socket.GetName(), &commandGroup);
                        break;
                    }
                }
            }

            if (commandGroup.GetNumCommands() == 0)
            {
                return false;
            }

            AZStd::string result;
            if (!CommandSystem::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
                return false;
            }
            return true;
        }
    } // namespace SocketHelpers
} // namespace EMotionFX
