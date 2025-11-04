/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptRemoteDebugging.h"
#include "ScriptDebugAgentBus.h"
#include "ScriptDebugMsgReflection.h"
#include <AzFramework/Network/IRemoteTools.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>
#include <AzFramework/Script/ScriptRemoteDebuggingConstants.h>
#include <AzFramework/AzFramework_Traits_Platform.h> // Need to know the state of AZ_TRAIT_AZFRAMEWORK_SHOW_MOUSE_ON_LUA_BREAKPOINT
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Script/ScriptSystemBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzNetworking/Framework/INetworking.h>

namespace AzFramework
{
    namespace ScriptDebugAgentInternal
    {
        //-------------------------------------------------------------------------
        static bool EnumClass(const char* name, const AZ::Uuid& typeId, void* userData)
        {
            ScriptUserClassList& output = *reinterpret_cast<ScriptUserClassList*>(userData);
            output.emplace_back().m_name = name;
            output.back().m_typeId = typeId;
            return true;
        }
        //-------------------------------------------------------------------------
        static bool EnumMethod(const AZ::Uuid* classTypeId, const char* name, const char* dbgParamInfo, void* userData)
        {
            (void)classTypeId;
            ScriptUserMethodList& output = *reinterpret_cast<ScriptUserMethodList*>(userData);
            output.emplace_back().m_name = name;
            output.back().m_dbgParamInfo = dbgParamInfo ? dbgParamInfo : "null";
            return true;
        }
        //-------------------------------------------------------------------------
        static bool EnumProperty(const AZ::Uuid* classTypeId, const char* name, bool isRead, bool isWrite, void* userData)
        {
            (void)classTypeId;
            ScriptUserPropertyList& output = *reinterpret_cast<ScriptUserPropertyList*>(userData);
            output.emplace_back().m_name = name;
            output.back().m_isRead = isRead;
            output.back().m_isWrite = isWrite;
            return true;
        }
        //-------------------------------------------------------------------------
        static bool EnumEBus(const AZStd::string& name, bool canBroadcast, bool canQueue, bool hasHandler, void* userData)
        {
            ScriptUserEBusList& output = *reinterpret_cast<ScriptUserEBusList*>(userData);

            bool found = false;

            for (ScriptUserEBusList::iterator it = output.begin(); it != output.end(); ++it)
            {
                if (name == it->m_name)
                {
                    found = true;
                }
            }

            AZ_Warning("ScriptRemoteDebugging", !found, "Ebus (%s) already enumerated", name.c_str());

            if (!found)
            {
                auto& ebus = output.emplace_back();
                ebus.m_name = name;
                ebus.m_canBroadcast = canBroadcast;
                ebus.m_canQueue = canQueue;
                ebus.m_hasHandler = hasHandler;
            }

            return true;
        }
        //-------------------------------------------------------------------------
        static bool EnumEBusSender(const AZStd::string& ebusName, const AZStd::string& senderName, const AZStd::string& dbgParamInfo, const AZStd::string& category, void* userData)
        {
            ScriptUserEBusList& output = *reinterpret_cast<ScriptUserEBusList*>(userData);

            for (ScriptUserEBusList::iterator it = output.begin(); it != output.end(); ++it)
            {
                if (ebusName == it->m_name)
                {
                    auto& event = it->m_events.emplace_back();
                    event.m_name = senderName;
                    event.m_dbgParamInfo = dbgParamInfo;
                    event.m_category = category;
                    return true;
                }
            }
            AZ_Assert(false, "Received an enumeration of an eBus sender method for an eBus we have not enumerated yet!");
            return false;
        }

