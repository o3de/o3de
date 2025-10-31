/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

            private:
                static const size_t k_keySlotIndex;
                static const size_t k_valueSlotIndex;

                static AZ::Crc32 GetContainerGroupId() { return AZ_CRC_CE("ContainerGroup"); }

                void AddPropertySlotsFromType(const Data::Type& dataType);

                void ClearPropertySlots();

                ExecutionNameMap GetExecutionNameMap() const override;

                void OnInit() override;

                void OnDynamicGroupDisplayTypeChanged(const AZ::Crc32& dynamicGroup, const Data::Type& dataType) override;
                                                
                SlotId m_sourceSlot;
                AZ::TypeId m_previousTypeId;
                AZStd::vector<Data::PropertyMetadata> m_propertySlots;
            };
        }
    }
}
