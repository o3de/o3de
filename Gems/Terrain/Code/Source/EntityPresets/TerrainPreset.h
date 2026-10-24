/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>

namespace Terrain
{
    //! Builds a working terrain setup in one action.
    //!
    //! Written as code rather than preset data because it adds level components, creates a parent and
    //! child entity, and points them at each other, which presets cannot express. It attaches to the
    //! editor's "Create Preset" menu through EntityPresetsIdentifiers.
    namespace TerrainPreset
    {
        //! How much to build. Each variant adds to the one before.
        enum class Variant
        {
            //! A terrain region with noise driving its height.
            Simple,

            //! The same terrain under an entity with a Landscape Canvas component, so it opens as a node graph.
            Landscape,

            //! Landscape plus a vegetation area and the level's vegetation settings. The asset list is
            //! left empty, so nothing is planted until a mesh or prefab is added.
            LandscapeWithVegetation
        };

        //! Create the terrain as one undo step, adding any missing level components.
        //! @return The spawner entity, or an invalid id if it could not be built.
        AZ::EntityId Create(Variant variant);

        //! Register the Terrain actions. Call from OnActionRegistrationHook.
        void RegisterActions();

        //! Register the Terrain submenu. Call from OnMenuRegistrationHook.
        void RegisterMenus();

        //! Attach the Terrain submenu to "Create Preset". Call from OnMenuBindingHook.
        void BindMenus();
    } // namespace TerrainPreset
} // namespace Terrain
