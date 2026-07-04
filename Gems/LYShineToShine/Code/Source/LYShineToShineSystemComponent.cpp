/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LYShineToShineSystemComponent.h"
#include "CanvasUpgrader.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Console/IConsole.h>

namespace LYShineToShine
{
    void LYShineToShineSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LYShineToShineSystemComponent, AZ::Component>()
                ->Version(1);

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LYShineToShineSystemComponent>(
                    "LYShineToShine", "Upgrades old LyShine v1/v2 .uicanvas files to Shine v3 format")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
            }
        }
    }

    void LYShineToShineSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LYShineToShineService"));
    }

    void LYShineToShineSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LYShineToShineService"));
    }

    void LYShineToShineSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // No required services — the conversion code uses raw AzCore serialization APIs
        // and does not depend on Shine being active.
    }

    void LYShineToShineSystemComponent::Activate()
    {
        AZ_TracePrintf("LYShineToShine", "LYShineToShine Gem activated. Use 'upgrade_canvases <path>' to convert old LyShine canvases.\n");
    }

    void LYShineToShineSystemComponent::Deactivate()
    {
    }

} // namespace LYShineToShine

////////////////////////////////////////////////////////////////////////////////////////////////////
// Free-function console commands (registered with short names)
////////////////////////////////////////////////////////////////////////////////////////////////////

void upgrade_canvases(const AZ::ConsoleCommandContainer& args)
{
    if (args.empty())
    {
        AZ_Error("LYShineToShine", false, "Usage: upgrade_canvases <directory_path>");
        return;
    }

    AZ::IO::Path directoryPath(args[0]);
    AZ_TracePrintf("LYShineToShine", "Starting canvas upgrade for directory: %s\n", directoryPath.c_str());

    LYShineToShine::CanvasUpgrader upgrader;
    LYShineToShine::CanvasUpgrader::UpgradeReport report = upgrader.UpgradeDirectory(directoryPath);

    AZ_TracePrintf("LYShineToShine", "=== Canvas Upgrade Report ===\n");
    AZ_TracePrintf("LYShineToShine", "  Files scanned:    %zu\n", report.m_filesScanned);
    AZ_TracePrintf("LYShineToShine", "  Already v4:       %zu\n", report.m_alreadyV3);
    AZ_TracePrintf("LYShineToShine", "  Upgraded (simple): %zu\n", report.m_upgradedSimple);
    AZ_TracePrintf("LYShineToShine", "  Upgraded (slices): %zu\n", report.m_upgradedWithSliceRefs);
    AZ_TracePrintf("LYShineToShine", "  Failed:           %zu\n", report.m_failed);

    for (const auto& failure : report.m_failureDetails)
    {
        AZ_Error("LYShineToShine", false, "  FAILED: %s - %s", failure.first.c_str(), failure.second.c_str());
    }
}
AZ_CONSOLEFREEFUNC(upgrade_canvases, AZ::ConsoleFunctorFlags::Null,
    "Upgrade old LyShine .uicanvas files to Shine v4 format. Usage: upgrade_canvases <directory_path>");

void convert_slices(const AZ::ConsoleCommandContainer& args)
{
    if (args.empty())
    {
        AZ_Error("LYShineToShine", false, "Usage: convert_slices <directory_path>");
        return;
    }

    AZ::IO::Path directoryPath(args[0]);
    AZ_TracePrintf("LYShineToShine", "Starting slice-to-uiprefab conversion for directory: %s\n", directoryPath.c_str());

    LYShineToShine::CanvasUpgrader upgrader;
    LYShineToShine::CanvasUpgrader::SliceConvertReport report = upgrader.ConvertSlicesInDirectory(directoryPath);

    AZ_TracePrintf("LYShineToShine", "=== Slice Conversion Report ===\n");
    AZ_TracePrintf("LYShineToShine", "  Files scanned:   %zu\n", report.m_filesScanned);
    AZ_TracePrintf("LYShineToShine", "  Converted:       %zu\n", report.m_converted);
    AZ_TracePrintf("LYShineToShine", "  Skipped (non-UI): %zu\n", report.m_skippedNonUi);
    AZ_TracePrintf("LYShineToShine", "  Failed:          %zu\n", report.m_failed);

    for (const auto& failure : report.m_failureDetails)
    {
        AZ_Error("LYShineToShine", false, "  FAILED: %s - %s", failure.first.c_str(), failure.second.c_str());
    }
}
AZ_CONSOLEFREEFUNC(convert_slices, AZ::ConsoleFunctorFlags::Null,
    "Convert .slice files to .uiprefab JSON format. Usage: convert_slices <directory_path>");
