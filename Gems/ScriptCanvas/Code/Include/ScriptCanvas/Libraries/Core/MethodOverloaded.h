/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Method.h"

#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <ScriptCanvas/Core/Contracts/MethodOverloadContract.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/SlotMetadata.h>
#include <ScriptCanvas/Grammar/Primitives.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    namespace Grammar
    {
        struct FunctionPrototype;
    }

    namespace Nodes
    {
        namespace Core
        {
            class MethodOverloaded
                : public Method
                , public OverloadContractInterface
            {
            private:
                friend class SerializeContextReadWriteHandler<MethodOverloaded>;

            public:
                static void Reflect(AZ::ReflectContext* reflectContext);

                AZ_COMPONENT(MethodOverloaded, "{C1E3C9D0-42E3-4D00-AE73-2A881E7E76A8}", Method);
                
                MethodOverloaded() = default;
                ~MethodOverloaded() override = default;

                // Node
                void OnInit() override;
                void OnPostActivate() override;
                void OnSlotDisplayTypeChanged(const SlotId& slotId, const ScriptCanvas::Data::Type& slotType) override;

                Data::Type FindFixedDataTypeForSlot(const Slot& slot) const override;

                AZ::Outcome<AZStd::string, void> GetFunctionCallName(const Slot* slot) const override;
                ////

                // EndpointNotificationBus
                void OnEndpointDisconnected(const Endpoint& targetEndpoint) override;
                ////

                // Method
                void InitializeMethod(const MethodConfiguration& config) override;
                SlotId AddMethodInputSlot(const MethodConfiguration& config, size_t argumentIndex) override;

                void OnInitializeOutputPost(const MethodOutputConfig& config) override;
                void OnInitializeOutputPre(MethodOutputConfig& config) override;

                DynamicDataType GetOverloadedOutputType(size_t resultIndex) const override;
                bool IsMethodOverloaded() const override { return true; }
                ////

                // OverloadContractInterface
                AZ::Outcome<void, AZStd::string> IsValidInputType(size_t index, const Data::Type& dataType) override;
                const DataTypeSet& FindPossibleInputTypes(size_t index) const override;

                AZ::Outcome<void, AZStd::string> IsValidOutputType(size_t index, const Data::Type& dataType) override;
                const DataTypeSet& FindPossibleOutputTypes(size_t index) const override;
                ////

            protected:

                void OnReconfigurationBegin() override;
                void OnReconfigurationEnd() override;

                void OnSanityCheckDisplay() override;

            private:

                bool IsAmbiguousOverload() const;
                int GetActiveIndex() const;

                // \todo make execution thread sensitive, which can then support generic programming
                Grammar::FunctionPrototype GetInputSignature() const;

                // SerializeContextReadWriteHandler
                void OnReadBegin();
                void OnReadEnd();

                void OnWriteBegin();
                void OnWriteEnd();
                ////

                void SetupMethodData(const AZ::BehaviorMethod* lookupMethod, const AZ::BehaviorClass* lookupClass);
                void ConfigureContracts();

                void RefreshActiveIndexes(bool checkForConnections = true, bool adjustSlots = false);
                void FindDataIndexMappings(DataIndexMapping& inputMapping, DataIndexMapping& outputMapping, bool checkForConnections) const;

                void UpdateSlotDisplay();

                AZ::Outcome<void, AZStd::string> IsValidConfiguration(const DataIndexMapping& inutMapping, const DataIndexMapping& outputMapping) const;                

                OverloadConfiguration m_overloadConfiguration;

                AZStd::vector<SlotId> m_orderedInputSlotIds;
                AZStd::vector<SlotId> m_outputSlotIds;

                bool m_isCheckingForDataTypes = false;
                bool m_updatingDisplay = false;
                bool m_isTypeChecking = false;

                OverloadSelection m_overloadSelection;
            };
        }
    }
}
