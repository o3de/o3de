/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>

namespace PhysX
{
    ///
    /// The type ID of runtime component PhysX::TerrainComponent.
    ///
    static const AZ::TypeId TerrainComponentTypeId("{51D9CFD0-842D-4263-880C-A38171FC2D2F}");

    ///
    /// The type ID of editor component PhysX::EditorTerrainComponent.
    ///
    static const AZ::TypeId EditorTerrainComponentTypeId("{84CC2375-927A-4DB9-8040-10768BDBCB44}");

    ///
    /// The type ID of runtime component PhysX::ForceRegionComponent.
    ///
    static const AZ::TypeId ForceRegionComponentTypeId("{7E374A32-6BED-476E-88B8-6DB598C9563B}");

    ///
    /// The type ID of editor component PhysX::EditorForceRegionComponent.
    ///
    static const AZ::TypeId EditorForceRegionComponentTypeId("{AAD18665-8EBC-4CAF-8491-524F054463BC}");

    ///
    /// The type ID of runtime component PhysX::StaticRigidBodyComponent.
    ///
    static const AZ::TypeId StaticRigidBodyComponentTypeId("{A2CCCD3D-FB31-4D65-8DCD-2CD7E1D09538}");
}
