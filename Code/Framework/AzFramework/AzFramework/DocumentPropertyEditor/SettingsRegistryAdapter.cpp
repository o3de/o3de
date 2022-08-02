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
    {
    }

    SettingsRegistryAdapter::~SettingsRegistryAdapter()
    {
    }

    bool SettingsRegistryAdapter::BuildField(
        AZStd::string_view path, AZStd::string_view fieldName, AZ::SettingsRegistryInterface::Type type)
    {
        auto settingsRegistryOriginTracker = AZ::Interface<AZ::SettingsRegistryOriginTracker>::Get();
        AZ::SettingsRegistryInterface& settingsRegistry = settingsRegistryOriginTracker->GetSettingsRegistry();
        auto visitorCallback = [this](AZStd::string_view path, AZStd::string_view fieldName, AZ::SettingsRegistryInterface::Type type)
        {
            BuildField(path, fieldName, type);
        };
        m_builder.BeginRow();
        m_builder.Label(fieldName);
        if (type == AZ::SettingsRegistryInterface::Type::Boolean)
        {
            bool value;
            settingsRegistry.Get(value, path);
            m_builder.BeginPropertyEditor<Nodes::CheckBox>(Dom::Value(value));
            m_builder.EndPropertyEditor();
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Integer)
        {
            AZ::s64 value;
            settingsRegistry.Get(value, path);
            m_builder.BeginPropertyEditor<Nodes::IntSpinBox>(Dom::Value(value));
            m_builder.EndPropertyEditor();
        }
        else if (type == AZ::SettingsRegistryInterface::Type::FloatingPoint)
        {
            m_builder.BeginPropertyEditor<Nodes::DoubleSlider>(Dom::Value(1.0));
            m_builder.EndPropertyEditor();
        }
        else if (type == AZ::SettingsRegistryInterface::Type::String)
        {
            AZStd::string value;
            settingsRegistry.Get(value, path);
            m_builder.BeginPropertyEditor<Nodes::LineEdit>(Dom::Value(AZStd::string_view(value), true));
            m_builder.Attribute(Nodes::PropertyEditor::ValueType, AZ::Dom::Utils::TypeIdToDomValue(azrtti_typeid<AZStd::string>()));
            m_builder.EndPropertyEditor();
        }
        else if (type == AZ::SettingsRegistryInterface::Type::Object || type == AZ::SettingsRegistryInterface::Type::Array)
        {
            AZ::SettingsRegistryVisitorUtils::VisitField(settingsRegistry, visitorCallback, path);
            m_builder.Label("");
        }
        AZ::IO::Path originPath;
        if (settingsRegistryOriginTracker->FindLastOrigin(originPath, path))
        {
            m_builder.Label(originPath.FixedMaxPathString());
        }
        m_builder.EndRow();
        return true;
    }

    Dom::Value SettingsRegistryAdapter::GenerateContents()
    {
        m_builder.BeginAdapter();

        auto settingsRegistryOriginTracker = AZ::Interface<AZ::SettingsRegistryOriginTracker>::Get();
        AZ::SettingsRegistryInterface& settingsRegistry = settingsRegistryOriginTracker->GetSettingsRegistry();
        if (settingsRegistryOriginTracker != nullptr)
        {
            BuildField("", "", settingsRegistry.GetType(""));
        }
        m_builder.EndAdapter();
        return m_builder.FinishAndTakeResult();
    }
}
