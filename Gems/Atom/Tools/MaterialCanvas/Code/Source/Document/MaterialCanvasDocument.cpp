/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <Document/MaterialCanvasDocument.h>

namespace MaterialCanvas
{
    MaterialCanvasDocument::MaterialCanvasDocument()
        : AtomToolsFramework::AtomToolsDocument()
    {
        MaterialCanvasDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialCanvasDocument::~MaterialCanvasDocument()
    {
        MaterialCanvasDocumentRequestBus::Handler::BusDisconnect();
    }

    const AZStd::any& MaterialCanvasDocument::GetPropertyValue(const AZ::Name& propertyId) const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document is not open.");
            return m_invalidValue;
        }

        const auto it = m_properties.find(propertyId);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document property could not be found: '%s'.", propertyId.GetCStr());
            return m_invalidValue;
        }

        const AtomToolsFramework::DynamicProperty& property = it->second;
        return property.GetValue();
    }

    const AtomToolsFramework::DynamicProperty& MaterialCanvasDocument::GetProperty(const AZ::Name& propertyId) const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document is not open.");
            return m_invalidProperty;
        }

        const auto it = m_properties.find(propertyId);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document property could not be found: '%s'.", propertyId.GetCStr());
            return m_invalidProperty;
        }

        const AtomToolsFramework::DynamicProperty& property = it->second;
        return property;
    }
    
    bool MaterialCanvasDocument::IsPropertyGroupVisible(const AZ::Name& propertyGroupFullName) const
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document is not open.");
            return false;
        }

        const auto it = m_propertyGroupVisibility.find(propertyGroupFullName);
        if (it == m_propertyGroupVisibility.end())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document property group could not be found: '%s'.", propertyGroupFullName.GetCStr());
            return false;
        }

        return it->second;
    }

    void MaterialCanvasDocument::SetPropertyValue(const AZ::Name& propertyId, [[maybe_unused]] const AZStd::any& value)
    {
        if (!IsOpen())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document is not open.");
            return;
        }

        const auto it = m_properties.find(propertyId);
        if (it == m_properties.end())
        {
            AZ_Error("MaterialCanvasDocument", false, "Document property could not be found: '%s'.", propertyId.GetCStr());
            return;
        }

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentPropertyValueModified, m_id, it->second);
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Broadcast(
            &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentModified, m_id);
    }

    bool MaterialCanvasDocument::IsModified() const
    {
        return AZStd::any_of(m_properties.begin(), m_properties.end(),
            [](const auto& propertyPair)
        {
            const AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            return !AtomToolsFramework::ArePropertyValuesEqual(property.GetValue(), property.GetConfig().m_originalValue);
        });
    }

    bool MaterialCanvasDocument::IsSavable() const
    {
        return true;
    }

    bool MaterialCanvasDocument::BeginEdit()
    {
        // Save the current properties as a momento for undo before any changes are applied
        m_propertyValuesBeforeEdit.clear();
        for (const auto& propertyPair : m_properties)
        {
            const AtomToolsFramework::DynamicProperty& property = propertyPair.second;
            m_propertyValuesBeforeEdit[property.GetId()] = property.GetValue();
        }
        return true;
    }

    bool MaterialCanvasDocument::EndEdit()
    {
        PropertyValueMap propertyValuesForUndo;
        PropertyValueMap propertyValuesForRedo;

        // After editing has completed, check to see if properties have changed so the deltas can be recorded in the history
        for (const auto& propertyBeforeEditPair : m_propertyValuesBeforeEdit)
        {
            const auto& propertyName = propertyBeforeEditPair.first;
            const auto& propertyValueForUndo = propertyBeforeEditPair.second;
            const auto& propertyValueForRedo = GetPropertyValue(propertyName);
            if (!AtomToolsFramework::ArePropertyValuesEqual(propertyValueForUndo, propertyValueForRedo))
            {
                propertyValuesForUndo[propertyName] = propertyValueForUndo;
                propertyValuesForRedo[propertyName] = propertyValueForRedo;
            }
        }

        if (!propertyValuesForUndo.empty() && !propertyValuesForRedo.empty())
        {
            AddUndoRedoHistory(
                [this, propertyValuesForUndo]() { RestorePropertyValues(propertyValuesForUndo); },
                [this, propertyValuesForRedo]() { RestorePropertyValues(propertyValuesForRedo); });
        }

        m_propertyValuesBeforeEdit.clear();
        return true;
    }

    void MaterialCanvasDocument::Clear()
    {
        AtomToolsFramework::AtomToolsDocument::Clear();

        m_compilePending = {};
        m_properties.clear();
        m_propertyValuesBeforeEdit.clear();
    }

    void MaterialCanvasDocument::RestorePropertyValues(const PropertyValueMap& propertyValues)
    {
        for (const auto& propertyValuePair : propertyValues)
        {
            const auto& propertyName = propertyValuePair.first;
            const auto& propertyValue = propertyValuePair.second;
            SetPropertyValue(propertyName, propertyValue);
        }
    }

} // namespace MaterialCanvas
