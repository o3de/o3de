/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/DOM/DomComparison.h>
#include <AzCore/DOM/DomUtils.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

AZ_CVAR(
    bool,
    ed_debugDocumentPropertyEditorUpdates,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "If set, enables debugging change change notifications on DocumentPropertyEditor adapters by validating their contents match their "
    "emitted patches");

namespace AZ::DocumentPropertyEditor
{
    Dom::Value DocumentAdapter::GetContents() const
    {
        if (m_cachedContents.IsNull())
        {
            m_cachedContents = GenerateContents();
        }
        return m_cachedContents;
    }

    void DocumentAdapter::ConnectResetHandler(ResetEvent::Handler& handler)
    {
        handler.Connect(m_resetEvent);
    }

    void DocumentAdapter::ConnectChangedHandler(ChangedEvent::Handler& handler)
    {
        handler.Connect(m_changedEvent);
    }

    void DocumentAdapter::SetRouter(RoutingAdapter* /*router*/, const Dom::Path& /*route*/)
    {
        // By default, setting a router is a no-op, this only matters for nested routing adapters
    }

    bool DocumentAdapter::IsDebugModeEnabled()
    {
        return ed_debugDocumentPropertyEditorUpdates;
    }

    void DocumentAdapter::SetDebugModeEnabled(bool enableDebugMode)
    {
        ed_debugDocumentPropertyEditorUpdates = enableDebugMode;
    }

    void DocumentAdapter::NotifyResetDocument(DocumentResetType resetType)
    {
        if (resetType == DocumentResetType::HardReset || m_cachedContents.IsNull())
        {
            // If it's a hard reset, or we don't have any lazily cached contents, just send the reset signal.
            m_cachedContents.SetNull();
            m_resetEvent.Signal();
        }
        else
        {
            // Otherwise, compare the new contents to the old contents and send the difference as patches.
            Dom::Value newContents = GenerateContents();
            Dom::PatchUndoRedoInfo patches = Dom::GenerateHierarchicalDeltaPatch(m_cachedContents, newContents);
            m_cachedContents = newContents;
            m_changedEvent.Signal(patches.m_forwardPatches);
        }
    }

    void DocumentAdapter::NotifyContentsChanged(const AZ::Dom::Patch& patch)
    {
        if (!m_cachedContents.IsNull())
        {
            Dom::PatchOutcome outcome = patch.ApplyInPlace(m_cachedContents);
            if (!outcome.IsSuccess())
            {
                AZ_Warning("DPE", false, "DocumentAdapter::NotifyContentsChanged: Failed to apply DOM patches: %s", outcome.GetError().c_str());
                m_cachedContents.SetNull();
            }
            else if (IsDebugModeEnabled())
            {
                Dom::Value actualContents = GenerateContents();
                Dom::Utils::ComparisonParameters comparisonParameters;
                // Because callbacks are often dynamically generated as opaque attributes, only do type-level comparison when validating patches
                comparisonParameters.m_treatOpaqueValuesOfSameTypeAsEqual = true;
                [[maybe_unused]] const bool valuesMatch = Dom::Utils::DeepCompareIsEqual(actualContents, m_cachedContents, comparisonParameters);
                AZ_Warning("DPE", valuesMatch, "DocumentAdapter::NotifyContentsChanged: DOM patches applied, but the new model contents don't match the result of GenerateContents");
            }
        }
        m_changedEvent.Signal(patch);
    }
} // namespace AZ::DocumentPropertyEditor
