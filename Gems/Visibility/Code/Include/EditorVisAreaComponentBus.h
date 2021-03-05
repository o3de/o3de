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

#include "VisAreaComponentBus.h"

namespace Visibility
{
    /// Request bus for the EditorVisAreaComponent.
    class EditorVisAreaComponentRequests
        : public VisAreaComponentRequests
        , public AZ::VariableVertices<AZ::Vector3>
    {
    public:
        virtual void SetHeight(float height) = 0;
        virtual void SetDisplayFilled(bool filled) = 0;
        virtual void SetAffectedBySun(bool affectedBySun) = 0;
        virtual void SetViewDistRatio(float viewDistRatio) = 0;
        virtual void SetOceanIsVisible(bool oceanVisible) = 0;
        virtual void UpdateVisAreaObject() = 0;
    };

    /// Type to inherit to implement EditorVisAreaComponentRequests.
    using EditorVisAreaComponentRequestBus = AZ::EBus<EditorVisAreaComponentRequests, AZ::ComponentBus>;

    /// Notification bus for the EditorVisAreaComponent.
    class EditorVisAreaComponentNotifications
        : public AZ::ComponentBus
        , public AZ::VertexContainerNotificationInterface<AZ::Vector3>
    {
    public:
        /// Called when a new vertex is added to the vis area.
        void OnVertexAdded(size_t /*index*/) override {}

        /// Called when a vertex is removed from the vis area.
        void OnVertexRemoved(size_t /*index*/) override {}

        /// Called when a vertex is updated.
        void OnVertexUpdated(size_t /*index*/) override {}

        /// Called when all vertices on the vis area are set.
        void OnVerticesSet(const AZStd::vector<AZ::Vector3>& /*vertices*/) override {}

        /// Called when all vertices are removed from the vis area.
        void OnVerticesCleared() override {}

    protected:
        ~EditorVisAreaComponentNotifications() = default;
    };

    /// Type to inherit to implement EditorVisAreaComponentNotifications.
    using EditorVisAreaComponentNotificationBus = AZ::EBus<EditorVisAreaComponentNotifications>;

} // namespace Visibility