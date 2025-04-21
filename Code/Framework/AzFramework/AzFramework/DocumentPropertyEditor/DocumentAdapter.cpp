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
#include <AzCore/Name/NameDictionary.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/ExpanderSettings.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>

AZ_CVAR(
    bool,
    ed_debugDocumentPropertyEditorUpdates,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate,
    "If set, enables debugging change notifications on DocumentPropertyEditor adapters by validating their contents match their "
    "emitted patches");

namespace AZ::DocumentPropertyEditor
{
    const AZ::Name BoundAdapterMessage::s_typeField = AZ::Name::FromStringLiteral("$type", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name BoundAdapterMessage::s_adapterField = AZ::Name::FromStringLiteral("adapter", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name BoundAdapterMessage::s_messageNameField = AZ::Name::FromStringLiteral("messageName", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name BoundAdapterMessage::s_messageOriginField = AZ::Name::FromStringLiteral("messageOrigin", AZ::Interface<AZ::NameDictionary>::Get());
    const AZ::Name BoundAdapterMessage::s_contextDataField = AZ::Name::FromStringLiteral("contextData", AZ::Interface<AZ::NameDictionary>::Get());

    Dom::Value DocumentAdapter::GetContents() const
    {
        if (m_cachedContents.IsNull())
        {
            m_cachedContents = const_cast<DocumentAdapter*>(this)->GenerateContents();
            // Create a placeholder with a failure message if the adapter fails to produce contents
            if (m_cachedContents.IsNull())
            {
                m_cachedContents = Dom::Value::CreateNode(GetNodeName<Nodes::Adapter>());
                Dom::Value row = Dom::Value::CreateNode(GetNodeName<Nodes::Row>());
                Dom::Value label = Dom::Value::CreateNode(GetNodeName<Nodes::Label>());
                label[Nodes::Label::Value.GetName()] = Dom::Value("Failed to create DocumentAdapter, please report this as a bug!", false);
                row.ArrayPushBack(AZStd::move(label));
                m_cachedContents.ArrayPushBack(AZStd::move(row));
            }
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

    void DocumentAdapter::ConnectMessageHandler(MessageEvent::Handler& handler)
    {
        handler.Connect(m_messageEvent);
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

    bool DocumentAdapter::IsRow(const Dom::Value& domValue)
    {
        return (domValue.IsNode() && domValue.GetNodeName() == Dpe::GetNodeName<Dpe::Nodes::Row>());
    }

    bool DocumentAdapter::IsEmpty()
    {
        const auto& contents = GetContents();
        return contents.IsArrayEmpty();
    }

    ExpanderSettings* DocumentAdapter::CreateExpanderSettings(
        DocumentAdapter* referenceAdapter, const AZStd::string& settingsRegistryKey, const AZStd::string& propertyEditorName)
    {
        return new ExpanderSettings(referenceAdapter, settingsRegistryKey, propertyEditorName);
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

            Dom::DeltaPatchGenerationParameters patchGenerationParams;

            // at most allow non-nested row replacement, a diff to replace the whole tree is almost certainly inefficient for this architecture
            patchGenerationParams.m_allowReplacement = [](const Dom::Value& before, [[maybe_unused]] const Dom::Value& after)
            {
                if (before.GetType() == Dom::Type::Node)
                {
                    auto beforeName = before.GetNodeName();
                    auto rowName = AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Row>();
                    if (beforeName == AZ::Dpe::GetNodeName<AZ::Dpe::Nodes::Adapter>())
                    {
                        // don't allow full adapter replacement
                        return false;
                    }
                    else if (beforeName == rowName)
                    {
                        // don't allow nested row replacement, it's too broad and likely inefficient
                        for (auto childIter = before.ArrayBegin(), endIter = before.ArrayEnd(); childIter != endIter; ++childIter)
                        {
                            if (childIter->GetNodeName() == rowName)
                            {
                                return false;
                            }
                        }
                    }
                }
                return true;
            };

            // Generate denormalized paths instead of EndOfArray entries (this is required by ChangedEvent)
            patchGenerationParams.m_generateDenormalizedPaths = true;
            Dom::PatchUndoRedoInfo patches = Dom::GenerateHierarchicalDeltaPatch(m_cachedContents, newContents, patchGenerationParams);
            m_cachedContents = newContents;
            m_changedEvent.Signal(patches.m_forwardPatches);
        }
    }

    void DocumentAdapter::NotifyContentsChanged(const AZ::Dom::Patch& patch)
    {
        const Dom::Patch* appliedPatch = &patch;
        // This is used as a scratch buffer if we need to denormalize the path before we emit it.
        // This lets the DPE and proxy adapters listen for patches that have valid indices instead of EndOfArray entries.
        Dom::Patch tempDenormalizedPatch;

        if (!m_cachedContents.IsNull())
        {
            Dom::PatchOutcome outcome;
            if (!patch.ContainsNormalizedEntries())
            {
                outcome = patch.ApplyInPlace(m_cachedContents);
            }
            else
            {
                tempDenormalizedPatch = patch;
                outcome = tempDenormalizedPatch.ApplyInPlaceAndDenormalize(m_cachedContents);
                appliedPatch = &tempDenormalizedPatch;
            }

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
        m_changedEvent.Signal(*appliedPatch);
    }

    Dom::Value DocumentAdapter::SendAdapterMessage(const AdapterMessage& message)
    {
        // First, fire HandleMessage to allow descendants to handle the message.
        Dom::Value result = HandleMessage(message);
        // Then, notify any consumers (e.g. the DocumentPropertyEditor) in case they want to handle the message
        m_messageEvent.Signal(message, result);
        return result;
    }

    Dom::Value DocumentAdapter::HandleMessage([[maybe_unused]] const AdapterMessage& message)
    {
        // By default, just ignore any messages. This can be overridden by subclasses.
        return Dom::Value();
    }

    Dom::Value BoundAdapterMessage::operator()(const Dom::Value& parameters)
    {
        AdapterMessage message;
        message.m_contextData = m_contextData;
        message.m_messageName = m_messageName;
        message.m_messageOrigin = m_messageOrigin;
        message.m_messageParameters = parameters;
        return m_adapter->SendAdapterMessage(message);
    }

    Dom::Value BoundAdapterMessage::MarshalToDom() const
    {
        Dom::Value result(Dom::Type::Object);
        result[s_typeField] = Dom::Value(s_typeName, false);
        result[s_adapterField] = Dom::Utils::ValueFromType(m_adapter);
        result[s_messageNameField] = Dom::Value(m_messageName.GetStringView(), true);
        result[s_messageOriginField] = Dom::Value(m_messageOrigin.ToString(), true);
        result[s_contextDataField] = m_contextData;
        return result;
    }

    AZStd::optional<BoundAdapterMessage> BoundAdapterMessage::TryMarshalFromDom(const Dom::Value& value)
    {
        if (!value.IsObject())
        {
            return {};
        }

        auto typeFieldIt = value.FindMember(s_typeField);
        if (typeFieldIt == value.MemberEnd() || typeFieldIt->second != Dom::Value(s_typeName, false))
        {
            return {};
        }

        BoundAdapterMessage message;
        message.m_adapter = Dom::Utils::ValueToTypeUnsafe<DocumentAdapter*>(value[s_adapterField]);
        message.m_messageName = AZ::Name(value[s_messageNameField].GetString());
        message.m_messageOrigin = Dom::Path(value[s_messageOriginField].GetString());
        message.m_contextData = value[s_contextDataField];
        return message;
    }
} // namespace AZ::DocumentPropertyEditor
