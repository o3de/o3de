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

#include "OccluderAreaComponentBus.h"

namespace Visibility
{
    /// Request bus for the EditorOccluderComponent.
    class EditorOccluderAreaRequests
        : public OccluderAreaRequests
        , public AZ::FixedVertices<AZ::Vector3>
    {
    public:
        virtual void SetDisplayFilled(bool displayFilled) = 0;
        virtual void SetCullDistRatio(float cullDistRatio) = 0;
        virtual void SetUseInIndoors(bool inDoors) = 0;
        virtual void SetDoubleSide(bool doubleSided) = 0;
        virtual void UpdateOccluderAreaObject() = 0;

    protected:
        ~EditorOccluderAreaRequests() = default;
    };

    /// Type to inherit to implement EditorOccluderAreaComponentRequests
    using EditorOccluderAreaRequestBus = AZ::EBus<EditorOccluderAreaRequests, AZ::ComponentBus>;

    /// Editor notification bus for EditorOccluderAreaComponent.
    class EditorOccluderAreaNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual void OnVerticesChangedInspector() {}

    protected:
        ~EditorOccluderAreaNotifications() = default;
    };

    /// Type to inherit to implement EditorOccluderAreaNotifications.
    using EditorOccluderAreaNotificationBus = AZ::EBus<EditorOccluderAreaNotifications>;

} // namespace Visibility