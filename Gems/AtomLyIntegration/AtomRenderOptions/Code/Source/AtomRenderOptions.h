/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Base.h>

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class Name;

    namespace RPI
    {
        class RenderPipeline;
    }

    namespace Render
    {
        //! Get the current default viewport pipeline (in use in the main editor level view)
        //! @return pipeline shared ptr
        RPI::RenderPipelinePtr GetDefaultViewportPipelinePtr();

        //! Find the first render pass matching the name and check its state
        //! @return boolean for enable state. Will be false if pass not found
        bool IsPassEnabled(const RPI::RenderPipeline& pipeline, const Name& name);

        //! Toggle the given render pass name in the given pipeline
        //! @return boolean true on success, false if failed to get the pass
        bool EnablePass(const RPI::RenderPipeline& pipeline, const Name& passName, bool enable);

        //! Get all of the render passes for the given pipeline that can be toggled on and off in the viewport option menu
        void GetViewportOptionsPasses(const RPI::RenderPipeline& pipeline, AZStd::vector<AZ::Name>& passNamesOut);

    } // namespace Render
} // namespace AZ
