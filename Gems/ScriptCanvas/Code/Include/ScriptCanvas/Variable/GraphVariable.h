/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/GraphScopedTypes.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include <ScriptCanvas/Core/DatumBus.h>

// Version Conversion Information
#include <ScriptCanvas/Deprecated/VariableHelpers.h>
////

namespace ScriptCanvas
{
    class ModifiableDatumView;

    //! Properties that govern Datum replication
    struct ReplicaNetworkProperties
    {
    AZ_TYPE_INFO(ReplicaNetworkProperties, "{4F055551-DD75-4877-93CE-E80C844FC155}");
    AZ_CLASS_ALLOCATOR(ReplicaNetworkProperties, AZ::SystemAllocator, 0);

    static void Reflect(AZ::ReflectContext* context);

    bool m_isSynchronized = false;
    };

    namespace VariableFlags
    {
        namespace Deprecated
        {
            enum Exposure : AZ::u8
            {
                Exp_Local = 1 << 0,
                Exp_Input = 1 << 1,
                Exp_Output = 1 << 2,
                Exp_InOut = (Exp_Input | Exp_Output)
            };
        }

        enum class Scope : AZ::u8
        {
            Graph,
            Function,
            FunctionReadOnly,
            COUNT
        };

        enum InitialValueSource : AZ::u8
        {
            Graph,
            Component,
            COUNT
        };

        const char* GetScopeDisplayLabel(Scope scopeType);
        Scope GetScopeFromLabel(const char* label);
        const char* GetScopeToolTip(Scope scopeType);
    }

    class GraphVariable
        : public DatumNotificationBus::Handler
    {
        friend class ModifiableDatumView;

    public:

        AZ_TYPE_INFO(GraphVariable, "{5BDC128B-8355-479C-8FA8-4BFFAB6915A8}");
        AZ_CLASS_ALLOCATOR(GraphVariable, AZ::SystemAllocator, 0);
        static void Reflect(AZ::ReflectContext* context);

        static const char* GetVariableNotificationBusName() { return k_OnVariableWriteEbusName; }

        class Comparator
        {
        public:

            bool operator()(const GraphVariable* a, const GraphVariable* b) const
            {
                if (a->m_sortPriority == b->m_sortPriority)
                {
                    return a->m_variableName < b->m_variableName;
                }

                if (b->m_sortPriority < 0)
                {
                    return true;
                }

                if (a->m_sortPriority < 0)
                {
                    return false;
                }
                
                return a->m_sortPriority < b->m_sortPriority;
            }
        };

        GraphVariable();
        explicit GraphVariable(const Datum& variableData);
        explicit GraphVariable(Datum&& variableData);

        GraphVariable(const Datum& variableData, const VariableId& variableId);

        // Conversion Information
        GraphVariable(Deprecated::VariableNameValuePair&& valuePair);
        ////

        ~GraphVariable();

        bool operator==(const GraphVariable& rhs) const;
        bool operator!=(const GraphVariable& rhs) const;

        void DeepCopy(const GraphVariable& source);

        const Data::Type& GetDataType() const;

        const VariableId& GetVariableId() const;

        const Datum*    GetDatum() const;

        Datum& ModDatum();

        bool IsComponentProperty() const;

        void ConfigureDatumView(ModifiableDatumView& accessController);
        
        void SetVariableName(AZStd::string_view displayName);
        AZStd::string_view GetVariableName() const;

        void SetScriptInputControlVisibility(const AZ::Crc32& inputControlVisibility);

        AZ::Crc32 GetInputControlVisibility() const;
        AZ::Crc32 GetScopeControlVisibility() const;
        AZ::Crc32 GetNetworkSettingsVisibility() const;

        AZ::Crc32 GetVisibility() const;
        void SetVisibility(AZ::Crc32 visibility);

        void SetScope(VariableFlags::Scope scopeType);
        VariableFlags::Scope GetScope() const;

        bool IsInScope(VariableFlags::Scope scopeType) const;

        void SetExposureCategory(AZStd::string_view exposureCategory) { m_exposureCategory = exposureCategory; }
        AZStd::string_view GetExposureCategory() const { return m_exposureCategory; }

        void GenerateNewId();

        void SetAllowSignalOnChange(bool allowSignalChange);
        
        bool IsSynchronized() const { return m_networkProperties.m_isSynchronized; }

        void SetOwningScriptCanvasId(const ScriptCanvasId& scriptCanvasId);
        GraphScopedVariableId GetGraphScopedId() const;

        AZStd::string_view GetInitialValueSourceName() const { return s_InitialValueSourceNames[m_InitialValueSource]; }
        VariableFlags::InitialValueSource GetInitialValueSource() const { return m_InitialValueSource; }
        AZ::u32 SetInitialValueSource(VariableFlags::InitialValueSource InitialValueSource);

        AZ::u32 SetInitialValueSourceFromName(AZStd::string_view name);


        // Editor Callbacks
        void OnDatumEdited(const Datum* datum) override;
        AZStd::vector<AZStd::pair<VariableFlags::Scope, AZStd::string>> GetScopes() const;
        ////

        int GetSortPriority() const;

        static const char* s_InitialValueSourceNames[VariableFlags::InitialValueSource::COUNT];
        static const char* s_ScopeNames[static_cast<int>(VariableFlags::Scope::COUNT)];

    private:

        AZStd::vector<AZStd::pair<unsigned char, AZStd::string>> GetPropertyChoices() const
        {
            AZStd::vector< AZStd::pair<unsigned char, AZStd::string>> choices;
            choices.emplace_back(AZStd::make_pair(static_cast<unsigned char>(VariableFlags::InitialValueSource::Graph), s_InitialValueSourceNames[0]));
            choices.emplace_back(AZStd::make_pair(static_cast<unsigned char>(VariableFlags::InitialValueSource::Component), s_InitialValueSourceNames[1]));
            return choices;
        }

        AZStd::vector<AZStd::pair<unsigned char, AZStd::string>> GetScopeChoices() const
        {
            AZStd::vector< AZStd::pair<unsigned char, AZStd::string>> choices;
            choices.emplace_back(AZStd::make_pair(static_cast<unsigned char>(VariableFlags::Scope::Graph), s_ScopeNames[0]));
            choices.emplace_back(AZStd::make_pair(static_cast<unsigned char>(VariableFlags::Scope::Function), s_ScopeNames[1]));
            return choices;
        }
                
        void OnScopeTypedChanged();
        AZ::u32 OnInitialValueSourceChanged();
        void OnSortPriorityChanged();

        void OnValueChanged();

        AZStd::string GetDescriptionOverride();

        int m_sortPriority;

        VariableFlags::Scope m_scope;
        VariableFlags::InitialValueSource m_InitialValueSource;

        // Still need to make this a proper bitmask, once we have support for multiple
        // input/output attributes. For now, just going to assume it's only the single flag(which is is).
        AZ::Crc32 m_inputControlVisibility;
        AZ::Crc32 m_visibility;

        AZStd::string m_exposureCategory;

        bool m_signalValueChanges;

        ScriptCanvasId m_scriptCanvasId;
        VariableId m_variableId;

        AZ::EntityId m_datumId;

        AZStd::string m_variableName;
        Datum m_datum;
        
        ReplicaNetworkProperties m_networkProperties;

    };

    using GraphVariableMapping = AZStd::unordered_map< VariableId, GraphVariable >;
}
namespace AZ
{

}
