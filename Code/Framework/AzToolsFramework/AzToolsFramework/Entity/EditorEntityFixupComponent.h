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

#include <AzCore/Slice/SliceBus.h>

namespace AzToolsFramework
{
    /**
     * System component which fixes up entity and component data in ways that
     * can't be dealt with by version-converters or component Init() functions.
     */
    class EditorEntityFixupComponent
        : public AZ::Component
        , protected AZ::SliceAssetSerializationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(EditorEntityFixupComponent, "{7D00B08D-DC26-465A-949B-8DAC7787E607}");
        static void Reflect(AZ::ReflectContext* context);

    protected:
        void Activate() override;
        void Deactivate() override;

        ////////////////////////////////////////////////////////////////////////
        // SliceAssetSerializationNotificationBus
        void OnSliceEntitiesLoaded(const AZStd::vector<AZ::Entity*>& entities) override;
        ////////////////////////////////////////////////////////////////////////
    };
} // namespace AzToolsFramework