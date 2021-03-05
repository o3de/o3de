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

#include <ScriptCanvas/Variable/GraphVariable.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/GraphScopedTypes.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>
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
            switch (scopeType)
            {
            case Scope::Local:
                return "Local";
            case Scope::Input:
                return "In";
            case Scope::Output:
                return "Out";
            case Scope::InOut:
                return "In/Out";
            default:
                return "?";
            }
        }

        Scope GetScopeFromLabel(const char* label)
        {
            if (strcmp("In", label) == 0)
            {
                return Scope::Input;
            }
            else if (strcmp("Out", label) == 0)
            {
                return Scope::Output;
            }
            else if (strcmp("In/Out", label) == 0)
            {
                return Scope::InOut;
            }

            return Scope::Local;
        }

        const char* GetScopeToolTip(Scope scopeType)
        {
            switch (scopeType)
            {
            case Scope::Local:
                return "Variable is for use in the local scope only.";
            case Scope::Input:
                return "Variable will have an initial value set from an external source.";
            case Scope::Output:
                return "Variable will be used to return values to an external source.";
            case Scope::InOut:
                return "Variable will be used to receive and return values to an external source.";
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

    static bool GraphVariableVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 3)
        {
            bool exposeAsInputField = false;
            classElement.GetChildData<bool>(AZ_CRC("ExposeAsInput", 0x0f7879f0), exposeAsInputField);

            if (exposeAsInputField)
            {
                classElement.RemoveElementByName(AZ_CRC("Exposure", 0x398f29cd));
                classElement.AddElementWithData<VariableFlags::Scope>(context, "Scope", VariableFlags::Scope::Input);
            }
            else
            {
                AZ::u8 exposureType = VariableFlags::Deprecated::Exposure::Exp_Local;
                classElement.GetChildData<AZ::u8>(AZ_CRC("Exposure", 0x398f29cd), exposureType);

                VariableFlags::Scope scope = VariableFlags::Scope::Local;

                if ((exposureType & VariableFlags::Deprecated::Exposure::Exp_InOut) == VariableFlags::Deprecated::Exposure::Exp_InOut)
                {
                    scope = VariableFlags::Scope::InOut;
                }
                else if (exposureType & VariableFlags::Deprecated::Exposure::Exp_Input)
                {
                    scope = VariableFlags::Scope::Input;
                }
                else if (exposureType & VariableFlags::Deprecated::Exposure::Exp_Output)
                {
                    scope = VariableFlags::Scope::Output;
                }

                classElement.AddElementWithData<VariableFlags::Scope>(context, "Scope", scope);
            }

            classElement.RemoveElementByName(AZ_CRC("Exposure", 0x398f29cd));
            classElement.RemoveElementByName(AZ_CRC("ExposeAsInput", 0x0f7879f0));
        }

        return true;
    }

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
                ->Version(3, &GraphVariableVersionConverter)
                ->Field("Datum", &GraphVariable::m_datum)
                ->Field("InputControlVisibility", &GraphVariable::m_inputControlVisibility)
                ->Field("ExposureCategory", &GraphVariable::m_exposureCategory)
                ->Field("SortPriority", &GraphVariable::m_sortPriority)
                ->Field("ReplicaNetProps", &GraphVariable::m_networkProperties)
                ->Field("VariableId", &GraphVariable::m_variableId)
                ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &VariableId::MakeVariableId)
                ->Field("VariableName", &GraphVariable::m_variableName)
                ->Field("Scope", &GraphVariable::m_scope)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<GraphVariable>("Variable", "Represents a Variable field within a Script Canvas Graph")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetVisibility)
                    ->Attribute(AZ::Edit::Attributes::ChildNameLabelOverride, &GraphVariable::GetDisplayName)
                    ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &GraphVariable::GetDisplayName)
                    ->Attribute(AZ::Edit::Attributes::DescriptionTextOverride, &GraphVariable::GetDescriptionOverride)
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GraphVariable::m_scope, "Scope", "Controls the scope of this variable. i.e. If this is exposed as input to this script, or output from this script, or if the variable is just locally scoped.")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetInputControlVisibility)
                    ->Attribute(AZ::Edit::Attributes::GenericValueList, &GraphVariable::GetScopes)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnScopeTypedChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_datum, "Datum", "Datum within Script Canvas Graph")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ->Attribute(AZ::Edit::Attributes::ChangeNotify, &GraphVariable::OnValueChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GraphVariable::m_networkProperties, "Network Properties", "Enables whether or not this value should be network synchronized")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &GraphVariable::GetScriptInputControlVisibility)
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
        , m_scope(VariableFlags::Scope::Local)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
    {
    }

    GraphVariable::GraphVariable(const Datum& datum)
        : m_sortPriority(-1)
        , m_scope(VariableFlags::Scope::Local)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
        , m_datum(datum)

    {
    }

    GraphVariable::GraphVariable(Datum&& datum)
        : m_sortPriority(-1)
        , m_scope(VariableFlags::Scope::Local)
        , m_inputControlVisibility(AZ::Edit::PropertyVisibility::Show)
        , m_visibility(AZ::Edit::PropertyVisibility::ShowChildrenOnly)
        , m_signalValueChanges(false)
        , m_variableId(VariableId::MakeVariableId())
        , m_datum(AZStd::move(datum))

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
            SetScope(VariableFlags::Scope::Input);
        }
        else
        {
            SetScope(VariableFlags::Scope::Local);
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

    void GraphVariable::ConfigureDatumView(ModifiableDatumView& datumView)
    {
        datumView.ConfigureView((*this));
    }

    void GraphVariable::SetVariableName(AZStd::string_view variableName)
    {
        m_variableName = variableName;
        SetDisplayName(variableName);
    }

    AZStd::string_view GraphVariable::GetVariableName() const
    {
        return m_variableName;
    }

    void GraphVariable::SetDisplayName(const AZStd::string& displayName)
    {
        m_datum.SetLabel(displayName);
    }

    AZStd::string_view GraphVariable::GetDisplayName() const
    {
        return m_datum.GetLabel();
    }

    void GraphVariable::SetScriptInputControlVisibility(const AZ::Crc32& inputControlVisibility)
    {
        m_inputControlVisibility = inputControlVisibility;
    }

    AZ::Crc32 GraphVariable::GetInputControlVisibility() const
    {
        return m_inputControlVisibility;
    }

    AZ::Crc32 GraphVariable::GetScriptInputControlVisibility() const
    {
        AZ::Data::AssetType assetType = AZ::Data::AssetType::CreateNull();

        ScriptCanvas::RuntimeRequestBus::EventResult(assetType, m_scriptCanvasId, &ScriptCanvas::RuntimeRequests::GetAssetType);

        if (assetType == azrtti_typeid<ScriptCanvas::RuntimeAsset>())
        {
            return m_inputControlVisibility;
        }
        else
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }
    }

    AZ::Crc32 GraphVariable::GetFunctionInputControlVisibility() const
    {
        AZ::Data::AssetType assetType = AZ::Data::AssetType::CreateNull();
        ScriptCanvas::RuntimeRequestBus::EventResult(assetType, m_scriptCanvasId, &ScriptCanvas::RuntimeRequests::GetAssetType);

        if (assetType == azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>())
        {
            return AZ::Edit::PropertyVisibility::Show;
        }
        else
        {
            return AZ::Edit::PropertyVisibility::Hide;
        }
    }

    AZ::Crc32 GraphVariable::GetVisibility() const
    {
        return m_visibility;
    }

    void GraphVariable::SetVisibility(AZ::Crc32 visibility)
    {
        m_visibility = visibility;
    }

    void GraphVariable::RemoveScope(VariableFlags::Scope scopeType)
    {
        if (IsInScope(scopeType))
        {
            if (m_scope == VariableFlags::Scope::InOut)
            {
                if (scopeType == VariableFlags::Scope::Input)
                {
                    m_scope = VariableFlags::Scope::Output;
                }
                else if (scopeType == VariableFlags::Scope::Output)
                {
                    m_scope = VariableFlags::Scope::Input;
                }
                else
                {
                    m_scope = VariableFlags::Scope::Local;
                }
            }
            else
            {
                m_scope = VariableFlags::Scope::Local;
            }
        }      
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
            // All variables are local scoped
        case VariableFlags::Scope::Local:
            return true;
        case VariableFlags::Scope::Input:
            return m_scope == VariableFlags::Scope::Input || m_scope == VariableFlags::Scope::InOut;
        case VariableFlags::Scope::Output:
            return m_scope == VariableFlags::Scope::Output || m_scope == VariableFlags::Scope::InOut;
        case VariableFlags::Scope::InOut:
            return m_scope == VariableFlags::Scope::InOut;
        default:
            return false;
        }
    }

    bool GraphVariable::IsLocalVariableOnly() const
    {
        return m_scope == VariableFlags::Scope::Local;
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

    void GraphVariable::OnDatumEdited([[maybe_unused]] const Datum* datum)
    {
        VariableNotificationBus::Event(GetGraphScopedId(), &VariableNotifications::OnVariableValueChanged);
    }

    AZStd::vector<AZStd::pair<AZ::u8, AZStd::string>> GraphVariable::GetScopes() const
    {
        AZStd::vector< AZStd::pair<AZ::u8, AZStd::string>> scopes;

        scopes.emplace_back(AZStd::make_pair(VariableFlags::Scope::Local, VariableFlags::GetScopeDisplayLabel(VariableFlags::Scope::Local)));
        scopes.emplace_back(AZStd::make_pair(VariableFlags::Scope::Input, VariableFlags::GetScopeDisplayLabel(VariableFlags::Scope::Input)));

        if (IsInFunction())
        {
            scopes.emplace_back(AZStd::make_pair(VariableFlags::Scope::Output, VariableFlags::GetScopeDisplayLabel(VariableFlags::Scope::Output)));
            scopes.emplace_back(AZStd::make_pair(VariableFlags::Scope::InOut, VariableFlags::GetScopeDisplayLabel(VariableFlags::Scope::InOut)));
        }

        return scopes;
    }

    int GraphVariable::GetSortPriority() const
    {
        return m_sortPriority;
    }

    bool GraphVariable::IsInFunction() const
    {
        AZ::Data::AssetType assetType = AZ::Data::AssetType::CreateNull();
        ScriptCanvas::RuntimeRequestBus::EventResult(assetType, m_scriptCanvasId, &ScriptCanvas::RuntimeRequests::GetAssetType);

        return assetType == azrtti_typeid<ScriptCanvas::RuntimeFunctionAsset>();
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
