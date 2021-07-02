/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

#include <Include/ScriptCanvas/Libraries/Core/ExtractProperty.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            //! A node that takes a Behavior Context object and displays its data components as accessor slots
            class ExtractProperty
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(ExtractProperty);

                static bool VersionConverter(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement);

                Data::Type GetSourceSlotDataType() const;

                //////////////////////////////////////////////////////////////////////////
                // Translation
                AZ::Outcome<DependencyReport, void> GetDependencies() const override;

                PropertyFields GetPropertyFields() const override;

                // Node...
                bool IsOutOfDate(const VersionData& graphVersion) const override;
                ////

                

            protected:
                bool IsPropertySlot(const SlotId& slotId) const;

                void OnInit() override;

                UpdateResult OnUpdateNode() override;

                void OnInputSignal(const SlotId&) override;

                void OnSlotDisplayTypeChanged(const SlotId& slotId, const Data::Type& dataType) override;

                void AddPropertySlots(const Data::Type& type);
                void ClearPropertySlots();

                void RefreshGetterFunctions();

                Data::Type m_dataType;
                AZStd::vector<Data::PropertyMetadata> m_propertyAccounts;

                friend class ExtractPropertyEventHandler;

            private:

                void UpdatePropertyVersion();

                void AddPropertySlots(const Data::Type& dataType, const AZStd::unordered_map<AZStd::string, SlotId>& versionedSlots);
            };
        }
    }
}
