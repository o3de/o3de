/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>

namespace ScriptCanvas
{
    // #scriptcanvas_component_extension
    // A place holder identifier for the AZ::Entity that owns the graph.
    // The actual value in each location initialized to GraphOwnerId is populated with the owning entity at editor-time, Asset
    // Processor-time, or runtime, as soon as the owning entity is known.
    using GraphOwnerIdType = AZ::EntityId;
    static const GraphOwnerIdType GraphOwnerId = AZ::EntityId(0xacedc0de);

    // \note Deprecated
    // A place holder identifier for unique runtime graph on Entity that is running more than one instance of the same graph.
    // This allows multiple instances of the same graph to be addressed individually on the same entity.
    // The actual value in each location initialized to UniqueId is populated at run-time.
    using RuntimeIdType = AZ::EntityId;
    static const RuntimeIdType UniqueId = AZ::EntityId(0xfee1baad);
} // namespace ScriptCanvas
