/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Core.h"
#include "Core/Datum.h"
#include "Core/Endpoint.h"
#include "Variable/GraphVariable.h"
#include "Core/NamedId.h"

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/time.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    class ExecutionState;

    // Graph info made to be serialized and sent over the network
    struct GraphInfo
    {
        AZ_CLASS_ALLOCATOR(GraphInfo, AZ::SystemAllocator);
        AZ_RTTI(GraphInfo, "{5E7ED577-2F0E-4BC2-97A0-B3B7307EDA26}");

        NamedActiveEntityId m_runtimeEntity;
        GraphIdentifier m_graphIdentifier;

        GraphInfo() = default;
        GraphInfo(const GraphInfo&) = default;
        GraphInfo(ExecutionStateWeakConstPtr executionState);
        GraphInfo(const NamedActiveEntityId& runtimeEntity, const GraphIdentifier& graphIdentifier);
        virtual ~GraphInfo() = default;

        bool operator==(const GraphInfo& graphInfo) const;

        AZStd::string ToString() const;
    };

    struct VariableIdentifier
    {
        AZ_CLASS_ALLOCATOR(VariableIdentifier, AZ::SystemAllocator);
        AZ_TYPE_INFO(VariableIdentifier, "{7DC089F4-B3D7-4F85-AA88-D215DF3C6831}");

        VariableId m_variableId;
        GraphIdentifier m_graphId;

        VariableIdentifier() = default;

        VariableIdentifier(VariableId variableId, const GraphIdentifier& graphId)
            : m_variableId(variableId)
            , m_graphId(graphId)
        {}

        AZStd::string ToString() const;
    };
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::GraphIdentifier>
    {
        using argument_type = ScriptCanvas::GraphIdentifier;
        using result_type = AZStd::size_t;

        AZ_FORCE_INLINE size_t operator()(const argument_type& argument) const
        {
            AZStd::size_t graphIdentifierHash = AZStd::hash<AZ::Data::AssetId>()(argument.m_assetId);
            AZStd::hash_combine(graphIdentifierHash, argument.m_componentId);

            return graphIdentifierHash;
        }
    };

    template<>
    struct hash<ScriptCanvas::GraphInfo>
    {
        using argument_type = ScriptCanvas::GraphInfo;
        using result_type = AZStd::size_t;

        AZ_FORCE_INLINE size_t operator()(const argument_type& argument) const
        {
            AZStd::size_t graphInfoHash = AZStd::hash<AZ::EntityId>()(argument.m_runtimeEntity);
            AZStd::hash_combine(graphInfoHash, argument.m_graphIdentifier);

            return graphInfoHash;
        }
    };
}

namespace ScriptCanvas
{
    using Timestamp = AZ::u64;

