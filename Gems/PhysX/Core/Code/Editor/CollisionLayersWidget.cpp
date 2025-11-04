/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Physics/Utils.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyStringLineEditCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/InstanceDataHierarchy.h>

#include <QBoxLayout>

#include <Editor/CollisionLayersWidget.h>

namespace PhysX
{
    namespace Editor
    {
        const AZStd::string CollisionLayersWidget::s_defaultCollisionLayerName = "Default";

        CollisionLayersWidget::CollisionLayersWidget(QWidget* parent)
            : QWidget(parent)
        {
            CreatePropertyEditor(this);
        }

        void CollisionLayersWidget::SetValue(const AzPhysics::CollisionLayers& layers)
        {
            m_value = layers;

            blockSignals(true);
            m_propertyEditor->ClearInstances();
            m_propertyEditor->AddInstance(&m_value);
            m_propertyEditor->InvalidateAll();
            SetWidgetParameters();
            blockSignals(false);
        }

        const AzPhysics::CollisionLayers& CollisionLayersWidget::GetValue() const
        {
            return m_value;
        }

        void CollisionLayersWidget::CreatePropertyEditor(QWidget* parent)
        {
            QVBoxLayout* verticalLayout = new QVBoxLayout(parent);
            verticalLayout->setContentsMargins(0, 0, 0, 0);
            verticalLayout->setSpacing(0);

            AZ::SerializeContext* m_serializeContext;
            AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(m_serializeContext, "Failed to retrieve serialize context.");

            static const int propertyLabelWidth = 250;
            m_propertyEditor = new AzToolsFramework::ReflectedPropertyEditor(parent);
            m_propertyEditor->Setup(m_serializeContext, this, true, propertyLabelWidth);
            m_propertyEditor->show();
            m_propertyEditor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            verticalLayout->addWidget(m_propertyEditor);
        }

        void CollisionLayersWidget::BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void CollisionLayersWidget::AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void CollisionLayersWidget::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/)
        {

        }

        void CollisionLayersWidget::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* node)
        {
            // Find index of modified layer
            AzToolsFramework::InstanceDataNode* nodeParent = node->GetParent();
            AzToolsFramework::InstanceDataNode::NodeContainer nodeSiblings = nodeParent->GetChildren();
            AZ::u32 nodeIndex = 0;
            AzToolsFramework::InstanceDataNode::Address nodeAddress = node->ComputeAddress();
            for (AzToolsFramework::InstanceDataNode& nodeSibling : nodeSiblings)
            {
                if (nodeSibling.ComputeAddress() == nodeAddress)
                {
                    break;
                }
                ++nodeIndex;
            }

            const AZStd::array<AZStd::string, AzPhysics::CollisionLayers::MaxCollisionLayers>& layerNames = m_value.GetNames();

            if (nodeIndex >= layerNames.size())
            {
                return;
            }
            
            // If corrections to layer name were made so it is unique, refresh UI.
            AZStd::string uniqueLayerName;
            if (ForceUniqueLayerName(nodeIndex, layerNames, uniqueLayerName))
            {
                AZ_Warning("PhysX Collision Layers"
                    , false
                    , "Invalid collision layer name used. Collision layer automatically renamed to: %s"
                    , uniqueLayerName.c_str());
                m_value.SetName(nodeIndex, uniqueLayerName);
                blockSignals(true);
                m_propertyEditor->InvalidateValues();
                blockSignals(false);
            }

            emit onValueChanged(m_value);
        }

        void CollisionLayersWidget::SealUndoStack()
        {

        }

        void CollisionLayersWidget::SetWidgetParameters()
        {
            const AzToolsFramework::ReflectedPropertyEditor::WidgetList& widgets = m_propertyEditor->GetWidgets();
            for (auto& widgetIter : widgets)
            {
                AzToolsFramework::PropertyRowWidget* rowWidget = widgetIter.second;
                QWidget* widget = rowWidget->GetChildWidget();
                if (widget == nullptr)
                {
                    continue;
                }
                AzToolsFramework::PropertyStringLineEditCtrl* lineEditCtrl =
                    static_cast<AzToolsFramework::PropertyStringLineEditCtrl*>(widget); // qobject_cast does not work here
                if (lineEditCtrl == nullptr)
                {
                    continue;
                }
                lineEditCtrl->setMaxLen(s_maxCollisionLayerNameLength);
                if (lineEditCtrl->value() == s_defaultCollisionLayerName)
                {
                    lineEditCtrl->setEnabled(false);
                }
            }
        }

        bool CollisionLayersWidget::ForceUniqueLayerName(AZ::u32 layerIndex
            , const AZStd::array<AZStd::string, AzPhysics::CollisionLayers::MaxCollisionLayers>& layerNames
            , AZStd::string& uniqueLayerNameOut)
        {
            if (layerIndex >= layerNames.size())
            {
                AZ_Warning("PhysX Collision Layers", false, "Trying to validate layer name of layer with invalid index: %d", layerIndex);
                return false;
            }

            if (IsLayerNameUnique(layerIndex, layerNames))
            {
                return false;
            }

            AZStd::unordered_set<AZStd::string> nameSet(layerNames.begin(), layerNames.end());
            nameSet.erase(AZStd::string("")); // Empty layer names are layers that are not used but remain in the array.
            uniqueLayerNameOut = layerNames[layerIndex];
            Physics::Utils::MakeUniqueString(nameSet
                , uniqueLayerNameOut
                , s_maxCollisionLayerNameLength);
            return true;
        }

        bool CollisionLayersWidget::IsLayerNameUnique(AZ::u32 layerIndex,
            const AZStd::array<AZStd::string, AzPhysics::CollisionLayers::MaxCollisionLayers>& layerNames)
        {
            for (size_t i = 0; i < layerNames.size(); ++i)
            {
                if (i == layerIndex)
                {
                    continue;
                }

                const AZStd::string& layerName = layerNames[i];
                if (layerName.length() > 0 && layerName == layerNames[layerIndex])
                {
                    return false;
                }
            }
            return true;
        }
    } // namespace Editor
} // namespace PhysX

#include <Editor/moc_CollisionLayersWidget.cpp>
