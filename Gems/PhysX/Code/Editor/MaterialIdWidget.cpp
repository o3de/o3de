/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <PhysX_precompiled.h>
#include <Editor/MaterialIdWidget.h>

#include <AzFramework/Physics/PropertyTypes.h>

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
                static const char* defaultLabel = "Default";
                gui->addItem(defaultLabel);
                gui->setCurrentIndex(0);
                return false;
            };

            if (!m_materialLibraryId.IsValid())
            {
                return lockToDefault();
            }

            auto materialAsset = AZ::Data::AssetManager::Instance().GetAsset<Physics::MaterialLibraryAsset>(m_materialLibraryId, AZ::Data::AssetLoadBehavior::Default);
            materialAsset.BlockUntilLoadComplete();

            if (materialAsset.Get() == nullptr)
            {
                return lockToDefault();
            }

            const auto& materialsData = materialAsset.Get()->GetMaterialsData();

            if (materialsData.size() == 0)
            {
                return lockToDefault();
            }

            m_libraryIds.reserve(materialsData.size());

            m_libraryIds.push_back(Physics::MaterialId());
            gui->addItem("Default");

            for (const auto& materialData : materialAsset.Get()->GetMaterialsData())
            {
                gui->addItem(materialData.m_configuration.m_surfaceType.c_str());
                m_libraryIds.push_back(materialData.m_id);
            }

            gui->setCurrentIndex(GetIndexForId(instance));

            return false;
        }

        Physics::MaterialId MaterialIdWidget::GetIdForIndex(size_t index)
        {
            if (m_libraryIds.size() <= index)
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
