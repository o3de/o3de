/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/MaterialIdWidget.h>

#include <AzFramework/Physics/PropertyTypes.h>

#include <QString>

namespace PhysX
{
    namespace Editor
    {
        AZ::u32 MaterialIdWidget::GetHandlerName() const
        {
            return Physics::Edit::MaterialIdSelector;
        }

        QWidget* MaterialIdWidget::CreateGUI(QWidget* parent)
        {
            widget_t* picker = new widget_t(parent);

            connect(picker, static_cast<void(QComboBox::*)(int)>(&widget_t::currentIndexChanged), this, [picker]()
            {
                EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, picker);
            });
            picker->setStyleSheet(":disabled { color: rgb(180, 180, 180); }");

            return picker;
        }

        bool MaterialIdWidget::IsDefaultHandler() const
        {
            return true;
        }

        void MaterialIdWidget::ConsumeAttribute(widget_t* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
        {
            if (attrib == AZ::Edit::Attributes::ReadOnly)
            {
                bool value = false;
                if (attrValue->Read<bool>(value))
                {
                    GUI->setEnabled(!value);
                }
            }

            if (attrib == Physics::Attributes::MaterialLibraryAssetId)
            {
                attrValue->Read<AZ::Data::AssetId>(m_materialLibraryId);
            }
        }

        void MaterialIdWidget::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, widget_t* gui, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            instance = GetIdForIndex(gui->currentIndex());
        }

        bool MaterialIdWidget::ReadValuesIntoGUI([[maybe_unused]] size_t index, widget_t* gui, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(gui);
            gui->clear();
            m_libraryIds.clear();

            auto lockToDefault = [gui]()
            {
                gui->addItem(QLatin1String(Physics::DefaultPhysicsMaterialLabel.data(), static_cast<int>(Physics::DefaultPhysicsMaterialLabel.size())));
                gui->setCurrentIndex(0);
                return false;
            };

            if (!m_materialLibraryId.IsValid())
            {
                return lockToDefault();
            }

            auto materialLibraryAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(m_materialLibraryId, AZ::Data::AssetLoadBehavior::Default);
            materialLibraryAsset.BlockUntilLoadComplete();

            if (materialLibraryAsset.Get() == nullptr)
            {
                return lockToDefault();
            }

            const auto& materials = materialLibraryAsset.Get()->GetMaterialsData();

            if (materials.empty())
            {
                return lockToDefault();
            }

            m_libraryIds.reserve(materials.size() + 1); // Plus one to reserve the first element for default physics material

            // Add default physics material first
            m_libraryIds.push_back(Physics::MaterialId());
            gui->addItem(QLatin1String(Physics::DefaultPhysicsMaterialLabel.data(), static_cast<int>(Physics::DefaultPhysicsMaterialLabel.size())));

            for (const auto& material : materials)
            {
                gui->addItem(material.m_configuration.m_surfaceType.c_str());
                m_libraryIds.push_back(material.m_id);
            }

            gui->setCurrentIndex(GetIndexForId(instance));

            return false;
        }

        Physics::MaterialId MaterialIdWidget::GetIdForIndex(size_t index)
        {
            if (index >= m_libraryIds.size())
            {
                return Physics::MaterialId();
            }

            return m_libraryIds[index];
        }

        int MaterialIdWidget::GetIndexForId(const Physics::MaterialId id)
        {
            for (int i = 0; i < m_libraryIds.size(); ++i)
            {
                if (m_libraryIds[i] == id)
                {
                    return i;
                }
            }
            return 0;
        }
    } // namespace Editor
} // namespace PhysX

#include <Editor/moc_MaterialIdWidget.cpp>