    struct BreakTag
    {
        AZ_TYPE_INFO(BreakTag, "{B1B0976D-E300-470B-B01C-8EED7571414A}");
        static const char* ToString() { return "Break"; }
    };
    struct BreakpointTag
    {
        AZ_TYPE_INFO(BreakpointTag, "{4915585E-9AF7-4414-87D4-F1EE31E04E4D}");
        static const char* ToString() { return "Breakpoint"; }
    };
    struct ContinueTag
    {
        AZ_TYPE_INFO(ContinueTag, "{611DF6CA-24CC-4F6B-89BF-4EDE56661040}");
        static const char* ToString() { return "Continue"; }
    };
    struct ExecutionThreadBeginTag
    {
        AZ_TYPE_INFO(ExecutionThreadBeginTag, "{43C2F51D-17E9-4B4A-A1EF-3D5FD39857A4}");
        static const char* ToString() { return "ExecutionThreadBegin"; }
    };
    struct ExecutionThreadEndTag
    {
        AZ_TYPE_INFO(ExecutionThreadEndTag, "{1BD155E9-ED07-4900-A6C9-04704A79424B}");
        static const char* ToString() { return "ExecutionThreadEnd"; }
    };
    struct GetAvailableScriptTargetsTag
    {
        AZ_TYPE_INFO(GetAvailableScriptTargetsTag, "{D6B4D3FE-5975-4974-8DF4-CF823CCEEDB9}");
        static const char* ToString() { return "GetAvailableScriptTargets"; }
    };
    struct GetActiveEntitiesTag
    {
        AZ_TYPE_INFO(GetActiveEntitiesTag, "{F28305CE-7CC4-4481-BCAA-5347361496B1}");
        static const char* ToString() { return "GetActiveEntities"; }
    };
    struct GetActiveGraphsTag
    {
        AZ_TYPE_INFO(GetActiveGraphsTag, "{4AF50B18-87A7-45F0-925C-76D89DFC6DB6}");
        static const char* ToString() { return "GetActiveGraphs"; }
    };
    struct GetVariableValueTag
    {
        AZ_TYPE_INFO(GetVariableValueTag, "{DBD77ADA-B8A5-423F-8524-5F2C765A1E46}");
        static const char* ToString() { return "GetVariableValueTag"; }
    };
    struct GetVariableValuesTag
    {
        AZ_TYPE_INFO(GetVariableValuesTag, "{AEBE5DB8-DD6D-4B3F-AFDA-5A89010C21DF}");
        static const char* ToString() { return "GetVariableValuesTag"; }
    };
    struct GraphActivationTag
    {
        AZ_TYPE_INFO(GraphActivationTag, "{9DC4188F-52A1-4F95-A20C-FEFECDF48FEE}");
        static const char* ToString() { return "GraphActivation"; }
    };
    struct GraphDeactivationTag
    {
        AZ_TYPE_INFO(GraphDeactivationTag, "{FE4B8C6B-B8EE-4CA1-A4D4-DB559D977E22}");
        static const char* ToString() { return "GraphDeactivation"; }
    };
    struct InputSignalTag
    {
        AZ_TYPE_INFO(InputSignalTag, "{AFAE431F-4E4F-4AC6-8EBB-5D6A209280A4}");
        static const char* ToString() { return "InputSignal"; }
    };
    struct OutputSignalTag
    {
        AZ_TYPE_INFO(OutputSignalTag, "{6E8D6FA8-92C5-4EEB-82DE-8CF4293F83E6}");
        static const char* ToString() { return "OutputSignal"; }
    };
    struct ReturnSignalTag
    {
        AZ_TYPE_INFO(ReturnSignalTag, "{CFA657CE-6073-4D3C-B5EF-B7BA624A4C19}");
        static const char* ToString() { return "ReturnSignal"; }
    };
    struct AnnotateNodeSignalTag
    {
        AZ_TYPE_INFO(AnnotateNodeSignalTag, "{6F61974F-B1BB-4377-8903-B360C50A28EC}");
        static const char* ToString() { return "AnnotateNodeSignal"; }
    };
    struct StepOverTag
    {
        AZ_TYPE_INFO(StepOverTag, "{44980605-0FF2-4A5C-870E-324B4184ADD6}");
        static const char* ToString() { return "StepOver"; }
    };
    struct VariableChangeTag
    {
        AZ_TYPE_INFO(VariableChangeTag, "{2936D848-1EA1-4B07-A462-F52F8A0ED395}");
        static const char* ToString() { return "VariableChange"; }
    };

    class LoggableEventVisitor;

    struct LoggableEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(LoggableEvent, AZ::SystemAllocator);
        AZ_RTTI(LoggableEvent, "{0ACA3F48-170F-4859-9ED7-9C60523758A7}");

        virtual ~LoggableEvent() = default;

