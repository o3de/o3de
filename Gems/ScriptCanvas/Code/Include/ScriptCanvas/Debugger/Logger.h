/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/EntityId.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Asset/ExecutionLogAsset.h>
#include <ScriptCanvas/Asset/ExecutionLogAssetBus.h>
#include <AzCore/EBus/EBus.h>
#include <Debugger/Bus.h>
#include <Debugger/Messages/Notify.h>

#include "APIArguments.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        class Logger
            : public ServiceNotificationsBus::Handler
            , public ExecutionLogAssetEBus::Handler
        {
            using Mutex = AZStd::recursive_mutex;
            using Lock = AZStd::lock_guard<Mutex>;

        public:
            AZ_CLASS_ALLOCATOR(Logger, AZ::SystemAllocator, 0);
            AZ_RTTI(Logger, "{BBA556C4-973B-4B2F-B2B9-357188086F78}");
            
            Logger();
            ~Logger();
                
            //////////////////////////////////////////////////////////////////////////
            // ServiceNotificationsBus::Handler
            void Connected(const ScriptCanvas::Debugger::Target&) override;
            void GraphActivated(const ScriptCanvas::GraphActivation&) override;
            void GraphDeactivated(const ScriptCanvas::GraphDeactivation&) override;
            void NodeStateChanged(const ScriptCanvas::NodeStateChange&) override;
            void SignaledInput(const ScriptCanvas::InputSignal&) override;
            void SignaledOutput(const ScriptCanvas::OutputSignal&) override;
            void VariableChanged(const ScriptCanvas::VariableChange&) override;
            //////////////////////////////////////////////////////////////////////////
                        
            //////////////////////////////////////////////////////////////////////////
            // ExecutionLogAssetBus::Handler
            void ClearLog() override;
            void ClearLogExecutionOverride() override;
            AZ::Data::Asset<ExecutionLogAsset> LoadFromRelativePath(AZStd::string_view path) override;
            void SaveToRelativePath(AZStd::string_view path) override;
            void SetLogExecutionOverride(bool value) override;
            //////////////////////////////////////////////////////////////////////////

        protected:
            AZ_FORCE_INLINE bool IsLoggingExecution() const 
            {
                return m_target.m_script.m_logExecution
                    || (m_logExecutionOverrideEnabled && m_logExecutionOverride);
            }

            template<typename t_Event>
            void AddToLog(const t_Event& loggableEvent)
            {
                SCRIPT_CANVAS_DEBUGGER_TRACE_CLIENT("Logging: %s", loggableEvent.ToString().data());
                m_logAsset.GetData().m_events.emplace_back(loggableEvent.Duplicate());
            }

        private:
            Mutex m_mutex;
            
            bool m_logExecutionOverrideEnabled = false;
            bool m_logExecutionOverride = false;

            ExecutionLogAsset m_logAsset;
            ScriptCanvas::Debugger::Target m_target;
        };
    }
}
