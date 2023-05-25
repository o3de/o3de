/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorLerpNodeable.h>
#include <ScriptCanvas/Core/Contracts/MethodOverloadContract.h>

namespace AZ
{
    class BehaviorClass;
    class BehaviorContext;
    class BehaviorMethod;
}

namespace ScriptCanvas
{
    namespace Nodes
    {
        class NodeableNodeOverloaded
            : public NodeableNode
            , protected NodeNotificationsBus::Handler
        {
        private:

            enum Version
            {
                Original = 0,
                DataDrivingOverloads,

                // add version label above
                Current
            };

            static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            class NodeableMethodOverloadContractInterface
                : public OverloadContractInterface
            {
            public:
                AZ_CLASS_ALLOCATOR(NodeableMethodOverloadContractInterface, AZ::SystemAllocator);

                NodeableMethodOverloadContractInterface(NodeableNodeOverloaded& nodeableOverloaded, size_t methodIndex);
                ~NodeableMethodOverloadContractInterface() override = default;

                AZ::Outcome<void, AZStd::string> IsValidInputType(size_t index, const Data::Type& dataType) override;
                const DataTypeSet& FindPossibleInputTypes(size_t index) const override;

                AZ::Outcome<void, AZStd::string> IsValidOutputType(size_t index, const Data::Type& dataType) override;
                const DataTypeSet& FindPossibleOutputTypes(size_t index) const override;

            private:

                NodeableNodeOverloaded& m_nodeableOverloaded;
                size_t m_methodIndex;
            };

            friend class NodeableMethodOverloadContractInterface;

        public:
            AZ_COMPONENT(NodeableNodeOverloaded, "{C5C21008-F0B8-4FC8-843E-9C5C50B9DCDC}", NodeableNode);

            static void Reflect(AZ::ReflectContext* reflectContext);

            ~NodeableNodeOverloaded() override;

            bool IsNodeableListEmpty() const;
            void SetNodeables(AZStd::vector<AZStd::unique_ptr<Nodeable>>&& nodeables);

            // Node
            void OnInit() override;
            void OnConfigured() override;
            void OnPostActivate() override;
            
            void OnSlotDisplayTypeChanged(const SlotId& slotId, const ScriptCanvas::Data::Type& slotType) override;

            void OnDynamicGroupTypeChangeBegin(const AZ::Crc32& dynamicGroup) override;
            void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) override;

            Data::Type FindFixedDataTypeForSlot(const Slot& slot) const override;
            ////

            // EndpointNotificationBus
            void OnEndpointConnected(const Endpoint& targetEndpoint) override;
            void OnEndpointDisconnected(const Endpoint& targetEndpoint) override;
            ////

        protected:

            void ConfigureNodeableOverloadConfigurations();

            Grammar::FunctionPrototype GetCurrentInputPrototype(const SlotExecution::In& in) const;

            virtual AZStd::vector<AZStd::unique_ptr<Nodeable>> GetInitializationNodeables() const;

            const OverloadSelection& GetOverloadSelection(size_t methodIndex) const;

            void CheckHasSingleOuts();

            void UpdateSlotDisplay();

            void ConfigureContracts();

            void RefreshAvailableNodeables(const bool checkForConnections = true);
            void FindDataIndexMappings(size_t methodIndex, DataIndexMapping& inputMapping, DataIndexMapping& outputMapping, bool checkForConnections);

            AZ::Outcome<void, AZStd::string> IsValidConfiguration(size_t methodIndex, const DataIndexMapping& inputMapping, const DataIndexMapping& outputMapping);
            AZ::Outcome<void, AZStd::string> CheckOverloadDataTypes(const AZStd::unordered_set<AZ::u32>& availableIndexes, size_t methodIndex);

            // More Specific OverloadContract overloads
            AZ::Outcome<void, AZStd::string> IsValidInputType(size_t methodIndex, size_t index, const Data::Type& dataType);
            const DataTypeSet& FindPossibleInputTypes(size_t methodIndex, size_t index) const;

            AZ::Outcome<void, AZStd::string> IsValidOutputType(size_t methodIndex, size_t index, const Data::Type& dataType);
            const DataTypeSet& FindPossibleOutputTypes(size_t methodIndex, size_t index) const;
            ////

        protected:

            void OnReconfigurationBegin() override;
            void OnReconfigurationEnd() override;

            void OnSanityCheckDisplay() override;

        private:

            bool m_isCheckingForDataTypes = false;
            bool m_updatingDynamicGroups = false;
            bool m_updatingDisplayTypes = false;
            bool m_isTypeChecking = false;

            bool m_slotTypeChange = false;

            AZStd::vector<AZStd::unique_ptr<Nodeable>> m_nodeables;

            // List of which nodeable definitions are valid for the given set of inputs.
            AZStd::unordered_set<AZ::u32> m_availableNodeables;

            // Nodeables can have 1 or more functions. Each Overload Configuration represents a single function that is the aggregate of all the available nodeables.
            AZStd::vector< OverloadConfiguration > m_methodConfigurations;

            // Nodeables can have 1 or more functions. Each OverloadSelection represents a single function that is based on the currently available nodeables.
            AZStd::vector< OverloadSelection > m_methodSelections;

            // Helper interfaces to shim in the methodIndex to the OperatorContract.
            AZStd::vector< NodeableMethodOverloadContractInterface* > m_methodOverloadContractInterface;
        };

    }
}