        //-------------------------------------------------------------------------
        static bool EnumClassMethod(const AZ::Uuid* classTypeId, const char* name, const char* dbgParamInfo, void* userData)
        {
            ScriptUserClassList& output = *reinterpret_cast<ScriptUserClassList*>(userData);
            for (ScriptUserClassList::iterator it = output.begin(); it != output.end(); ++it)
            {
                if (classTypeId && *classTypeId == it->m_typeId)
                {
                    return EnumMethod(classTypeId, name, dbgParamInfo, &(it->m_methods));
                }
            }
            AZ_Assert(false, "Received enumeration of a class method for a class we have not enumerated yet!");
            return true;
        }
        //-------------------------------------------------------------------------
        static bool EnumClassProperty(const AZ::Uuid* classTypeId, const char* name, bool isRead, bool isWrite, void* userData)
        {
            ScriptUserClassList& output = *reinterpret_cast<ScriptUserClassList*>(userData);
            for (ScriptUserClassList::iterator it = output.begin(); it != output.end(); ++it)
            {
                if (classTypeId && *classTypeId == it->m_typeId)
                {
                    return EnumProperty(classTypeId, name, isRead, isWrite, &(it->m_properties));
                }
            }
            AZ_Assert(false, "Received enumeration of a class property for a class we have not enumerated yet!");
            return true;
        }
        //-------------------------------------------------------------------------
        static bool EnumGlobalMethod(const AZ::Uuid* classTypeId, const char* name, const char* dbgParamInfo, void* userData)
        {
            ScriptDebugRegisteredGlobalsResult* output = reinterpret_cast<ScriptDebugRegisteredGlobalsResult*>(userData);
            return EnumMethod(classTypeId, name, dbgParamInfo, &output->m_methods);
        }
        //-------------------------------------------------------------------------
        static bool EnumGlobalProperty(const AZ::Uuid* classTypeId, const char* name, bool isRead, bool isWrite, void* userData)
        {
            ScriptDebugRegisteredGlobalsResult* output = reinterpret_cast<ScriptDebugRegisteredGlobalsResult*>(userData);
            return EnumProperty(classTypeId, name, isRead, isWrite, &output->m_properties);
        }
        //-------------------------------------------------------------------------
        static bool EnumLocals(AZStd::vector<AZStd::string>* output, const char* name, AZ::ScriptDataContext& dataContext)
        {
            (void)dataContext;
            output->push_back(name);
            return true;
        }
        //-------------------------------------------------------------------------
    }   // namespace ScriptDebugAgentInternal

    //-------------------------------------------------------------------------
    //-------------------------------------------------------------------------
    class ScriptDebugAgent
        : public AZ::Component
        , public ScriptDebugAgentBus::Handler
        , AZ::SystemTickBus::Handler
    {
    public:
        AZ_COMPONENT(ScriptDebugAgent, "{624a7be2-3c7e-4119-aee2-1db2bdb6cc89}");
        ScriptDebugAgent() = default;
        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::SystemTickBus
        void OnSystemTick() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // ScriptDebugAgentBus
        void RegisterContext(AZ::ScriptContext* sc, const char* name) override;
        void UnregisterContext(AZ::ScriptContext* sc) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        ScriptDebugAgent(const ScriptDebugAgent&) = delete;
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);
        static void Reflect(AZ::ReflectContext* context);

        void Attach(const RemoteToolsEndpointInfo& ti, const char* scriptContextName);
        void Detach();

        void BreakpointCallback(AZ::ScriptContextDebug* debugContext, const AZ::ScriptContextDebug::Breakpoint* breakpoint);
        void DebugCommandCallback(AZ::ScriptContextDebug* debugContext);

        void Process();

        RemoteToolsEndpointInfo     m_debugger;
        RemoteToolsMessageQueue     m_msgQueue;
        AZStd::mutex            m_msgMutex;
        AZ::ScriptContext*   m_curContext;

        struct ContextRecord
        {
            AZ::ScriptContext*  m_context;
            AZStd::string          m_name;
        };
        typedef AZStd::vector<ContextRecord> ContextMap;

        ContextMap              m_availableContexts;

