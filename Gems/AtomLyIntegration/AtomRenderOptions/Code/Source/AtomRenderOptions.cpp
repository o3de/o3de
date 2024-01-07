/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AtomRenderOptions.h"

#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/set.h>
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

    static bool IsToolExposedPass(const Name& passName)
    {
        // Temporary solution, need to introduce this concept in RPI::Pass class as a bool getter
        static AZStd::set<Name::Hash> toolExposedPasses{ Name("Shadows").GetHash(),
                                                         Name("TaaPass").GetHash(),
                                                         Name("ReflectionsPass").GetHash(),
                                                         Name("Ssao").GetHash(),
                                                         Name("TransparentPass").GetHash() };
        return toolExposedPasses.contains(passName.GetHash());
    }

    void GetToolExposedPasses(const RPI::RenderPipeline& pipeline, AZStd::vector<AZ::Name>& passNamesOut)
    {
        const auto& rootPass = pipeline.GetRootPass();
        if (!rootPass)
        {
            return;
        }

        AZStd::queue<const RPI::Pass*> passes;
        passes.push(rootPass.get());

        while (!passes.empty())
        {
            const auto* pass = passes.front();
            passes.pop();

            if (IsToolExposedPass(pass->GetName()))
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
