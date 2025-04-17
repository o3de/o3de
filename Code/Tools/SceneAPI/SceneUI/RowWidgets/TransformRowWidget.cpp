/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QLabel>
#include <QStyle>
#include <QGridLayout>
#include <SceneAPI/SceneUI/RowWidgets/TransformRowWidget.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyDoubleSpinCtrl.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            AZ_CLASS_ALLOCATOR_IMPL(ExpandedTransform, SystemAllocator);

            void PopulateVector3(AzQtComponents::VectorInput* vectorProperty, AZ::Vector3& vector)
            {
                AZ_Assert(vectorProperty->getSize() == 3, "Trying to populate a Vector3 from an invalidly sized Vector PropertyCtrl");

                if (vectorProperty->getSize() < 3)
                {
                    return;
                }

                AzQtComponents::VectorElement** elements = vectorProperty->getElements();

                for (int i = 0; i < vectorProperty->getSize(); ++i)
                {
                    AzQtComponents::VectorElement* currentElement = elements[i];
                    vector.SetElement(i, aznumeric_cast<float>(currentElement->getValue()));
                }
            }

            ExpandedTransform::ExpandedTransform()
                : m_translation(0, 0, 0)
                , m_rotation(0, 0, 0)
                , m_scale(1)
            {
            }

            ExpandedTransform::ExpandedTransform(const Transform& transform)
            {
                SetTransform(transform);
            }

            void ExpandedTransform::SetTransform(const AZ::Transform& transform)
            {
                m_translation = transform.GetTranslation();
                m_rotation = transform.GetEulerDegrees();
                m_scale = transform.GetUniformScale();
            }

            void ExpandedTransform::GetTransform(AZ::Transform& transform) const
            {
                transform = Transform::CreateTranslation(m_translation);
                transform *= AZ::ConvertEulerDegreesToTransform(m_rotation);
                transform.MultiplyByUniformScale(m_scale);
            }

            const AZ::Vector3& ExpandedTransform::GetTranslation() const
            {
                return m_translation;
            }

            void ExpandedTransform::SetTranslation(const AZ::Vector3& translation)
            {
                m_translation = translation;
            }

            const AZ::Vector3& ExpandedTransform::GetRotation() const
            {
                return m_rotation;
            }

            void ExpandedTransform::SetRotation(const AZ::Vector3& rotation)
            {
                m_rotation = rotation;
            }

            const float ExpandedTransform::GetScale() const
            {
                return m_scale;
            }

            void ExpandedTransform::SetScale(const float scale)
            {
                m_scale = scale;
            }


            AZ_CLASS_ALLOCATOR_IMPL(TransformRowWidget, SystemAllocator);

            TransformRowWidget::TransformRowWidget(QWidget* parent)
                : QWidget(parent)
            {
                m_containerWidget = new QWidget();
                QGridLayout* layout = new QGridLayout();
                layout->setMargin(0);
                QGridLayout* layout2 = new QGridLayout();
                setLayout(layout);
                AzToolsFramework::PropertyRowWidget* parentWidget = static_cast<AzToolsFramework::PropertyRowWidget*>(parent);
                QToolButton* toolButton{};
                QVBoxLayout* layoutOriginal{};
                if (parentWidget != nullptr)
                {
                    toolButton = parentWidget->GetIndicatorButton();
                    layoutOriginal = parentWidget->GetLeftHandSideLayoutParent();
                    parentWidget->SetAsCustom(true);
                    parentWidget->GetNameLabel()->setContentsMargins(0, 0, 0, 0);
                }

                m_translationWidget = new AzQtComponents::VectorInput(this, 3);
                m_translationWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                m_translationWidget->setMinimum(-9999999);
                m_translationWidget->setMaximum(9999999);

                m_rotationWidget = new AzQtComponents::VectorInput(this, 3);
                m_rotationWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                m_rotationWidget->setLabel(0, "P");
                m_rotationWidget->setLabel(1, "R");
                m_rotationWidget->setLabel(2, "Y");
                m_rotationWidget->setMinimum(0);
                m_rotationWidget->setMaximum(360);
                m_rotationWidget->setSuffix(" degrees");

                m_scaleWidget = new AzToolsFramework::PropertyDoubleSpinCtrl(this);
                m_scaleWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                m_scaleWidget->setMinimum(0);
                m_scaleWidget->setMaximum(10000);

                layout2->addWidget(new  AzQtComponents::ElidingLabel("Position"), 0, 1);
                layout->addWidget(m_translationWidget, 1, 1);
                layout2->addWidget(new  AzQtComponents::ElidingLabel("Rotation"), 1, 1);
                layout->addWidget(m_rotationWidget, 2, 1);
                layout2->addWidget(new  AzQtComponents::ElidingLabel("Scale"), 2, 1);
                layout->addWidget(m_scaleWidget, 3, 1);
                layout->setRowMinimumHeight(0,16);
                layout2->setColumnMinimumWidth(0, 30);

                if (parentWidget != nullptr)
                {
                    toolButton->setArrowType(Qt::DownArrow);
                    parentWidget->SetIndentSize(1);
                    toolButton->setVisible(true);

                    m_containerWidget->setLayout(layout2);
                    layoutOriginal->addWidget(m_containerWidget);

                    connect(toolButton, &QToolButton::clicked, this, [&, toolButton]
                    {
                        m_expanded = !m_expanded;
                        if (m_expanded)
                        {
                            show();
                            m_containerWidget->show();
                            toolButton->setArrowType(Qt::DownArrow);
                        }
                        else
                        {
                            hide();
                            m_containerWidget->hide();
                            toolButton->setArrowType(Qt::RightArrow);
                        }
                    });
                }

                QObject::connect(m_translationWidget, &AzQtComponents::VectorInput::valueChanged, this, [this]
                {
                    AzQtComponents::VectorInput* widget = this->GetTranslationWidget();
                    AZ::Vector3 translation;

                    PopulateVector3(widget, translation);

                    m_transform.SetTranslation(translation);
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, this);
                });

                QObject::connect(m_rotationWidget, &AzQtComponents::VectorInput::valueChanged, this, [this]
                {
                    AzQtComponents::VectorInput* widget = this->GetRotationWidget();
                    AZ::Vector3 rotation;

                    PopulateVector3(widget, rotation);

                    m_transform.SetRotation(rotation);
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, this);
                });

                QObject::connect(m_scaleWidget, &AzToolsFramework::PropertyDoubleSpinCtrl::valueChanged, this, [this]
                {
                    AzToolsFramework::PropertyDoubleSpinCtrl* widget = this->GetScaleWidget();
                    float scale = aznumeric_cast<float>(widget->value());
                    m_transform.SetScale(scale);
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, this);
                });
            }

            TransformRowWidget::~TransformRowWidget()
            {
                if (m_containerWidget)
                {
                    m_containerWidget->deleteLater();
                }
            }

            void TransformRowWidget::SetEnableEdit(bool enableEdit)
            {
                m_translationWidget->setEnabled(enableEdit);
                m_rotationWidget->setEnabled(enableEdit);
                m_scaleWidget->setEnabled(enableEdit);
            }

            void TransformRowWidget::SetTransform(const AZ::Transform& transform)
            {
                blockSignals(true);

                m_transform.SetTransform(transform);

                m_translationWidget->setValuebyIndex(m_transform.GetTranslation().GetX(), 0);
                m_translationWidget->setValuebyIndex(m_transform.GetTranslation().GetY(), 1);
                m_translationWidget->setValuebyIndex(m_transform.GetTranslation().GetZ(), 2);

                m_rotationWidget->setValuebyIndex(m_transform.GetRotation().GetX(), 0);
                m_rotationWidget->setValuebyIndex(m_transform.GetRotation().GetY(), 1);
                m_rotationWidget->setValuebyIndex(m_transform.GetRotation().GetZ(), 2);

                m_scaleWidget->setValue(m_transform.GetScale());

                blockSignals(false);
            }

            void TransformRowWidget::GetTransform(AZ::Transform& transform) const
            {
                m_transform.GetTransform(transform);
            }

            const ExpandedTransform& TransformRowWidget::GetExpandedTransform() const
            {
                return m_transform;
            }

            AzQtComponents::VectorInput* TransformRowWidget::GetTranslationWidget()
            {
                return m_translationWidget;
            }

            AzQtComponents::VectorInput* TransformRowWidget::GetRotationWidget()
            {
                return m_rotationWidget;
            }

            AzToolsFramework::PropertyDoubleSpinCtrl* TransformRowWidget::GetScaleWidget()
            {
                return m_scaleWidget;
            }
        } // namespace SceneUI
    } // namespace SceneAPI
} // namespace AZ

#include <RowWidgets/moc_TransformRowWidget.cpp>
