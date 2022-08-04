/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Interface/Interface.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzFramework/DocumentPropertyEditor/SettingsRegistryAdapter.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>
#include <AzCore/Settings/SettingsRegistryVisitorUtils.h>
#include <AzCore/JSON/rapidjson.h>

namespace AZ::DocumentPropertyEditor
{
    SettingsRegistryAdapter::SettingsRegistryAdapter()
        : SettingsRegistryAdapter(AZ::Interface<AZ::SettingsRegistryOriginTracker>::Get())
    {
    }

    SettingsRegistryAdapter::SettingsRegistryAdapter(AZ::SettingsRegistryOriginTracker* originTracker)
        : m_originTracker(originTracker)
    {
    }

    SettingsRegistryAdapter::~SettingsRegistryAdapter() = default;

    bool SettingsRegistryAdapter::BuildField(
        AZStd::string_view path, AZStd::string_view fieldName, AZ::SettingsRegistryInterface::Type type)
    {
        AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
        m_builder.BeginRow();
        m_builder.Label(fieldName);
        if (type == AZ::SettingsRegistryInterface::Type::Boolean)
        {
            bool value;
            if (settingsRegistry.Get(value, path))
            {
                m_builder.BeginPropertyEditor<Nodes::CheckBox>(Dom::Value(value));
                m_builder.EndPropertyEditor();
            }
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Integer)
        {
            using SettingsType = AZ::SettingsRegistryInterface::SettingsType;
            const SettingsType settingsType = settingsRegistry.GetType(path);
            if (settingsType.m_signedness == AZ::SettingsRegistryInterface::Signedness::Signed)
            {
                if (AZ::s64 value; settingsRegistry.Get(value, path))
                {
                    m_builder.BeginPropertyEditor<Nodes::IntSpinBox>(Dom::Value(value));
                    m_builder.EndPropertyEditor();
                }
            }
            else
            {
                if (AZ::u64 value; settingsRegistry.Get(value, path))
                {
                    m_builder.BeginPropertyEditor<Nodes::UintSpinBox>(Dom::Value(value));
                    m_builder.EndPropertyEditor();
                }
            }
        }
        else if (type == AZ::SettingsRegistryInterface::Type::FloatingPoint)
        {
            if (double value; settingsRegistry.Get(value, path))
            {
                m_builder.BeginPropertyEditor<Nodes::DoubleSpinBox>(Dom::Value(value));
                m_builder.EndPropertyEditor();
            }
        }
        else if (type == AZ::SettingsRegistryInterface::Type::String)
        {
            if (AZStd::string value; settingsRegistry.Get(value, path))
            {
                m_builder.BeginPropertyEditor<Nodes::LineEdit>(Dom::Value(value, true));
                m_builder.Attribute(Nodes::PropertyEditor::ValueType, AZ::Dom::Utils::TypeIdToDomValue(azrtti_typeid<AZStd::string>()));
                m_builder.EndPropertyEditor();
            }
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Object || type == AZ::SettingsRegistryInterface::Type::Array)
        {
            auto visitorCallback = [this](AZStd::string_view path, AZStd::string_view fieldName, AZ::SettingsRegistryInterface::Type type)
            {
                BuildField(path, fieldName, type);
            };
            AZ::SettingsRegistryVisitorUtils::VisitField(settingsRegistry, visitorCallback, path);
            m_builder.Label("");
        }

        AZ::IO::Path originPath;
        if (m_originTracker->FindLastOrigin(originPath, path))
        {
            m_builder.Label(originPath.Native());
        }
        m_builder.EndRow();
        return true;
    }

    Dom::Value SettingsRegistryAdapter::GenerateContents()
    {
        m_builder.BeginAdapter();
        if (m_originTracker != nullptr)
        {
            // Start from the root key of the Settings Registry
            AZ::SettingsRegistryInterface& settingsRegistry = m_originTracker->GetSettingsRegistry();
            BuildField("", "", settingsRegistry.GetType(""));
        }

        m_builder.EndAdapter();
        return m_builder.FinishAndTakeResult();
    }
}
