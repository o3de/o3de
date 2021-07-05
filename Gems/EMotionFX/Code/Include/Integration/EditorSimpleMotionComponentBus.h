/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class EditorSimpleMotionComponentRequests
            : public AZ::ComponentBus
        {
        public:
            virtual void SetPreviewInEditor(bool enable) = 0;
            virtual bool GetPreviewInEditor() const = 0;
            virtual float GetAssetDuration(const AZ::Data::AssetId& assetId) = 0;
        };
        using EditorSimpleMotionComponentRequestBus = AZ::EBus<EditorSimpleMotionComponentRequests>;
    }
}
