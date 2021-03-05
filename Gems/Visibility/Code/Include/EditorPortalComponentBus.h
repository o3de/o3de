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

#include <AzCore/Component/ComponentBus.h>

#include "PortalComponentBus.h"

namespace Visibility
{
    /// Request bus for the EditorPortalComponent.
    class EditorPortalRequests
        : public PortalRequests
        , public AZ::FixedVertices<AZ::Vector3>
    {
    public:
        virtual void SetHeight(float height) = 0;
        virtual void SetDisplayFilled(bool filled) = 0;
        virtual void SetAffectedBySun(bool affectedBySun) = 0;
        virtual void SetViewDistRatio(float viewDistRatio) = 0;
        virtual void SetSkyOnly(bool skyOnly) = 0;
        virtual void SetOceanIsVisible(bool oceanVisible) = 0;
        virtual void SetUseDeepness(bool useDeepness) = 0;
        virtual void SetDoubleSide(bool doubleSided) = 0;
        virtual void SetLightBlending(bool lightBending) = 0;
        virtual void SetLightBlendValue(float lightBendAmount) = 0;
        virtual void UpdatePortalObject() = 0;

    protected:
        ~EditorPortalRequests() = default;
    };

    /// Type to inherit to implement EditorPortalComponentRequests
    using EditorPortalRequestBus = AZ::EBus<EditorPortalRequests, AZ::ComponentBus>;

    /// Editor notification bus for EditorPortalComponent.
    class EditorPortalNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual void OnVerticesChangedInspector() {}

    protected:
        ~EditorPortalNotifications() = default;
    };

    /// Type to inherit to implement EditorPortalNotifications.
    using EditorPortalNotificationBus = AZ::EBus<EditorPortalNotifications>;

} // namespace Visibility