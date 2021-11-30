/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/SliceEntityOwnershipServiceBus.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Asset/RuntimeAssetHandler.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Libraries/Comparison/Comparison.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>
#include <ScriptCanvas/Libraries/Core/CoreNodes.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Libraries/Logic/Logic.h>
#include <ScriptCanvas/Libraries/Math/Math.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace AZ
{
    class ScriptAsset;
}

namespace ScriptCanvas
{
    class RuntimeAsset;
    class RuntimeComponent;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAsset;

    struct LoadTestGraphResult
    {
        AZStd::string_view m_graphPath;
        AZStd::unique_ptr<AZ::Entity> m_entity;
        ScriptCanvas::RuntimeComponent* m_runtimeComponent = nullptr;
        bool m_nativeFunctionFound = false;
        AZ::Data::Asset<ScriptCanvasEditor::ScriptCanvasAsset> m_editorAsset;
        AZ::Data::Asset<ScriptCanvas::RuntimeAsset> m_runtimeAsset;
        AZ::Data::Asset<AZ::ScriptAsset> m_scriptAsset;
    };

    // how long a unit test should be allowed to execute
    enum class eDuration
    {
        InitialActivation = 0, // kill the test as soon as control returns from the graph
        Seconds, // wait for the specified amount of seconds, regardless of ticks
        Ticks, // wait for the the specified amount of ticks, regardless of seconds
    };

    struct DurationSpec
    {
        eDuration m_spec = eDuration::InitialActivation;
        size_t m_ticks = 0;
        float m_seconds = 0.0f;
        float m_timeStep = (1.0f / 60.0f);

        static DurationSpec Seconds(float seconds);
        static DurationSpec Ticks(size_t ticks);
    };

    class TraceSuppressionRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void SuppressPreAssert(bool suppress) = 0;
        virtual void SuppressAssert(bool suppress) = 0;
        virtual void SuppressException(bool suppress) = 0;
        virtual void SuppressPreError(bool suppress) = 0;
        virtual void SuppressError(bool suppress) = 0;
        virtual void SuppressPreWarning(bool suppress) = 0;
        virtual void SuppressWarning(bool suppress) = 0;
        virtual void SuppressPrintf(bool suppress) = 0;
        virtual void SuppressAllOutput(bool suppress) = 0;
    };

    using TraceSuppressionBus = AZ::EBus<TraceSuppressionRequests>;

    class TraceMessageComponent
        : public AZ::Component
        , protected AZ::Debug::TraceMessageBus::Handler
        , protected TraceSuppressionBus::Handler
    {
    public:
        AZ_COMPONENT(TraceMessageComponent, "{E12144CE-809D-4056-9735-4384D7DBCCDC}");

        TraceMessageComponent() = default;
        ~TraceMessageComponent() override = default;

        void Activate() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
            TraceSuppressionBus::Handler::BusConnect();
        }

        void Deactivate() override
        {
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
            TraceSuppressionBus::Handler::BusDisconnect();
        }

    private:

        /// \ref ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<TraceMessageComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
        }

        bool OnPreAssert(const char*, int, const char*, const char*) override { return suppressPreAssert; }
        bool OnAssert(const char*) override { return suppressAssert; }
        bool OnException(const char*) override { return suppressException; }
        bool OnPreError(const char*, const char*, int, const char*, const char*) override { return suppressPreError; }
        bool OnError(const char*, const char*) override { return suppressError; }
        bool OnPreWarning(const char*, const char*, int, const char*, const char*) override { return suppressPreWarning; }
        bool OnWarning(const char*, const char*) override { return suppressWarning; }
        bool OnPrintf(const char*, const char*) override { return suppressPrintf; }
        bool OnOutput(const char*, const char*) override { return suppressAllOutput; }

        void SuppressPreAssert(bool suppress) override { suppressPreAssert = suppress; }
        void SuppressAssert(bool suppress)override { suppressAssert = suppress; }
        void SuppressException(bool suppress) override { suppressException = suppress; }
        void SuppressPreError(bool suppress) override { suppressPreError = suppress; }
        void SuppressError(bool suppress) override { suppressPreError = suppress; }
        void SuppressPreWarning(bool suppress) override { suppressPreWarning = suppress; }
        void SuppressWarning(bool suppress) override { suppressWarning = suppress; }
        void SuppressPrintf(bool suppress) override { suppressPrintf = suppress; }
        void SuppressAllOutput(bool suppress) override { suppressAllOutput = suppress; }

        bool suppressPreAssert = false;
        bool suppressAssert = false;
        bool suppressException = false;
        bool suppressPreError = false;
        bool suppressError = false;
        bool suppressPreWarning = false;
        bool suppressWarning = false;
        bool suppressPrintf = false;
        bool suppressAllOutput = false;
    };

    struct ScopedOutputSuppression
    {
        ScopedOutputSuppression(bool suppressState = true)
        {
            AZ::Debug::TraceMessageBus::BroadcastResult(m_oldSuppression, &AZ::Debug::TraceMessageEvents::OnOutput, "", "");
            TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressAllOutput, suppressState);
        }

        ~ScopedOutputSuppression()
        {
            TraceSuppressionBus::Broadcast(&TraceSuppressionRequests::SuppressAllOutput, m_oldSuppression);
        }
    private:
        bool m_oldSuppression = false;
    };

} // ScriptCanvasEditor
