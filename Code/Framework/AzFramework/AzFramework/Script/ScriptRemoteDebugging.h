/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef SCRIPT_REMOTE_DEBUGGING_H
#define SCRIPT_REMOTE_DEBUGGING_H

#include <AzCore/Component/Component.h>
#include <AzCore/Script/ScriptContextDebug.h>

/*
 * Remote script debugging is accomplished through the ScriptDebugAgent, which
 * sits on the target running the VM and communicates with the remote debugger
 * through the IRemoteTools interface.
 *
 * To communicate with the agent, send TM messages to AZ_CRC_CE("ScriptDebugAgent").
 * The agent will respond by sending TM messages to AZ_CRC_CE("ScriptDebugger").
 * See the comments in TargetManagementAPI.h" for information on how to send TM
 * messages.
 *
 * Valid commands and agent's responses:
 *  Commands that can be sent at any time:
 *      AZ_CRC_CE("AttachDebugger")
 *          AZ_CRC_CE("DebuggerAttached")
 *          AZ_CRC_CE("DebuggerRefused")
 *      AZ_CRC_CE("EnumContexts")
 *          AZ_CRC(EnumContextsResult")
 *  Commands that can be sent when attached:
 *      AZ_CRC_CE("DetachDebugger")
 *          AZ_CRC_CE("DebuggerDetached") *** can also be sent to current debugger if another debugger is attaching
 *      AZ_CRC_CE("AddBreakpoint")
 *          AZ_CRC_CE("BreakpointAdded")
 *          AZ_CRC_CE("BreakpointHit") *** sent when a breakpoint hits
 *      AZ_CRC_CE("RemoveBreakpoint")
 *          AZ_CRC_CE("BreakpointRemoved")
 *      AZ_CRC_CE("EnumRegisteredGlobals")
 *          AZ_CRC_CE("EnumRegisteredGlobalsResult")
 *      AZ_CRC_CE("EnumRegisteredClasses")
 *          AZ_CRC_CE("EnumRegisteredClassesResult")
 *      AZ_CRC_CE("GetValue")
 *          AZ_CRC_CE("GetValueResult")
 *  Commands that can only be sent while NOT on a breakpoint
 *      AZ_CRC_CE("ExecuteScript")
 *          AZ_CRC_CE("ExecutionCompleted")
 *  Commands that can only be sent while sitting on a breakpoint
 *      AZ_CRC_CE("GetCallstack")
 *          AZ_CRC_CE("CallstackResult")
 *      AZ_CRC_CE("EnumLocals")
 *          AZ_CRC_CE("EnumLocalsResult")
 *      AZ_CRC_CE("SetValue")
 *          AZ_CRC_CE("SetValueResult")
 *      AZ_CRC_CE("StepOver")
 *          AZ_CRC_CE("Ack")
 *      AZ_CRC_CE("StepIn")
 *          AZ_CRC_CE("Ack")
 *      AZ_CRC_CE("StepOut")
 *          AZ_CRC_CE("Ack")
 *      AZ_CRC_CE("Continue")
 *          AZ_CRC_CE("Ack")
 */

namespace AzFramework
{
    struct ScriptUserMethodInfo
    {
        AZ_TYPE_INFO(ScriptUserMethodInfo, "{32fe4b43-2c23-4ab4-9374-3d13cf050002}");
        AZStd::string   m_name;
        AZStd::string   m_dbgParamInfo;
    };

    typedef AZStd::vector<ScriptUserMethodInfo> ScriptUserMethodList;

    struct ScriptUserPropertyInfo
    {
        AZ_TYPE_INFO(ScriptUserPropertyInfo, "{6cd9f5be-b2cd-41bb-9da5-1b053548cf56}");
        AZStd::string   m_name;
        bool            m_isRead;
        bool            m_isWrite;
    };

    typedef AZStd::vector<ScriptUserPropertyInfo> ScriptUserPropertyList;

    struct ScriptUserClassInfo
    {
        AZ_TYPE_INFO(ScriptUserClassInfo, "{08b32f99-2ea2-4abe-a05f-1aa32ef44b15}");
        AZStd::string           m_name;
        AZ::Uuid                m_typeId;
        ScriptUserMethodList    m_methods;
        ScriptUserPropertyList  m_properties;
    };

    typedef AZStd::vector<ScriptUserClassInfo> ScriptUserClassList;

    struct ScriptUserEBusMethodInfo : public ScriptUserMethodInfo
    {
        AZ_TYPE_INFO(ScriptUserEBusMethodInfo, "{FD805F6C-8612-41CF-85FE-3B97683C98F2}");
        AZStd::string m_category;
    };

    typedef AZStd::vector<ScriptUserEBusMethodInfo> ScriptUserEBusMethodList;

    struct ScriptUserEBusInfo
    {
        AZ_TYPE_INFO(ScriptUserEBusInfo, "{2376407e-1621-4d7f-b4ad-de04a81a2616}");
        AZStd::string               m_name;
        ScriptUserEBusMethodList    m_events;
        bool                        m_canBroadcast;
        bool                        m_canQueue;
        bool                        m_hasHandler;
    };

    typedef AZStd::vector<ScriptUserEBusInfo> ScriptUserEBusList;

    AZ::ComponentDescriptor* CreateScriptDebugAgentFactory();
}   // namespace AzFramework

#endif  // SCRIPT_REMOTE_DEBUGGING_H
#pragma once
