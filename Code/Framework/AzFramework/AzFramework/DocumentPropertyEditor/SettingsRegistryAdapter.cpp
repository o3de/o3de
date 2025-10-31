/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/PropertyEditorNodes.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/SettingsRegistryAdapter.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/JSON/rapidjson.h>

namespace AZ::DocumentPropertyEditor
{
    SettingsRegistryAdapter::SettingsNotificationHandler::SettingsNotificationHandler(SettingsRegistryAdapter& adapter)
        : m_settingsRegistryAdapter(adapter)
    {
    }

    SettingsRegistryAdapter::SettingsNotificationHandler::~SettingsNotificationHandler() = default;

    void SettingsRegistryAdapter::SettingsNotificationHandler::operator()(
        const AZ::SettingsRegistryInterface::NotifyEventArgs&)
    {
        // If the settings registry adapter message handler is updating the Settings Registry
        // prevent recursive loop by protecthing against the NotifyResetDocument call
        if (!m_settingsRegistryAdapter.m_inEdit)
        {
            m_settingsRegistryAdapter.NotifyResetDocument();
        }
    }

    SettingsRegistryAdapter::SettingsRegistryAdapter()
        : SettingsRegistryAdapter(AZ::Interface<AZ::SettingsRegistryOriginTracker>::Get())
    {
    }

    SettingsRegistryAdapter::SettingsRegistryAdapter(AZ::SettingsRegistryOriginTracker* originTracker)
        : m_originTracker(originTracker)
    {
        if (m_originTracker)
        {
            AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
            m_notifyHandler = settingsRegistry.RegisterNotifier(SettingsNotificationHandler(*this));
        }
    }

    SettingsRegistryAdapter::~SettingsRegistryAdapter() = default;

    SettingsRegistryOriginTracker* SettingsRegistryAdapter::GetSettingsRegistryOriginTracker()
    {
        return m_originTracker;
    }

    const SettingsRegistryOriginTracker* SettingsRegistryAdapter::GetSettingsRegistryOriginTracker() const
    {
        return m_originTracker;
    }

    bool SettingsRegistryAdapter::BuildBool(AZStd::string_view path)
    {
        AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
        if (bool value; settingsRegistry.Get(value, path))
        {
            auto UpdateSettingsRegistry = [this, &settingsRegistry](
                AZStd::string_view keyPath, const AZ::Dom::Value& valueFromEditor) -> AZ::Dom::Value
            {
                if (valueFromEditor.IsBool())
                {
                    bool domValue = valueFromEditor.GetBool();
                    m_inEdit = true;
                    settingsRegistry.Set(keyPath, domValue);
                    m_inEdit = false;
                    return AZ::Dom::Value(domValue);
                }
                return {};
            };
            m_builder.BeginPropertyEditor<Nodes::CheckBox>(Dom::Value(value));
            m_builder.AddMessageHandler(this, Nodes::PropertyEditor::OnChanged);
            SettingsRegistryDomData domData = { path, AZStd::move(UpdateSettingsRegistry) };
            m_fieldCallbackPrefixTree.SetValue(m_builder.GetCurrentPath(), domData);
            m_builder.EndPropertyEditor();
            return true;
        }
        return false;
    }

    bool SettingsRegistryAdapter::BuildInt64(AZStd::string_view path)
    {
        AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
        if (AZ::s64 value;settingsRegistry.Get(value, path))
        {
            auto UpdateSettingsRegistry =  [this, &settingsRegistry](
                AZStd::string_view keyPath, const AZ::Dom::Value& valueFromEditor) -> AZ::Dom::Value
            {
                if (valueFromEditor.IsInt())
                {
                    int64_t domValue = valueFromEditor.GetInt64();
                    m_inEdit = true;
                    settingsRegistry.Set(keyPath, static_cast<AZ::s64>(domValue));
                    m_inEdit = false;
                    return AZ::Dom::Value(domValue);
                }
                return {};
            };
            m_builder.BeginPropertyEditor<Nodes::IntSpinBox>(Dom::Value(static_cast<int64_t>(value)));
            m_builder.AddMessageHandler(this, Nodes::PropertyEditor::OnChanged);
            SettingsRegistryDomData domData = { path, AZStd::move(UpdateSettingsRegistry) };
            m_fieldCallbackPrefixTree.SetValue(m_builder.GetCurrentPath(), domData);
            m_builder.EndPropertyEditor();
            return true;
        }
        return false;
    }