        enum SDA_STATE
        {
            SDA_STATE_DETACHED,
            SDA_STATE_RUNNING,
            SDA_STATE_PAUSED,
            SDA_STATE_DETACHING,
        };
        AZStd::atomic_uint      m_executionState;
    };
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::Init()
    {
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::Activate()
    {
        m_executionState = SDA_STATE_DETACHED;
        m_curContext = nullptr;

        // register default app script context if there is one
        AZ::ScriptContext* defaultScriptContext = nullptr;
        AZ::ScriptSystemRequestBus::BroadcastResult(
            defaultScriptContext, &AZ::ScriptSystemRequestBus::Events::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
        if (defaultScriptContext)
        {
            RegisterContext(defaultScriptContext, "Default");
        }

        AZ::ScriptContext* cryScriptContext = nullptr;
        AZ::ScriptSystemRequestBus::BroadcastResult(
            cryScriptContext, &AZ::ScriptSystemRequestBus::Events::GetContext, AZ::ScriptContextIds::CryScriptContextId);
        if (cryScriptContext)
        {
            RegisterContext(cryScriptContext, "Cry");
        }

        ScriptDebugAgentBus::Handler::BusConnect();
        AZ::SystemTickBus::Handler::BusConnect();
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::Deactivate()
    {
        AZ::SystemTickBus::Handler::BusDisconnect();

        // TODO: Make thread safe if we ever have multithreaded script contexts!
        if (m_executionState != SDA_STATE_DETACHED)
        {
            Detach();
        }

        AZStd::lock_guard<AZStd::mutex> l(m_msgMutex);
        m_msgQueue.clear();
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::OnSystemTick()
    {
        AzFramework::IRemoteTools* remoteTools = AzFramework::RemoteToolsInterface::Get();
        if (remoteTools)
        {
            const AzFramework::ReceivedRemoteToolsMessages* messages =
                remoteTools->GetReceivedMessages(AzFramework::LuaToolsKey);
            if (messages)
            {
                for (const AzFramework::RemoteToolsMessagePointer& msg : *messages)
                {
                    AZStd::lock_guard<AZStd::mutex> l(m_msgMutex);
                    m_msgQueue.push_back(msg);
                }
                remoteTools->ClearReceivedMessages(AzFramework::LuaToolsKey);
            }
        }

        // If we are attached, then all processing should happen
        // in the attached context.
        if (m_executionState == SDA_STATE_DETACHED)
        {
            Process();
        }
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::RegisterContext(AZ::ScriptContext* sc, const char* name)
    {
        for (ContextMap::const_iterator it = m_availableContexts.begin(); it != m_availableContexts.end(); ++it)
        {
            if (it->m_context == sc)
            {
                AZ_Assert(false, "ScriptContext 0x%p is already registered as %s! New registration ignored.", sc, it->m_name.c_str());
                return;
            }
        }
        m_availableContexts.emplace_back().m_context = sc;
        m_availableContexts.back().m_name = name;
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::UnregisterContext(AZ::ScriptContext* sc)
    {
        for (ContextMap::const_iterator it = m_availableContexts.begin(); it != m_availableContexts.end(); ++it)
        {
            if (it->m_context == sc)
            {
                if (m_curContext == sc)
                {
                    // TODO: This operation needs to be thread-safe if we ever run contexts from multiple threads.
                    Detach();
                }
                m_availableContexts.erase(it);
                return;
            }
        }
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::Attach(const RemoteToolsEndpointInfo& ti, const char* scriptContextName)
    {
        for (ContextMap::iterator it = m_availableContexts.begin(); it != m_availableContexts.end(); ++it)
        {
            if (azstricmp(scriptContextName, it->m_name.c_str()) == 0)
            {
                AZ::ScriptContext* sc = it->m_context;
                AZ_Assert(sc, "How did we end up with a NULL in the available contexts map?");

                m_debugger = ti;
                m_curContext = sc;
                sc->EnableDebug();
                AZ::ScriptContextDebug* dbgContext = sc->GetDebugContext();
                if (dbgContext)
                {
                    dbgContext->EnableStackRecord();

                    AZ::ScriptContextDebug::BreakpointCallback breakpointCallback = AZStd::bind(&ScriptDebugAgent::BreakpointCallback, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
                    dbgContext->EnableBreakpoints(breakpointCallback);

                    AZ::ScriptContextDebug::ProcessDebugCmdCallback debugCommandCallback = AZStd::bind(&ScriptDebugAgent::DebugCommandCallback, this, AZStd::placeholders::_1);
                    dbgContext->EnableDebugCmdProcess(debugCommandCallback);
                }

                // Notify debugger that we successfully connected
                RemoteToolsInterface::Get()->SendRemoteToolsMessage(
                    ti, ScriptDebugAck(AZ_CRC_CE("AttachDebugger"), AZ_CRC_CE("Ack")));
                AZ_TracePrintf("LUA", "Remote debugger %s has attached to context %s.\n", m_debugger.GetDisplayName(), it->m_name.c_str());
                m_executionState = SDA_STATE_RUNNING;

                return;
            }
        }
        // Failed to find context, notify debugger that the connection was rejected.
        RemoteToolsInterface::Get()->SendRemoteToolsMessage(
            ti, ScriptDebugAck(AZ_CRC_CE("AttachDebugger"), AZ_CRC_CE("IllegalOperation")));
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::Detach()
    {
        RemoteToolsInterface::Get()->SendRemoteToolsMessage(
            m_debugger, ScriptDebugAck(AZ_CRC_CE("DetachDebugger"), AZ_CRC_CE("Ack")));

        // TODO: We need to make sure we are thread safe if the contexts are running on
        // different threads.
        //if (m_curContext->GetErrorHookUserData() == this) {
        //  m_curContext->SetErrorHook(NULL);
        //}
        AZ::ScriptContextDebug* debugContext = m_curContext->GetDebugContext();
        debugContext->DisableBreakpoints();
        debugContext->DisableStackRecord();
        debugContext->DisableDebugCmdProcess();
        m_curContext->DisableDebug();

        AZ_TracePrintf("LUA", "Remote debugger %s has detached from context 0x%p.\n", m_debugger.GetDisplayName(), m_curContext);
        m_debugger = RemoteToolsEndpointInfo();
        m_curContext = nullptr;
        m_executionState = SDA_STATE_DETACHED;
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::DebugCommandCallback(AZ::ScriptContextDebug* debugContext)
    {
        (void)debugContext;
        AZ_Assert(m_curContext, "We are debugging without a script context!");
        AZ_Assert(m_curContext->GetDebugContext() == debugContext, "Context mismatch. Are we attached to the correct script context?");

        if (m_executionState != SDA_STATE_DETACHED)
        {
            // This is the only safe place to tear down the debug context because
            // it is the only function that runs in the script context thread and is never
            // called from within any debugContext callbacks.
            if (m_executionState == SDA_STATE_DETACHING)
            {
                AZ_TracePrintf("LUA", "Disabling debugging for script context(0x%p).\n", m_curContext);
                Detach();
            }
            else
            {
                Process();
            }
        }
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::BreakpointCallback(AZ::ScriptContextDebug* debugContext, const AZ::ScriptContextDebug::Breakpoint* breakpoint)
    {
        (void)debugContext;
        AZ_Assert(m_curContext, "We are debugging without a script context!");
        AZ_Assert(m_curContext->GetDebugContext() == debugContext, "Context mismatch. Are we attached to the correct script context?");

        if (m_executionState == SDA_STATE_RUNNING)
        {
            m_executionState = SDA_STATE_PAUSED;

#if AZ_TRAIT_AZFRAMEWORK_SHOW_MOUSE_ON_LUA_BREAKPOINT
            // We are about to block the main thread, only allowing to process
            // network events. This works fine on Windows, but on Linux the mouse
            // pointer doesn't show up when the user ALT+TAB out of the Editor window.
            // We need to make mouse cursor visible again and it fixes all the Linux
            // problems, and it doesn't hurt Windows either.
            SystemCursorState systemCursorState{}; // Remember the state of the cursor.
            AzFramework::InputSystemCursorRequestBus::Event(
                AzFramework::InputDeviceMouse::Id,
                [&systemCursorState](AzFramework::InputSystemCursorRequests* requests)
                {
                    systemCursorState = requests->GetSystemCursorState();
                    requests->SetSystemCursorState(AzFramework::SystemCursorState::UnconstrainedAndVisible);
                }
            );
#endif

            ScriptDebugAckBreakpoint response;
            response.m_id = AZ_CRC_CE("BreakpointHit");
            response.m_moduleName = breakpoint->m_sourceName;
            response.m_line = static_cast<AZ::u32>(breakpoint->m_lineNumber);
            RemoteToolsInterface::Get()->SendRemoteToolsMessage(m_debugger, response);

            while (m_executionState == SDA_STATE_PAUSED)
            {
                AzFramework::IRemoteTools* remoteTools = AzFramework::RemoteToolsInterface::Get();
                if (remoteTools)
                {
                    const AzFramework::ReceivedRemoteToolsMessages* messages =
                        remoteTools->GetReceivedMessages(AzFramework::LuaToolsKey);
                    if (messages)
                    {
                        for (const AzFramework::RemoteToolsMessagePointer& msg : *messages)
                        {
                            AZStd::lock_guard<AZStd::mutex> l(m_msgMutex);
                            m_msgQueue.push_back(msg);
                        }
                        remoteTools->ClearReceivedMessages(AzFramework::LuaToolsKey);
                    }
                }
                Process();
                AZ::Interface<AzNetworking::INetworking>::Get()->ForceUpdate();
                AZStd::this_thread::yield();
            }

#if AZ_TRAIT_AZFRAMEWORK_SHOW_MOUSE_ON_LUA_BREAKPOINT
            // Restore the state of the mouse cursor, and the game should continue running as usual.
            AzFramework::InputSystemCursorRequestBus::Event(
                AzFramework::InputDeviceMouse::Id,
                &AzFramework::InputSystemCursorRequests::SetSystemCursorState,
                systemCursorState);
#endif

        }
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::Process()
    {
        // Process messages
        AZ::ScriptContextDebug* dbgContext = m_curContext ? m_curContext->GetDebugContext() : nullptr;
        while (!m_msgQueue.empty())
        {
            m_msgMutex.lock();
            RemoteToolsMessagePointer msg = *m_msgQueue.begin();
            m_msgQueue.pop_front();
            m_msgMutex.unlock();
            AZ_Assert(msg, "We received a NULL message in the script debug agent's message queue!");
            RemoteToolsEndpointInfo sender = RemoteToolsInterface::Get()->GetEndpointInfo(AzFramework::LuaToolsKey, msg->GetSenderTargetId());

            // The only message we accept without a target match is AttachDebugger
            if (!m_debugger.IsIdentityEqualTo(sender))
            {
                ScriptDebugRequest* request = azdynamic_cast<ScriptDebugRequest*>(msg.get());
                if (!request ||
                    (request->m_request != AZ_CRC_CE("AttachDebugger") &&
                     request->m_request != AZ_CRC_CE("EnumContexts")))
                {
                    AZ_TracePrintf(
                        "LUA", "Rejecting msg 0x%x (%s is not the attached debugger)\n", request->m_request, sender.GetDisplayName());
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(
                        sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("AccessDenied")));
                    continue;
                }
            }

            if (azrtti_istypeof<ScriptDebugBreakpointRequest*>(msg.get()))
            {
                ScriptDebugBreakpointRequest* request = azdynamic_cast<ScriptDebugBreakpointRequest*>(msg.get());

                AZ::ScriptContextDebug::Breakpoint bp;
                bp.m_sourceName = request->m_context.c_str();
                bp.m_lineNumber = request->m_line;

                if (request->m_request == AZ_CRC_CE("AddBreakpoint"))
                {
                    AZ_TracePrintf("LUA", "Adding breakpoint %s:%d\n", bp.m_sourceName.c_str(), bp.m_lineNumber);
                    dbgContext->AddBreakpoint(bp);
                }
                else if (request->m_request == AZ_CRC_CE("RemoveBreakpoint"))
                {
                    AZ_TracePrintf("LUA", "Removing breakpoint %s:%d\n", bp.m_sourceName.c_str(), bp.m_lineNumber);
                    dbgContext->RemoveBreakpoint(bp);
                }

                ScriptDebugAckBreakpoint response;
                response.m_id = request->m_request;
                response.m_moduleName = request->m_context;
                response.m_line = request->m_line;
                RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
            }
            else if (azrtti_istypeof<ScriptDebugSetValue*>(msg.get()))      // sets the value of a variable
            {
                if (m_executionState == SDA_STATE_PAUSED)
                {
                    ScriptDebugSetValue* request = azdynamic_cast<ScriptDebugSetValue*>(msg.get());
                    ScriptDebugSetValueResult response;
                    response.m_name = request->m_value.m_name;
                    response.m_result = dbgContext->SetValue(request->m_value);
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                }
                else
                {
                    AZ_TracePrintf("LUA", "Command rejected. 'SetValue' can only be issued while on a breakpoint.\n");
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(AZ_CRC_CE("SetValue"), AZ_CRC_CE("IllegalOperation")));
                }
            }
            else if (azrtti_istypeof<ScriptDebugRequest*>(msg.get()))
            {
                ScriptDebugRequest* request = azdynamic_cast<ScriptDebugRequest*>(msg.get());
                // Check request type
                // EnumLocals
                if (request->m_request == AZ_CRC_CE("EnumLocals"))     // enumerates local variables
                {
                    if (m_executionState == SDA_STATE_PAUSED)
                    {
                        ScriptDebugEnumLocalsResult response;
                        AZ::ScriptContextDebug::EnumLocalCallback enumCB = AZStd::bind(&ScriptDebugAgentInternal::EnumLocals, &response.m_names, AZStd::placeholders::_1, AZStd::placeholders::_2);
                        dbgContext->EnumLocals(enumCB);
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    }
                    else
                    {
                        AZ_TracePrintf("LUA", "Command rejected. 'EnumLocals' can only be issued while on a breakpoint.\n");
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("IllegalOperation")));
                    }
                    // GetValue
                }
                else if (request->m_request == AZ_CRC_CE("GetValue"))
                {
                    ScriptDebugGetValueResult response;
                    response.m_value.m_name = request->m_context;
                    dbgContext->GetValue(response.m_value);
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    // StepOver
                }
                else if (request->m_request == AZ_CRC_CE("StepOver"))
                {
                    if (m_executionState == SDA_STATE_PAUSED)
                    {
                        dbgContext->StepOver();
                        m_executionState = SDA_STATE_RUNNING;
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("Ack")));
                    }
                    else
                    {
                        AZ_TracePrintf("LUA", "Command rejected. 'StepOver' can only be issued while on a breakpoint.\n");
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("IllegalOperation")));
                    }
                    // StepIn
                }
                else if (request->m_request == AZ_CRC_CE("StepIn"))
                {
                    if (m_executionState == SDA_STATE_PAUSED)
                    {
                        dbgContext->StepInto();
                        m_executionState = SDA_STATE_RUNNING;
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("Ack")));
                    }
                    else
                    {
                        AZ_TracePrintf("LUA", "Command rejected. 'StepIn' can only be issued while on a breakpoint.\n");
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("IllegalOperation")));
                    }
                    // StepOut
                }
                else if (request->m_request == AZ_CRC_CE("StepOut"))
                {
                    if (m_executionState == SDA_STATE_PAUSED)
                    {
                        dbgContext->StepOut();
                        m_executionState = SDA_STATE_RUNNING;
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("Ack")));
                    }
                    else
                    {
                        AZ_TracePrintf("LUA", "Command rejected. 'StepOut' can only be issued while on a breakpoint.\n");
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("IllegalOperation")));
                    }
                    // Continue
                }
                else if (request->m_request == AZ_CRC_CE("Continue"))
                {
                    if (m_executionState == SDA_STATE_PAUSED)
                    {
                        m_executionState = SDA_STATE_RUNNING;
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("Ack")));
                    }
                    else
                    {
                        AZ_TracePrintf("LUA", "Command rejected. 'Continue' can only be issued while on a breakpoint.\n");
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("IllegalOperation")));
                    }
                    // GetCallstack
                }
                else if (request->m_request == AZ_CRC_CE("GetCallstack"))
                {
                    if (m_executionState == SDA_STATE_PAUSED)
                    {
                        char bufStackTrace[4096];
                        dbgContext->StackTrace(bufStackTrace, 4096);
                        ScriptDebugCallStackResult response;
                        response.m_callstack = bufStackTrace;
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    }
                    else
                    {
                        AZ_TracePrintf("LUA", "Command rejected. 'GetCallstack' can only be issued while on a breakpoint.\n");
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("IllegalOperation")));
                    }
                    // enumerates global C++ functions that have been exposed to script
                }
                else if (request->m_request == AZ_CRC_CE("EnumRegisteredGlobals"))
                {
                    ScriptDebugRegisteredGlobalsResult response;
                    dbgContext->EnumRegisteredGlobals(&ScriptDebugAgentInternal::EnumGlobalMethod, &ScriptDebugAgentInternal::EnumGlobalProperty, &response);
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    // enumerates C++ classes that have been exposed to script
                }
                else if (request->m_request == AZ_CRC_CE("EnumRegisteredClasses"))
                {
                    ScriptDebugRegisteredClassesResult response;
                    dbgContext->EnumRegisteredClasses(&ScriptDebugAgentInternal::EnumClass, &ScriptDebugAgentInternal::EnumClassMethod, &ScriptDebugAgentInternal::EnumClassProperty, &response.m_classes);
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    // enumerates C++ busses that have been exposed to script
                }
                else if (request->m_request == AZ_CRC_CE("EnumRegisteredEBuses"))
                {
                    ScriptDebugRegisteredEBusesResult response;
                    dbgContext->EnumRegisteredEBuses(&ScriptDebugAgentInternal::EnumEBus, &ScriptDebugAgentInternal::EnumEBusSender, &response.m_ebusList);
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    // ExecuteScript
                }
                else if (request->m_request == AZ_CRC_CE("ExecuteScript"))
                {
                    if (sender.IsSelf() && m_debugger.IsSelf() && m_executionState == SDA_STATE_RUNNING)
                    {
                        AZ_Assert(!request->GetCustomBlob().empty(), "ScriptDebugAgent was asked to execute a script but script is missing!");
                        ScriptDebugAckExecute response;
                        response.m_moduleName = request->m_context;
                        response.m_result =
                            m_curContext->Execute(reinterpret_cast<const char*>(request->GetCustomBlob().data()), request->m_context.c_str());
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    }
                    else
                    {
                        AZ_TracePrintf("LUA", "Command rejected. 'ExecuteScript' cannot be issued while on a breakpoint or remotely.\n");
                        RemoteToolsInterface::Get()->SendRemoteToolsMessage(
                            sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("IllegalOperation")));
                    }
                    // AttachDebugger
                }
                else if (request->m_request == AZ_CRC_CE("AttachDebugger"))
                {
                    if (m_executionState == SDA_STATE_DETACHED)
                    {
                        Attach(sender, request->m_context.c_str());
                    }
                    else if (m_debugger.GetNetworkId() != sender.GetNetworkId())
                    {
                        // we need to detach from the current context first
                        AZ_TracePrintf("LUA", "Received connection from %s while still connected to %s! Detaching from %s.\n", sender.GetDisplayName(), m_debugger.GetDisplayName(), m_debugger.GetDisplayName());
                        AZ_TracePrintf("LUA", "Force disconnecting debugger %s from context 0x%p.\n", m_debugger.GetDisplayName(), m_curContext);
                        AZStd::lock_guard<AZStd::mutex> l(m_msgMutex);
                        m_msgQueue.push_front(msg);
                        m_executionState = SDA_STATE_DETACHING;
                    }
                    // We need to switch contexts before any more processing, keep remaining messages
                    // in the queue and return.
                    return;
                    // DetachDebugger
                }
                else if (request->m_request == AZ_CRC_CE("DetachDebugger"))
                {
                    // We need to switch contexts before any more processing, keep remaining messages
                    // in the queue and return.
                    if (m_executionState != SDA_STATE_DETACHED)
                    {
                        m_executionState = SDA_STATE_DETACHING;
                    }
                    return;
                    // EnumContexts
                }
                else if (request->m_request == AZ_CRC_CE("EnumContexts"))
                {
                    AZ_TracePrintf("LUA", "Received EnumContexts request\n");
                    ScriptDebugEnumContextsResult response;
                    for (ContextMap::const_iterator it = m_availableContexts.begin(); it != m_availableContexts.end(); ++it)
                    {
                        response.m_names.push_back(it->m_name);
                    }
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, response);
                    // Invalid command
                }
                else
                {
                    AZ_TracePrintf("LUA", "Received invalid command 0x%x.\n", request->m_request);
                    RemoteToolsInterface::Get()->SendRemoteToolsMessage(sender, ScriptDebugAck(request->m_request, AZ_CRC_CE("InvalidCmd")));
                }
            }
            else
            {
                AZ_Assert(false, "ScriptDebugAgent received a message that is not of any recognized types!");
            }
        }

        // Check if our debugger is still around
        if (m_executionState != SDA_STATE_DETACHED)
        {
            if (!RemoteToolsInterface::Get()->IsEndpointOnline(AzFramework::LuaToolsKey, m_debugger.GetPersistentId()))
            {
                m_executionState = SDA_STATE_DETACHING;
            }
        }
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptDebugService"));
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ScriptDebugService"));
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC_CE("ScriptService"));
    }
    //-------------------------------------------------------------------------
    void ScriptDebugAgent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ScriptDebugAgent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ScriptDebugAgent>(
                    "Script Debug Agent", "Provides remote debugging services for script contexts")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Profiling")
                    ;
            }
        }
        ReflectScriptDebugClasses(context);

        static bool registeredComponentUuidWithMetricsAlready = false;
        if (!registeredComponentUuidWithMetricsAlready)
        {
            // have to let the metrics system know that it's ok to send back the name of the ScriptDebugAgent component to Amazon as plain
            // text, without hashing
            AzFramework::MetricsPlainTextNameRegistrationBus::Broadcast(
                &AzFramework::MetricsPlainTextNameRegistrationBus::Events::RegisterForNameSending,
                AZStd::vector<AZ::Uuid>{ azrtti_typeid<ScriptDebugAgent>() });

            // only ever do this once
            registeredComponentUuidWithMetricsAlready = true;
        }
    }

    ////-------------------------------------------------------------------------
    //class ScriptDebugAgentFactory : public AZ::ComponentFactory<ScriptDebugAgent>
    //{
    //public:
    //    AZ_CLASS_ALLOCATOR(ScriptDebugAgentFactory, AZ::SystemAllocator,0);

    //    ScriptDebugAgentFactory() : AZ::ComponentFactory<ScriptDebugAgent>(AZ_CRC_CE("ScriptDebugAgent"))
    //    {
    //    }

    //    virtual const char*   GetName() { return "ScriptDebugAgent"; }
    //    virtual void      Reflect(const AZ::ClassDataReflection& reflection)
    //    {
    //        if( reflection.m_serialize )
    //        {
    //            reflection.m_serialize->Class<ScriptDebugAgent>("ScriptDebugAgent", "{6CEA890A-CEC0-4725-8E9A-97ACCE5941A9}")
    //                ->Version(1)

    //            AZ::EditContext *ec = reflection.m_serialize->GetEditContext();
    //            if (ec) {
    //                ec->Class<ScriptDebugAgent>("Script Debug Agent", "Provides remote debugging services for script contexts.");
    //            }
    //        }
    //        ReflectScriptDebugClasses(reflection);
    //    }
    //};
    //-------------------------------------------------------------------------
    AZ::ComponentDescriptor* CreateScriptDebugAgentFactory()
    {
        return ScriptDebugAgent::CreateDescriptor();
    }
    //-------------------------------------------------------------------------
}   // namespace AzFramework
