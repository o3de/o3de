/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Core/SlotConfigurations.h>
#include <AzCore/RTTI/BehaviorContextUtilities.h>

#include <ScriptCanvas/Grammar/Primitives.h>

namespace ScriptCanvas
{
    typedef AZStd::unordered_set<ScriptCanvas::Data::Type> DataTypeSet;

    typedef AZStd::unordered_map<size_t, DataTypeSet> DataSetIndexMapping;
    typedef AZStd::unordered_map<size_t, ScriptCanvas::Data::Type> DataIndexMapping;

    // Information needed to deal with partial OverloadSelection.
    //
    // Works in conjunction with the OverloadConfiguration and essentially represents
    // a stripped down version of the configuration data based on the list of available indexes.
    struct OverloadSelection
    {
        bool IsAmbiguous() const;
        AZ::u32 GetActiveIndex() const;

        const DataTypeSet& FindPossibleInputTypes(size_t index) const;
        const DataTypeSet& FindPossibleOutputTypes(size_t index) const;

        ScriptCanvas::Data::Type GetInputDisplayType(size_t index) const;
        ScriptCanvas::Data::Type GetOutputDisplayType(size_t index) const;

        DataSetIndexMapping           m_inputDataTypes;
        DataSetIndexMapping           m_outputDataTypes;
        AZStd::unordered_set<AZ::u32> m_availableIndexes;
    };

    // Stores the overall configuration for an overload for a method.
    struct OverloadConfiguration
    {
        void Clear();
        void CopyData(const OverloadConfiguration& overloadConfiguration);

        // Setups up all of the method/class overloads.
        void SetupOverloads(const AZ::BehaviorMethod* behaviorMethod, const AZ::BehaviorClass* behaviorClass, AZ::VariantOnThis variantOnThis = AZ::VariantOnThis::Yes);

        // Sets he DynamicDataType for each slot based on the current overloads.
        void DetermineInputOutputTypes();

        // Returns the set of available indexes that represent which overloads are stil viable for the Input/Output data mapping.
        AZStd::unordered_set<AZ::u32> GenerateAvailableIndexes(const DataIndexMapping& inputMapping, const DataIndexMapping& outputMapping) const;

        // Helper methods fo populating the overload selection based either on a list of indexes, or the Input/Output data mapping
        void PopulateOverloadSelection(OverloadSelection& overloadSelection, const DataIndexMapping& inputMapping, const DataIndexMapping& outputMapping) const;
        void PopulateOverloadSelection(OverloadSelection& overloadSelection, const AZStd::unordered_set<AZ::u32>& availableIndexes) const;

        // Helper function for populating a DataSetIndexMapping for either Input or Output.
        void PopulateDataIndexMapping(const AZStd::unordered_set<AZ::u32>& availableIndexes, ConnectionType connectionType, DataSetIndexMapping& dataIndexMapping) const;

        AZStd::vector<Grammar::FunctionPrototype> m_prototypes;
        AZStd::vector<AZStd::pair<const AZ::BehaviorMethod*, const AZ::BehaviorClass*>> m_overloads;

        AZStd::unordered_map<size_t, DynamicDataType> m_inputDataTypes;
        AZStd::unordered_map<size_t, DynamicDataType> m_outputDataTypes;

        AZ::OverloadVariance m_overloadVariance;
    };

    // Interface for talking back to the source node to confirm information about the overloads.
    class OverloadContractInterface
    {
    public:

        virtual ~OverloadContractInterface() = default;

        virtual AZ::Outcome<void, AZStd::string> IsValidInputType(size_t index, const Data::Type& dataType) = 0;
        virtual const DataTypeSet& FindPossibleInputTypes(size_t index) const = 0;

        virtual AZ::Outcome<void, AZStd::string> IsValidOutputType(size_t index, const Data::Type& dataType) = 0;
        virtual const DataTypeSet& FindPossibleOutputTypes(size_t index) const = 0;
    };

    // General contract that deals with Overloaded methods. Needs to be configured by the owning node to hook
    // up the interface, and supply other data.
    class OverloadContract
        : public Contract
    {
    public:
        AZ_CLASS_ALLOCATOR(OverloadContract, AZ::SystemAllocator);
        AZ_RTTI(OverloadContract, "{45622160-13C5-46E3-94D9-AE2EAFE6AC64}", Contract);

        static void Reflect(AZ::ReflectContext* reflection);

        OverloadContract() = default;
        ~OverloadContract() override = default;

        void ConfigureContract(OverloadContractInterface* overloadInterface, size_t index, ConnectionType connectionType);

    protected:

        OverloadContractInterface*      m_overloadInterface = nullptr;
        size_t                          m_methodIndex = 0;
        ConnectionType                  m_connectionType = ConnectionType::Input;

        AZ::Outcome<void, AZStd::string> OnEvaluate(const Slot& sourceSlot, const Slot& targetSlot) const override;
        AZ::Outcome<void, AZStd::string> OnEvaluateForType(const Data::Type& dataType) const override;
    };
}