        virtual LoggableEvent* Duplicate() const = 0;
        virtual Timestamp GetTimestamp() const = 0;
        virtual void SetTimestamp(Timestamp) = 0;
        virtual AZStd::string ToString() const = 0;
        virtual void Visit(LoggableEventVisitor& visitor) = 0;
    };

    template<typename t_Tag, typename t_Parent>
    struct TaggedParent
        : public t_Parent
        , public LoggableEvent
    {
        using ThisType = TaggedParent<t_Tag, t_Parent>;
        AZ_RTTI((TaggedParent, "{CF75CEEE-2305-49D4-AD41-407E82F819D7}", t_Tag, t_Parent), t_Parent, LoggableEvent);
        AZ_CLASS_ALLOCATOR(ThisType, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ThisType, t_Parent, LoggableEvent>()
                    ->Version(0)
                    ->Field("timestamp", &ThisType::m_timestamp)
                    ;
            }
        }

        Timestamp m_timestamp = 0;

        TaggedParent()
            : m_timestamp(AZStd::GetTimeUTCMilliSecond())
        {}

        TaggedParent(const t_Parent& parent)
            : t_Parent(parent)
            , m_timestamp(AZStd::GetTimeUTCMilliSecond())
        {}

        LoggableEvent* Duplicate() const override
        {
            return aznew ThisType(*this);
        }

        Timestamp GetTimestamp() const override
        {
            return m_timestamp;
        }

        void SetTimestamp(Timestamp timestamp) override
        {
            m_timestamp = timestamp;
        }

        AZStd::string ToString() const override
        {
            return AZStd::string::format("%s:%s", t_Tag::ToString(), t_Parent::ToString().data());
        }

        void Visit(LoggableEventVisitor& visitor) override;
    };

    struct ActiveGraphStatus final
    {
        AZ_CLASS_ALLOCATOR(ActiveGraphStatus, AZ::SystemAllocator);
        AZ_TYPE_INFO(ActiveGraphStatus, "{6E251A99-EE03-4C12-9122-35A90CBB5891}");

        int               m_instanceCounter = 0;
        bool              m_isObserved = false;
    };

    using ActiveGraphStatusMap = AZStd::unordered_map< AZ::Data::AssetId, ActiveGraphStatus >;
    using EntityActiveGraphStatusMap = AZStd::unordered_map< GraphIdentifier, ActiveGraphStatus >;

    struct ActiveEntityStatus final
    {
        AZ_CLASS_ALLOCATOR(ActiveEntityStatus, AZ::SystemAllocator);
        AZ_TYPE_INFO(ActiveEntityStatus, "{7D6013B6-142F-446B-9995-54C84EF64F7B}");

        AZ::NamedEntityId m_namedEntityId;

        EntityActiveGraphStatusMap m_activeGraphs;
    };

    using ActiveEntityStatusMap = AZStd::unordered_map< AZ::EntityId, ActiveEntityStatus >;
    using ActiveEntitiesAndGraphs = AZStd::pair<ActiveEntityStatusMap, ActiveGraphStatusMap>;

    struct DatumValue
    {
        AZ_CLASS_ALLOCATOR(DatumValue, AZ::SystemAllocator);
        AZ_RTTI(DatumValue, "{5B4C8EA8-747E-4557-A10A-0EA0ADB387CA}");

        static DatumValue Create(const Datum& value);

        static DatumValue Create(const GraphVariable& value);

        // if valid, the datum will contain a string result of BCO->ToString()
        AZ::TypeId m_behaviorContextObjectType;
        Datum m_datum;

        DatumValue() = default;
        virtual ~DatumValue() = default;

        DatumValue(const DatumValue&) = default;

        DatumValue(AZ::TypeId behaviorContextObjectType, const AZStd::string& toStringResult)
            : m_behaviorContextObjectType(behaviorContextObjectType)
            , m_datum(Datum(toStringResult))
        {}

        DatumValue(const Datum& datum)
            : m_behaviorContextObjectType{}
            , m_datum(datum)
        {}

        AZStd::string ToString() const;
    };

    using SlotDataMap = AZStd::unordered_map<NamedSlotId, DatumValue>;
    using VariableValues = AZStd::unordered_map<VariableId, AZStd::pair<AZStd::string, DatumValue>>;

    struct ActivationInfo
        : public GraphInfo
    {
        AZ_CLASS_ALLOCATOR(ActivationInfo, AZ::SystemAllocator);
        AZ_RTTI(ActivationInfo, "{9EBCB557-80D1-43CA-840E-BB8945BF13F4}", GraphInfo);

        bool m_entityIsObserved = false;

        VariableValues m_variableValues;

        ActivationInfo() = default;

        virtual ~ActivationInfo() = default;

        ActivationInfo(const ActivationInfo&) = default;

        ActivationInfo(const GraphInfo& info);

        ActivationInfo(const GraphInfo& info, const VariableValues& variableValues);

        AZStd::string ToString() const;
    };

    struct Signal
        : public GraphInfo
    {
        AZ_CLASS_ALLOCATOR(Signal, AZ::SystemAllocator);
        AZ_RTTI(Signal, "{F65B92D1-10D8-4065-90FA-8FD46A9B122A}", GraphInfo);

        NodeTypeIdentifier m_nodeType;
        NamedEndpoint m_endpoint;
        SlotDataMap m_data;

        Signal() = default;

        Signal(const Signal& signal) = default;

        Signal(const GraphInfo& graphInfo)
            : GraphInfo(graphInfo)
        {}

        Signal(const GraphInfo& graphInfo, const NodeTypeIdentifier& nodeType, const NamedEndpoint& endpoint)
            : GraphInfo(graphInfo)
            , m_nodeType(nodeType)
            , m_endpoint(endpoint)
        {}

        Signal(const GraphInfo& graphInfo, const NodeTypeIdentifier& nodeType, const NamedEndpoint& endpoint, const SlotDataMap& data)
            : GraphInfo(graphInfo)
            , m_nodeType(nodeType)
            , m_endpoint(endpoint)
            , m_data(data)
        {}

        virtual ~Signal() = default;

        bool operator==(const Signal& other) const;

        AZStd::string ToString() const;
    };

    template<typename t_Tag>
    struct TaggedDataValue
        : public DatumValue
        , public GraphInfo
        , public LoggableEvent
    {
        using ThisType = TaggedDataValue<t_Tag>;

        AZ_RTTI((TaggedDataValue, "{893B73BA-E1CC-4D91-92D1-C1CF46817A57}", t_Tag), DatumValue, GraphInfo, LoggableEvent);
        AZ_CLASS_ALLOCATOR(TaggedDataValue<t_Tag>, AZ::SystemAllocator);
        using DatumValue::DatumValue;

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ThisType, DatumValue, GraphInfo, LoggableEvent>()
                    ->Version(0)
                    ->Field("timestamp", &ThisType::m_timestamp)
                    ;
            }
        }

        Timestamp m_timestamp = 0;

        TaggedDataValue()
            : m_timestamp(AZStd::GetTimeUTCMilliSecond())
        {}

        TaggedDataValue(const TaggedDataValue&) = default;

        TaggedDataValue(const GraphInfo& graphInfo, const DatumValue& dataValue)
            : DatumValue(dataValue)
            , GraphInfo(graphInfo)
            , m_timestamp(AZStd::GetTimeUTCMilliSecond())
        {}

        //TaggedDataValue(const Signal& signal)
        //    : Signal(signal)
        //    , m_timestamp(AZStd::GetTimeUTCMilliSecond())
        //{}

        LoggableEvent* Duplicate() const override
        {
            return aznew TaggedDataValue<t_Tag>(*this);
        }

        Timestamp GetTimestamp() const override
        {
            return m_timestamp;
        }

        void SetTimestamp(Timestamp timestamp) override
        {
            m_timestamp = timestamp;
        }

        AZStd::string ToString() const override
        {
            return AZStd::string::format("%s %s %s", t_Tag::ToString(), DatumValue::ToString().data(), GraphInfo::ToString().data());
        }

        void Visit(LoggableEventVisitor& visitor) override;
    };

    using Breakpoint = TaggedParent<BreakpointTag, Signal>;

    struct ExecutionThreadBeginning
        : public GraphInfo
        , public LoggableEvent
    {
        AZ_CLASS_ALLOCATOR(ExecutionThreadBeginning, AZ::SystemAllocator);
        AZ_RTTI(ExecutionThreadBeginning, "{410EB31A-F6DC-415D-848B-43537B962A43}", GraphInfo, LoggableEvent);

        NamedActiveEntityId m_nodeId;
        Timestamp m_timestamp;

        ExecutionThreadBeginning();

        ExecutionThreadBeginning(const ExecutionThreadBeginning&) = default;

        ExecutionThreadBeginning(const GraphInfo& graphInfo, AZ::EntityId nodeId)
            : GraphInfo(graphInfo)
            , m_nodeId(nodeId)
        {}

        virtual ~ExecutionThreadBeginning() = default;

        LoggableEvent* Duplicate() const override;

        Timestamp GetTimestamp() const override;

        void SetTimestamp(Timestamp timestamp) override;

        AZStd::string ToString() const override;

        void Visit(LoggableEventVisitor& visitor) override;
    };

    using ExecutionThreadEnd = TaggedParent<ExecutionThreadEndTag, GraphInfo>;
    using GraphActivation = TaggedParent<GraphActivationTag, ActivationInfo>;
    using GraphDeactivation = TaggedParent<GraphDeactivationTag, ActivationInfo>;
    using InputSignal = TaggedParent<InputSignalTag, Signal>;
    using OutputSignal = TaggedParent<OutputSignalTag, Signal>;
    using ReturnSignal = TaggedParent<ReturnSignalTag, Signal>;

    // Base class to handle some basic information
    struct GraphInfoEventBase
        : public GraphInfo
        , public LoggableEvent
    {
    public:
        AZ_CLASS_ALLOCATOR(GraphInfoEventBase, AZ::SystemAllocator);
        AZ_RTTI(GraphInfoEventBase, "{873431EB-7B4D-410A-9F2F-5E2E0E00140B}", GraphInfo, LoggableEvent);

        GraphInfoEventBase();
        GraphInfoEventBase(const GraphInfo& graphInfo);

        Timestamp GetTimestamp() const override final;
        void SetTimestamp(Timestamp timestamp) override final;

        Timestamp m_timestamp;
    };

    struct NodeStateChange
        : public GraphInfoEventBase
    {
        AZ_CLASS_ALLOCATOR(NodeStateChange, AZ::SystemAllocator);
        AZ_RTTI(NodeStateChange, "{6D3B9C70-E6E9-4780-87C0-D74E7BFBE53D}", GraphInfoEventBase);

        NodeStateChange();
        NodeStateChange(const NodeStateChange&) = default;

        LoggableEvent* Duplicate() const override;
        AZStd::string ToString() const override;

        void Visit(LoggableEventVisitor& visitor) override;
    };

    using VariableChange = TaggedDataValue<VariableChangeTag>;

    struct AnnotateNodeSignal
        : public GraphInfoEventBase
    {
    public:
        AZ_CLASS_ALLOCATOR(AnnotateNodeSignal, AZ::SystemAllocator);
        AZ_RTTI(AnnotateNodeSignal, "{EE13C14C-9EFA-47F6-9B23-900D71BC9DDE}", GraphInfoEventBase);

        enum AnnotationLevel
        {
            Info,
            Warning,
            Error
        };

        AnnotateNodeSignal();
        AnnotateNodeSignal(const AnnotateNodeSignal&) = default;
        AnnotateNodeSignal(const GraphInfo& graphInfo, AnnotationLevel annotationLevel, AZStd::string_view annotation, const AZ::NamedEntityId& assetId);

        LoggableEvent* Duplicate() const override;

        AZStd::string ToString() const override;

        void Visit(LoggableEventVisitor& visitor) override;

        AnnotationLevel m_annotationLevel;
        AZStd::string   m_annotation;

        AZ::NamedEntityId m_assetNodeId;
    };

    class ExecutionNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual void AnnotateNode(const AnnotateNodeSignal&) = 0;
        virtual void GraphActivated(const GraphActivation&) = 0;
        virtual void GraphDeactivated(const GraphDeactivation&) = 0;
        virtual void RuntimeError(const ExecutionState& executionState, const AZStd::string_view& description) = 0;
        virtual bool IsGraphObserved(const AZ::EntityId& entityId, const GraphIdentifier& identifier) = 0;
        virtual bool IsVariableObserved(const VariableId&) = 0;
        virtual void NodeSignaledOutput(const OutputSignal&) = 0;
        virtual void NodeSignaledInput(const InputSignal&) = 0;
        virtual void GraphSignaledReturn(const ReturnSignal&) = 0;
        virtual void NodeStateUpdated(const NodeStateChange&) = 0;
        virtual void VariableChanged(const VariableChange&) = 0;
    };
    using ExecutionNotificationsBus = AZ::EBus<ExecutionNotifications>;

    class LoggableEventVisitor
    {
    public:
        virtual ~LoggableEventVisitor() = default;

        // used for logging
        virtual void Visit(ExecutionThreadEnd&) = 0;
        virtual void Visit(ExecutionThreadBeginning&) = 0;
        virtual void Visit(GraphActivation&) = 0;
        virtual void Visit(GraphDeactivation&) = 0;
        virtual void Visit(NodeStateChange&) = 0;
        virtual void Visit(InputSignal&) = 0;
        virtual void Visit(OutputSignal&) = 0;
        virtual void Visit(ReturnSignal&) = 0;
        virtual void Visit(VariableChange&) = 0;
        virtual void Visit(AnnotateNodeSignal&) = 0;

        // should never show up in logging
        void Visit(Breakpoint&) {};
    };

    void ReflectExecutionBusArguments(AZ::ReflectContext* context);
    AZStd::string ToString(const SlotDataMap& map);
    AZStd::string ToString(const VariableValues& variableValues);

    template<typename t_Tag, typename t_Parent>
    void TaggedParent<t_Tag, t_Parent>::Visit(LoggableEventVisitor& visitor)
    {
        visitor.Visit(*this);
    }

    template<typename t_Tag>
    void TaggedDataValue<t_Tag>::Visit(LoggableEventVisitor& visitor)
    {
        visitor.Visit(*this);
    }
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::Breakpoint>
    {
        using argument_type = ScriptCanvas::Breakpoint;
        using result_type = AZStd::size_t;

        AZ_FORCE_INLINE size_t operator()(const argument_type& argument) const
        {
            result_type result = AZStd::hash<const AZ::u64>()(static_cast<AZ::u64>(argument.m_runtimeEntity));
            AZStd::hash_combine(result, argument.m_graphIdentifier);
            AZStd::hash_combine(result, argument.m_endpoint);
            return result;
        }
    };
}
