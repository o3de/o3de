/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Logger.h"

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/std/smart_ptr/make_shared.h>


namespace ScriptCanvas
{
    using namespace AzFramework;

    namespace Debugger
    {
        Logger::Logger()
        {
            ScriptCanvas::Debugger::ServiceNotificationsBus::Handler::BusConnect();
            ScriptCanvas::ExecutionLogAssetEBus::Handler::BusConnect();
        }

        Logger::~Logger()
        {
            ScriptCanvas::Debugger::ServiceNotificationsBus::Handler::BusDisconnect();
            ScriptCanvas::ExecutionLogAssetEBus::Handler::BusDisconnect();
        }

        void Logger::ClearLog()
        {
#if defined(SC_EXECUTION_TRACE_ENABLED)
            m_logAsset.GetData().Clear();
#endif // SC_EXECUTION_TRACE_ENABLED
        }

        void Logger::ClearLogExecutionOverride()
        {
            m_logExecutionOverrideEnabled = false;
        }

        void Logger::Connected(const ScriptCanvas::Debugger::Target& target)
        {
            m_target = target;
        }

        AZ::Data::Asset<ExecutionLogAsset> Logger::LoadFromRelativePath([[maybe_unused]] AZStd::string_view path)
        {
#if defined(SC_EXECUTION_TRACE_ENABLED) 
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to retrieve serialize context.");

            AZ::IO::FixedMaxPath fullpath = ExecutionLogAsset::GetDefaultDirectoryPath() /path;

            AZ::IO::FileIOStream fileStream;
            if (!fileStream.Open(fullpath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText))
            {
                AZ_Error("ScriptCanvas", false, "Failed to open path: %s", fullpath.c_str());
                return {};
            }

            AZ::TypeId executionLogTypeId = AZ::AzTypeInfo<ExecutionLogAsset>::Uuid();
            if (ExecutionLogAsset* logAsset = reinterpret_cast<ExecutionLogAsset*>(AZ::Utils::LoadObjectFromStream(fileStream, serializeContext, &executionLogTypeId, AZ::ObjectStream::FilterDescriptor())))
            {
                return AZ::Data::Asset<ExecutionLogAsset>(logAsset, AZ::Data::AssetLoadBehavior::Default);
            }
            else
            {
                AZ_Error("ScriptCanvas", false, "Failed to load object: %s", fullpath.c_str());
            }
#endif
            return {};
        }

        void Logger::SaveToRelativePath([[maybe_unused]] AZStd::string_view path)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to retrieve serialize context.");

#if defined(SC_EXECUTION_TRACE_ENABLED) 
            AZ::IO::FixedMaxPath fullpath = ExecutionLogAsset::GetDefaultDirectoryPath() / path;

            AZ_VerifyError("ScriptCanvas", AZ::Utils::SaveObjectToFile(fullpath.String(), AZ::DataStream::ST_XML, &m_logAsset),
                "File failed to save: %s", fullpath.c_str());
#endif
        }

        void Logger::SetLogExecutionOverride(bool value)
        {
            m_logExecutionOverrideEnabled = true;
            m_logExecutionOverride = value;
        }

        void Logger::GraphActivated(const ScriptCanvas::GraphActivation& activation)
        {
            AddToLog(activation);
        }

        void Logger::GraphDeactivated(const ScriptCanvas::GraphDeactivation& deactivation)
        {
            AddToLog(deactivation);
        }

        void Logger::NodeStateChanged(const ScriptCanvas::NodeStateChange& stateChange)
        {
            AddToLog(stateChange);
        }

        void Logger::SignaledInput(const ScriptCanvas::InputSignal& signal)
        {
            AddToLog(signal);
        }

        void Logger::SignaledOutput(const ScriptCanvas::OutputSignal& signal)
        {
            AddToLog(signal);
        }

        void Logger::VariableChanged(const ScriptCanvas::VariableChange& variableChange)
        {
            AddToLog(variableChange);
        }
    }
}
