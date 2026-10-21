/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>

namespace AZ::SceneAPI::SceneBuilder
{
    //! Stores the string-value from a source scent asset's node; scene builders will be able to access
    //! the key-value pairs to tweak the scene manifest, create special rules, and produce custom assets
    //!
    //! The keys are all AZStd::string
    //! The supported value types are AZStd::string, bool, int32_t, int64_t, float, and double
    class AssImpCustomPropertyImporter
        : public SceneCore::LoadingComponent
    {
    public:
        AZ_COMPONENT(AssImpCustomPropertyImporter, "{BEFF2CA0-CB11-43FF-8BF9-1A58E133186A}", SceneCore::LoadingComponent);

        AssImpCustomPropertyImporter();
        ~AssImpCustomPropertyImporter() override = default;

        static void Reflect(ReflectContext* context);

        Events::ProcessingResult ImportCustomProperty(AssImpSceneNodeAppendedContext& context);
    };
}
