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

#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/SlotMetadata.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Data/PropertyTraits.h>

#include <Include/ScriptCanvas/Libraries/Core/ForEach.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            //! Provides a node that will iterate over the values in a container
            class ForEach
                : public Node
                , public EndpointNotificationBus::Handler
            {
            public:

                SCRIPTCANVAS_NODE(ForEach);

                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                SlotId GetLoopBreakSlotId() const;

                SlotId GetLoopFinishSlotId() const override;

                SlotId GetLoopSlotId() const override;

                Data::Type GetKeySlotDataType() const;

                SlotId GetKeySlotId() const;

                Data::Type GetValueSlotDataType() const;

                SlotId GetValueSlotId() const;

                bool IsFormalLoop() const override;

                bool IsBreakSlot(const SlotId&) const;

                bool IsOutOfDate(const VersionData& graphVersion) const override;

                

                UpdateResult OnUpdateNode() override;

            protected:
                ExecutionNameMap GetExecutionNameMap() const override;

                void OnInit() override;
                void OnInputSignal(const SlotId&) override;

                bool InitializeLoop();
                void Iterate();
                bool SetPropertySlotData(Datum& atResult, size_t propertyIndex);
                void ResetLoop();

                void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) override;

                void ClearPropertySlots();
                void AddPropertySlotsFromType(const Data::Type& dataType);

                static AZ::Crc32 GetContainerGroupId() { return AZ_CRC("ContainerGroup", 0xb81ed451); }

                SlotId m_sourceSlot;
                AZ::TypeId m_previousTypeId;
                AZStd::vector<Data::PropertyMetadata> m_propertySlots;

                static const size_t k_keySlotIndex;
                static const size_t k_valueSlotIndex;

                size_t m_index;
                size_t m_size;

                bool m_breakCalled;

                Datum m_sourceContainer;
                Datum m_keysVector;
            };
        }
    }
}