    bool SettingsRegistryAdapter::BuildUInt64(AZStd::string_view path)
    {
        AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
        if (AZ::u64 value; settingsRegistry.Get(value, path))
        {
            auto UpdateSettingsRegistry = [this, &settingsRegistry](
                AZStd::string_view keyPath, const AZ::Dom::Value& valueFromEditor) -> AZ::Dom::Value
            {
                if (valueFromEditor.IsUint())
                {
                    uint64_t domValue = valueFromEditor.GetUint64();
                    m_inEdit = true;
                    settingsRegistry.Set(keyPath, static_cast<AZ::u64>(domValue));
                    m_inEdit = false;
                    return AZ::Dom::Value();
                }
                return {};
            };
            m_builder.BeginPropertyEditor<Nodes::UintSpinBox>(Dom::Value(static_cast<uint64_t>(value)));
            m_builder.AddMessageHandler(this, Nodes::PropertyEditor::OnChanged);
            SettingsRegistryDomData domData = { path, AZStd::move(UpdateSettingsRegistry) };
            m_fieldCallbackPrefixTree.SetValue(m_builder.GetCurrentPath(), domData);
            m_builder.EndPropertyEditor();
            return true;
        }
        return false;
    }

    bool SettingsRegistryAdapter::BuildFloat(AZStd::string_view path)
    {
        AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
        if (double value; settingsRegistry.Get(value, path))
        {
            auto UpdateSettingsRegistry = [this, &settingsRegistry](
                AZStd::string_view keyPath, const AZ::Dom::Value& valueFromEditor) -> AZ::Dom::Value
            {
                if (valueFromEditor.IsDouble())
                {
                    double domValue = valueFromEditor.GetDouble();
                    m_inEdit = true;
                    settingsRegistry.Set(keyPath, domValue);
                    m_inEdit = false;
                    return AZ::Dom::Value(domValue);
                }
                return {};
            };
            m_builder.BeginPropertyEditor<Nodes::DoubleSpinBox>(Dom::Value(value));
            m_builder.AddMessageHandler(this, Nodes::PropertyEditor::OnChanged);
            SettingsRegistryDomData domData = { path, AZStd::move(UpdateSettingsRegistry) };
            m_fieldCallbackPrefixTree.SetValue(m_builder.GetCurrentPath(), domData);
            m_builder.EndPropertyEditor();
            return true;
        }
        return false;
    }

    bool SettingsRegistryAdapter::BuildString(AZStd::string_view path)
    {
        AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
        if (AZStd::string value; settingsRegistry.Get(value, path))
        {
            auto UpdateSettingsRegistry = [this, &settingsRegistry](
                AZStd::string_view keyPath, const AZ::Dom::Value& valueFromEditor) -> AZ::Dom::Value
            {
                if (valueFromEditor.IsString())
                {
                    AZStd::string_view domValue = valueFromEditor.GetString();
                    m_inEdit = true;
                    settingsRegistry.Set(keyPath, domValue);
                    m_inEdit = false;
                    return AZ::Dom::Value(domValue, true);
                }
                return {};
            };
            m_builder.BeginPropertyEditor<Nodes::LineEdit>(Dom::Value(AZStd::string_view(value), true));
            m_builder.Attribute(Nodes::PropertyEditor::ValueType, AZ::Dom::Utils::TypeIdToDomValue(azrtti_typeid<AZStd::string>()));
            m_builder.AddMessageHandler(this, Nodes::PropertyEditor::OnChanged);
            SettingsRegistryDomData domData = { path, AZStd::move(UpdateSettingsRegistry) };
            m_fieldCallbackPrefixTree.SetValue(m_builder.GetCurrentPath(), domData);
            m_builder.EndPropertyEditor();
            return true;
        }
        return false;
    }

