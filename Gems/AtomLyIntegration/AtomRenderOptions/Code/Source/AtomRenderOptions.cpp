/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRenderOptions.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/span.h>

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RenderPipeline.h>
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

    bool IsPassEnabled(const RPI::RenderPipeline& pipeline, const Name& name)
    {
        const RPI::Pass* pass = RPI::PassSystemInterface::Get()->FindFirstPass(RPI::PassFilter::CreateWithPassName(name, &pipeline));
        return pass ? pass->IsEnabled() : false;
    }

    bool EnablePass(const RPI::RenderPipeline& pipeline, const Name& passName, bool enable)
    {
        bool found = false;
        RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassName(passName, &pipeline);
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

    static void GetAllowedPassNamesInViewportOptionsMenu(AZStd::unordered_set<Name>& passNamesOut)
    {
        if (const auto* settingsRegistry = SettingsRegistry::Get(); settingsRegistry)
        {
            [[maybe_unused]] const bool found =
                settingsRegistry->GetObject(passNamesOut, "/O3DE/AtomRenderOptions/PassNamesInViewportOptionsMenu");
            AZ_Warning("AtomRenderOptions", found, "No AtomRenderOptions settings found from the settings registry");
        }
    }

    void GetViewportOptionsPasses(const RPI::RenderPipeline& pipeline, AZStd::vector<AZ::Name>& passNamesOut)
    {
        const auto& rootPass = pipeline.GetRootPass();
        if (!rootPass)
        {
            return;
        }

        AZStd::unordered_set<Name> allowedPassNames;
        GetAllowedPassNamesInViewportOptionsMenu(allowedPassNames);

        AZStd::queue<const RPI::Pass*> passes;
        passes.push(rootPass.get());

        while (!passes.empty())
        {
            const auto* pass = passes.front();
            passes.pop();

            if (allowedPassNames.contains(pass->GetName()))
            {
                passNamesOut.push_back(pass->GetName());
            }

            const auto* asParent = pass->AsParent();
            if (!asParent)
            {
                continue;
            }

            for (const auto& child : asParent->GetChildren())
            {
                passes.push(child.get());
            }
        }
    }
} // namespace AZ::Render
