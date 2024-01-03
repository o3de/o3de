/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRenderOptions.h"

#include <AzCore/Name/Name.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>

namespace AZ::Render
{
    RPI::RenderPipelinePtr GetDefaultViewportPipelinePtr()
    {
        RPI::RPISystemInterface* rpiSystem = RPI::RPISystemInterface::Get();
        if (!rpiSystem)
        {
            return nullptr;
        }

        AzFramework::NativeWindowHandle windowHandle = RPI::ViewportContextRequests::Get()->GetDefaultViewportContext()->GetWindowHandle();
        if (!windowHandle)
        {
            return nullptr;
        }

        return rpiSystem->GetRenderPipelineForWindow(windowHandle);
    }

    AZStd::optional<bool> IsPassEnabled(const Name& name)
    {
        RPI::RenderPipelinePtr pipeline = GetDefaultViewportPipelinePtr();
        if (!pipeline)
        {
            return AZStd::nullopt;
        }

        RPI::Pass* pass = RPI::PassSystemInterface::Get()->FindFirstPass(RPI::PassFilter::CreateWithPassName(name, pipeline.get()));
        if (!pass)
        {
            return AZStd::nullopt;
        }

        return pass->IsEnabled();
    }

    bool EnableTAA(bool enable)
    {
        RPI::RenderPipelinePtr pipeline = GetDefaultViewportPipelinePtr();
        if (!pipeline)
        {
            return false;
        }

        bool found = false;
        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(Name("TaaPass"), pipeline.get());
        RPI::PassSystemInterface::Get()->ForEachPass(
            passFilter,
            [enable, &found](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
            {
                found = true;
                pass->SetEnabled(enable);
                return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
            });

        return found;
    }
} // namespace AZ::Render
