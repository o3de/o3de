/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