    bool SettingsRegistryAdapter::BuildField(
        AZStd::string_view path, AZStd::string_view fieldName, AZ::SettingsRegistryInterface::SettingsType type)
    {
        struct ScopedAdapterBuilderRowCreation
        {
            ScopedAdapterBuilderRowCreation(AZ::DocumentPropertyEditor::AdapterBuilder& adapterBuilder)
                : m_adapterBuilder(adapterBuilder)
            {
                m_adapterBuilder.BeginRow();
            }
            ~ScopedAdapterBuilderRowCreation()
            {
                m_adapterBuilder.EndRow();
            }
        private:
            AZ::DocumentPropertyEditor::AdapterBuilder& m_adapterBuilder;
        };

        ScopedAdapterBuilderRowCreation scopedBuilderRow(m_builder);
        m_builder.Label(fieldName);
        if (type == AZ::SettingsRegistryInterface::Type::Boolean)
        {
            BuildBool(path);
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Integer)
        {
            if (type.m_signedness == AZ::SettingsRegistryInterface::Signedness::Signed)
            {
                BuildInt64(path);
            }
            else
            {
                BuildUInt64(path);
            }
        }
        else if (type == AZ::SettingsRegistryInterface::Type::FloatingPoint)
        {
            BuildFloat(path);
        }
        else if (type == AZ::SettingsRegistryInterface::Type::String)
        {
            BuildString(path);
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Object || type == AZ::SettingsRegistryInterface::Type::Array)
        {
            auto visitorCallback = [this](const AZ::SettingsRegistryInterface::VisitArgs& visitArgs)
            {
                BuildField(visitArgs.m_jsonKeyPath, visitArgs.m_fieldName, visitArgs.m_type);
                return AZ::SettingsRegistryInterface::VisitResponse::Skip;
            };
            AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
            AZ::SettingsRegistryVisitorUtils::VisitField(settingsRegistry, visitorCallback, path);
            m_builder.Label("");
            SettingsRegistryDomData domData;
            domData.m_settingsKeyPath = path;
            domData.m_callback = [](AZStd::string_view, const AZ::Dom::Value&) -> AZ::Dom::Value
            {
                // When support for adding for objects/array types
                // are implemented, This function will need to prompt the user
                // for the name of the new field(objects only) and the value type of the field
                return {};
            };
            m_fieldCallbackPrefixTree.SetValue(m_builder.GetCurrentPath(), domData);
        }
        else
        {
            return false;
        }

        AZ::IO::Path originPath;
        if (m_originTracker->FindLastOrigin(originPath, path))
        {
            m_builder.Label(originPath.Native());
        }
        return true;
    }

    Dom::Value SettingsRegistryAdapter::GenerateContents()
    {
        if (m_originTracker != nullptr)
        {
            m_builder.BeginAdapter();
            // Start from the root key of the Settings Registry
            AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
            BuildField("", "", settingsRegistry.GetType(""));
            m_builder.EndAdapter();
        }

        return m_builder.FinishAndTakeResult();
    }

    Dom::Value SettingsRegistryAdapter::HandleMessage(const AdapterMessage& message)
    {
        using ValueChangeType = Nodes::ValueChangeType;
        auto handlePropertyEditorChanged = [messageOrigin = message.m_messageOrigin, this](
            const Dom::Value& valueFromEditor,
            ValueChangeType changeType)
        {
            if (changeType == ValueChangeType::FinishedEdit)
            {
                if (SettingsRegistryDomData* domData =
                    m_fieldCallbackPrefixTree.ValueAtPath(messageOrigin, AZ::Dom::PrefixTreeMatch::ExactPath);
                    domData != nullptr)
                {
                    if (domData->m_callback)
                    {
                        Dom::Value newValue = domData->m_callback(domData->m_settingsKeyPath, valueFromEditor);
                        NotifyContentsChanged({ Dom::PatchOperation::ReplaceOperation(messageOrigin / "Value", newValue) });
                    }
                }
            }
        };

        auto handleTreeUpdate = [this](Nodes::PropertyRefreshLevel)
        {
            // For now just trigger a soft reset.
            // This will still only send the view patches for what's actually changed.
            NotifyResetDocument();
        };

        return message.Match(
            Nodes::PropertyEditor::OnChanged,
            handlePropertyEditorChanged,
            Nodes::PropertyEditor::RequestTreeUpdate,
            handleTreeUpdate);
    }
}
