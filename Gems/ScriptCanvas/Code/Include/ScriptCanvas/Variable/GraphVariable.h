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

        enum Scope : AZ::u8
        {
            Local = 0,
            Input = 1,
            Output = 2,
            InOut = 3
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

        static const char* GetVariableNotificationBusName() { return "VariableNotification"; }

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
        void            ConfigureDatumView(ModifiableDatumView& accessController);
        
        void SetVariableName(AZStd::string_view displayName);
        AZStd::string_view GetVariableName() const;

        void SetDisplayName(const AZStd::string& displayName);
        AZStd::string_view GetDisplayName() const;
        
        void SetScriptInputControlVisibility(const AZ::Crc32& inputControlVisibility);

        AZ::Crc32 GetInputControlVisibility() const;
        AZ::Crc32 GetScriptInputControlVisibility() const;
        AZ::Crc32 GetFunctionInputControlVisibility() const;
        
        AZ::Crc32 GetVisibility() const;
        void SetVisibility(AZ::Crc32 visibility);

        void RemoveScope(VariableFlags::Scope scopeType);
        void SetScope(VariableFlags::Scope scopeType);
        VariableFlags::Scope GetScope() const;

        bool IsInScope(VariableFlags::Scope scopeType) const;
        bool IsLocalVariableOnly() const;

        void SetExposureCategory(AZStd::string_view exposureCategory) { m_exposureCategory = exposureCategory; }
        AZStd::string_view GetExposureCategory() const { return m_exposureCategory; }

        void GenerateNewId();

        void SetAllowSignalOnChange(bool allowSignalChange);
        
        bool IsSynchronized() const { return m_networkProperties.m_isSynchronized; }

        void SetOwningScriptCanvasId(const ScriptCanvasId& scriptCanvasId);
        GraphScopedVariableId GetGraphScopedId() const;

        // Editor Callbacks
        void OnDatumEdited(const Datum* datum) override;
        AZStd::vector<AZStd::pair<AZ::u8, AZStd::string>> GetScopes() const;
        ////

        int GetSortPriority() const;

    private:

        bool IsInFunction() const;
        
        void OnScopeTypedChanged();
        void OnSortPriorityChanged();

        void OnValueChanged();

        AZStd::string GetDescriptionOverride();

        int m_sortPriority;

        VariableFlags::Scope m_scope;

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
