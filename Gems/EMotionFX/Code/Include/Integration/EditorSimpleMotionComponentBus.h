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