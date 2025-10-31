/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptCanvas/Variable/GraphVariable.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/GraphScopedTypes.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    void ReplicaNetworkProperties::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ReplicaNetworkProperties>()
                ->Version(1)
                ->Field("m_isSynchronized", &ReplicaNetworkProperties::m_isSynchronized)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<ReplicaNetworkProperties>("ReplicaNetworkProperties", "Network Properties")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ReplicaNetworkProperties::m_isSynchronized, "Is Synchronized", "Controls whether or not this value is reflected across the network.")
                    ;
            }
        }
    }

    namespace VariableFlags
    {
        const char* GetScopeDisplayLabel(Scope scopeType)
        {
            return GraphVariable::s_ScopeNames[static_cast<int>(scopeType)];
        }

        Scope GetScopeFromLabel(const char* label)
        {
            if (strcmp(GraphVariable::s_ScopeNames[static_cast<int>(VariableFlags::Scope::Function)], label) == 0)
            {
                return Scope::Function;
            }

            return Scope::Graph;
        }

        const char* GetScopeToolTip(Scope scopeType)
        {
            switch (scopeType)
            {
            case Scope::Graph:
                return "Variable is accessible in the entire graph.";
            case Scope::Function:
            case Scope::FunctionReadOnly:
                return "Variable is accessible only in the execution path of the function that defined it";
            default:
                return "?";
            }
        }
    }

    //////////////////
    // GraphVariable
    //////////////////

    class BehaviorVariableChangedBusHandler : public VariableNotificationBus::Handler, public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(BehaviorVariableChangedBusHandler, "{6469646D-EB7A-4F76-89E3-81EF05D2E688}", AZ::SystemAllocator,
            OnVariableValueChanged);

        // Sent when the light is turned on.
        void OnVariableValueChanged() override
        {
            Call(FN_OnVariableValueChanged);
        }
    };

    static bool GraphVariableVersionConverter([[maybe_unused]] AZ::SerializeContext& context, [[maybe_unused]] AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 4)
        {
            enum class DeprecatedScope : AZ::u8
            {
                Local = 0,
                Input = 1,
                Output = 2,
                InOut = 3
            };

            DeprecatedScope scope = DeprecatedScope::Local;
            classElement.GetChildData<DeprecatedScope>(AZ_CRC_CE("Scope"), scope);
            if (scope == DeprecatedScope::Input)
            {
                classElement.RemoveElementByName(AZ_CRC_CE("Scope"));
                classElement.AddElementWithData<VariableFlags::InitialValueSource>(context, "InitialValueSource", VariableFlags::InitialValueSource::Component);
            }

            classElement.RemoveElementByName(AZ_CRC_CE("ExposeAsInput"));
            classElement.RemoveElementByName(AZ_CRC_CE("Exposure"));
        }
        else
            if (classElement.GetVersion() < 3)
            {
                bool exposeAsInputField = false;
                classElement.GetChildData<bool>(AZ_CRC_CE("ExposeAsInput"), exposeAsInputField);

                if (exposeAsInputField)
                {
                    classElement.RemoveElementByName(AZ_CRC_CE("Exposure"));
                    classElement.AddElementWithData<VariableFlags::Scope>(context, "Scope", VariableFlags::Scope::Graph);
                }
                else
                {
                    AZ::u8 exposureType = VariableFlags::Deprecated::Exposure::Exp_Local;
                    classElement.GetChildData<AZ::u8>(AZ_CRC_CE("Exposure"), exposureType);

                    VariableFlags::Scope scope = VariableFlags::Scope::Graph;

                    if (((exposureType & VariableFlags::Deprecated::Exposure::Exp_InOut) == VariableFlags::Deprecated::Exposure::Exp_InOut)
                    || exposureType & VariableFlags::Deprecated::Exposure::Exp_Input)
                    {
                        scope = VariableFlags::Scope::Graph;
                    }
                    else if (exposureType & VariableFlags::Deprecated::Exposure::Exp_Output)
                    {
                        scope = VariableFlags::Scope::Function;
                    }

                    classElement.AddElementWithData<VariableFlags::Scope>(context, "Scope", scope);
                }

                classElement.RemoveElementByName(AZ_CRC_CE("Exposure"));
                classElement.RemoveElementByName(AZ_CRC_CE("ExposeAsInput"));
            }

        return true;
    }

    const char* GraphVariable::s_InitialValueSourceNames[VariableFlags::InitialValueSource::COUNT] =
    {
        "From Graph",
        "From Component"
    };

    const char* GraphVariable::s_ScopeNames[static_cast<int>(VariableFlags::Scope::COUNT)] =
    {
        "Graph",
        "Function",
        "Function",
    };


    void GraphVariable::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Don't want to store the scoped id. That will need to be generated at point.
            // For now we focus only on the identifier.
            serializeContext->Class<GraphScopedVariableId>()
                ->Version(1)
                ->Field("Identifier", &GraphScopedVariableId::m_identifier)
                ;


            serializeContext->Class<GraphVariable>()
                ->Version(4, &GraphVariableVersionConverter)
                ->Field("Datum", &GraphVariable::m_datum)
                ->Field("InputControlVisibility", &GraphVariable::m_inputControlVisibility)
                ->Field("ExposureCategory", &GraphVariable::m_exposureCategory)
                ->Field("SortPriority", &GraphVariable::m_sortPriority)
                ->Field("ReplicaNetProps", &GraphVariable::m_networkProperties)
                ->Field("VariableId", &GraphVariable::m_variableId)
                ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &VariableId::MakeVariableId)
                ->Field("VariableName", &GraphVariable::m_variableName)
                ->Field("Scope", &GraphVariable::m_scope)
                ->Field("InitialValueSource", &GraphVariable::m_InitialValueSource)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphVariable>("Variable", "Represents a Variable field within a Script Canvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChildNameLabelOverride, &GraphVariable::GetVariableName)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &GraphVariable::GetVariableName)
                    ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &GraphVariable::GetDescriptionOverride)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GraphVariable::m_InitialValueSource, "Initial Value Source", "Variables can get their values from within the graph or through component properties.")
                    ->Attribute(AZ::Edit::Attributes::GenericValueList, &GraphVariable::GetPropertyChoices)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnInitialValueSourceChanged)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetInputControlVisibility)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_datum, "Datum", "Datum within Script Canvas Graph")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnValueChanged)

                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GraphVariable::m_scope, "Scope", "Controls the scope of this variable. i.e. If this is exposed as input to this script, or output from this script, or if the variable is just locally scoped.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetScopeControlVisibility)
                    ->Attribute(AZ::Edit::Attributes::GenericValueList, &GraphVariable::GetScopeChoices)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnScopeTypedChanged)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_networkProperties, "Network Properties", "Enables whether or not this value should be network synchronized")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetNetworkSettingsVisibility)

                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_sortPriority, "Display Order", "Allows for customizable display order. -1 implies it will be at the end of the list.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetInputControlVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnSortPriorityChanged)
                    ->Attribute(AZ::Edit::Attributes::Min, -1)

                    ;
            }
        }

        AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);

        if (behaviorContext)
        {
            behaviorContext->Class<GraphScopedVariableId>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;

            behaviorContext->EBus<VariableNotificationBus>(GetVariableNotificationBusName(), "VariableNotificationBus", "Notifications from the Variables in the current Script Canvas graph")
                ->Attribute(AZ::Script::Attributes::Category, "Variables")
                ->Handler<BehaviorVariableChangedBusHandler>()
                ;
        }

        ReplicaNetworkProperties::Reflect(context);
    }

    GraphVariable::GraphVariable()
        : m_sortPriority(-1)
        , m_scope(VariableFlags::Scope::Graph)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
        , m_InitialValueSource(VariableFlags::InitialValueSource::Graph)
    {
    }

    GraphVariable::GraphVariable(const Datum& datum)
        : m_sortPriority(-1)
        , m_scope(VariableFlags::Scope::Graph)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
        , m_datum(datum)
        , m_InitialValueSource(VariableFlags::InitialValueSource::Graph)

    {
    }

    GraphVariable::GraphVariable(Datum&& datum)
        : m_sortPriority(-1)
        , m_scope(VariableFlags::Scope::Graph)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
        , m_datum(AZStd::move(datum))
        , m_InitialValueSource(VariableFlags::InitialValueSource::Graph)

    {
    }

    GraphVariable::GraphVariable(const Datum& variableData, const VariableId& variableId)
        : GraphVariable(variableData)
    {
        m_variableId = variableId;
    }

    GraphVariable::GraphVariable(Deprecated::VariableNameValuePair&& valuePair)
        : GraphVariable(AZStd::move(valuePair.m_varDatum.GetData()))
    {
        SetVariableName(AZStd::move(valuePair.GetVariableName()));
        m_variableId = valuePair.m_varDatum.GetId();

        if (valuePair.m_varDatum.ExposeAsComponentInput())
        {
            SetScope(VariableFlags::Scope::Graph);
        }

        m_inputControlVisibility = valuePair.m_varDatum.GetInputControlVisibility();
        m_visibility = valuePair.m_varDatum.GetVisibility();

        m_exposureCategory = valuePair.m_varDatum.GetExposureCategory();

        m_signalValueChanges = valuePair.m_varDatum.AllowsSignalOnChange();
    }

    GraphVariable::~GraphVariable()
    {
        DatumNotificationBus::Handler::BusDisconnect();
    }

    void GraphVariable::DeepCopy(const GraphVariable& source)
    {
        *this = source;
        m_datum.DeepCopyDatum(source.m_datum);
    }

    bool GraphVariable::operator==(const GraphVariable& rhs) const
    {
        return GetVariableId() == rhs.GetVariableId();
    }

    bool GraphVariable::operator!=(const GraphVariable& rhs) const
    {
        return !operator==(rhs);
    }

    const Data::Type& GraphVariable::GetDataType() const
    {
        return GetDatum()->GetType();
    }

    const VariableId& GraphVariable::GetVariableId() const
    {
        return m_variableId;
    }

    const Datum* GraphVariable::GetDatum() const
    {
        return &m_datum;
    }

    Datum& GraphVariable::ModDatum()
    {
        return m_datum;
    }

    void GraphVariable::ConfigureDatumView(ModifiableDatumView& datumView)
    {
        datumView.ConfigureView((*this));
    }

    bool GraphVariable::IsComponentProperty() const
    {
        return m_scope == VariableFlags::Scope::Graph && m_InitialValueSource == ScriptCanvas::VariableFlags::InitialValueSource::Component;
    }

    void GraphVariable::SetVariableName(AZStd::string_view variableName)
    {
        m_variableName = variableName;
    }

    AZStd::string_view GraphVariable::GetVariableName() const
    {
        return m_variableName;
    }

    void GraphVariable::SetScriptInputControlVisibility(const AZ::Crc32& inputControlVisibility)
    {
        m_inputControlVisibility = inputControlVisibility;
    }

    AZ::Crc32 GraphVariable::GetScopeControlVisibility() const
    {
        if (m_scope == VariableFlags::Scope::FunctionReadOnly)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return GetInputControlVisibility();
    }

    AZ::Crc32 GraphVariable::GetInputControlVisibility() const
    {
        return m_inputControlVisibility;
    }

    AZ::Crc32 GraphVariable::GetNetworkSettingsVisibility() const
    {
        bool showNetworkSettings = false;
        ScriptCanvasSettingsRequestBus::BroadcastResult(showNetworkSettings, &ScriptCanvasSettingsRequests::CanShowNetworkSettings);
        if (!showNetworkSettings)
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }

        return AZ::Edit::PropertyVisibility::Show;
    }

    AZ::Crc32 GraphVariable::GetVisibility() const
    {
        return m_visibility;
    }

    void GraphVariable::SetVisibility(AZ::Crc32 visibility)
    {
        m_visibility = visibility;
    }

    void GraphVariable::SetScope(VariableFlags::Scope scopeType)
    {
        if (m_scope != scopeType)
        {
            m_scope = scopeType;
            OnScopeTypedChanged();
        }
    }

    VariableFlags::Scope GraphVariable::GetScope() const
    {
        return m_scope;
    }

    bool GraphVariable::IsInScope(VariableFlags::Scope scopeType) const
    {
        switch (scopeType)
        {
        case VariableFlags::Scope::Graph:
            return m_scope == VariableFlags::Scope::Graph;
            // All graph variables are in function local scope
        case VariableFlags::Scope::Function:
        case VariableFlags::Scope::FunctionReadOnly:

            return true;
        }

        return false;
    }

    void GraphVariable::GenerateNewId()
    {
        m_variableId = VariableId::MakeVariableId();
    }

    void GraphVariable::SetAllowSignalOnChange(bool allowSignalChange)
    {
        m_signalValueChanges = allowSignalChange;
    }

    void GraphVariable::SetOwningScriptCanvasId(const ScriptCanvasId& scriptCanvasId)
    {
        if (m_scriptCanvasId != scriptCanvasId)
        {
            m_scriptCanvasId = scriptCanvasId;

            if (!m_datumId.IsValid())
            {
                m_datumId = AZ::Entity::MakeId();
                m_datum.SetNotificationsTarget(m_datumId);
                DatumNotificationBus::Handler::BusConnect(m_datumId);
            }
        }
    }

    GraphScopedVariableId GraphVariable::GetGraphScopedId() const
    {
        return GraphScopedVariableId(m_scriptCanvasId, m_variableId);
    }

    AZ::u32 GraphVariable::SetInitialValueSource(VariableFlags::InitialValueSource InitialValueSource)
    {
        m_InitialValueSource = InitialValueSource;
        return OnInitialValueSourceChanged();
    }

    AZ::u32 GraphVariable::SetInitialValueSourceFromName(AZStd::string_view name)
    {
        for (int i = 0; i < VariableFlags::InitialValueSource::COUNT; ++i)
        {
            if (name.compare(s_InitialValueSourceNames[i]) == 0)
            {
                return SetInitialValueSource(static_cast<VariableFlags::InitialValueSource>(i));
            }
        }

        return 0;
    }

    void GraphVariable::OnDatumEdited([[maybe_unused]] const Datum* datum)
    {
        VariableNotificationBus::Event(GetGraphScopedId(), &VariableNotifications::OnVariableValueChanged);
    }

    AZStd::vector<AZStd::pair<VariableFlags::Scope, AZStd::string>> GraphVariable::GetScopes() const
    {
        AZStd::vector< AZStd::pair<VariableFlags::Scope, AZStd::string>> scopes;
        scopes.emplace_back(AZStd::make_pair(m_scope, VariableFlags::GetScopeDisplayLabel(m_scope)));
        return scopes;
    }

    int GraphVariable::GetSortPriority() const
    {
        return m_sortPriority;
    }

    AZ::u32 GraphVariable::OnInitialValueSourceChanged()
    {
        VariableNotificationBus::Event(GetGraphScopedId(), &VariableNotifications::OnVariableInitialValueSourceChanged);
        return AZ::Edit::PropertyRefreshLevels::EntireTree;
    }

    void GraphVariable::OnScopeTypedChanged()
    {
        VariableNotificationBus::Event(GetGraphScopedId(), &VariableNotifications::OnVariableScopeChanged);
    }

    void GraphVariable::OnSortPriorityChanged()
    {
        VariableNotificationBus::Event(GetGraphScopedId(), &VariableNotifications::OnVariablePriorityChanged);
    }

    void GraphVariable::OnValueChanged()
    {
        if (m_signalValueChanges)
        {
            AZ_TracePrintf("OnValueChanged", "OnValueChanged");
            //VariableNotificationBus::Event(GetVariableId(), &VariableNotifications::OnVariableValueChanged);
        }
    }

    AZStd::string GraphVariable::GetDescriptionOverride()
    {
        return Data::GetName(m_datum.GetType());
    }
}
