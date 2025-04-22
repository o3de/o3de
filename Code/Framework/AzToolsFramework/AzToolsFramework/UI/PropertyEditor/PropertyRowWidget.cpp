/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyRowWidget.hxx"

#include <AzQtComponents/Components/StyleManager.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyCheckBoxCtrl.hxx>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                                    // 4251: class '...' needs to have dll-interface to be used by clients of class 'QInputEvent'
                                                                    // 4800: QTextEngine *const ': forcing value to bool 'true' or 'false' (performance warning)
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>
#include <QtGui/QFontMetrics>
#include <QtGui/QTextLayout>
#include <QtGui/QPainter>
#include <QMessageBox>
#include <QStylePainter>
AZ_POP_DISABLE_WARNING

static const int LabelColumnStretch = 2;
static const int ValueColumnStretch = 3;
 
namespace AzToolsFramework
{
    PropertyRowWidget::PropertyRowWidget(QWidget* pParent)
        : QFrame(pParent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

        static QIcon s_iconOpen(":/PropertyEditor/Resources/group_open.png");
        static QIcon s_iconClosed(":/PropertyEditor/Resources/group_closed.png");
        m_iconOpen = s_iconOpen;
        m_iconClosed = s_iconClosed;

        m_mainLayout = new QHBoxLayout();
        m_mainLayout->setSpacing(0);
        m_mainLayout->setContentsMargins(0, 1, 0, 1);

        m_leftHandSideLayoutParent = new QVBoxLayout(nullptr);
        m_leftHandSideLayout = new QHBoxLayout(nullptr);
        m_leftHandSideLayoutParent->addLayout(m_leftHandSideLayout);
        m_middleLayout = new QHBoxLayout(nullptr);
        m_rightHandSideLayout = new QHBoxLayout(nullptr);

        m_leftHandSideLayout->setSpacing(0);
        m_leftHandSideLayoutParent->setSpacing(0);
        m_middleLayout->setSpacing(0);
        m_rightHandSideLayout->setSpacing(4);
        m_leftHandSideLayout->setContentsMargins(0, 0, 0, 0);
        m_leftHandSideLayoutParent->setContentsMargins(0, 0, 0, 0);
        m_middleLayout->setContentsMargins(0, 0, 0, 0);
        m_rightHandSideLayout->setContentsMargins(0, 0, 0, 0);

        m_indicatorButton = new QToolButton(this);
        m_indicatorButton->setObjectName("Indicator");
        m_indicatorButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_indicatorButton->setFixedHeight(16);
        m_indicatorButton->setFixedWidth(16);

        UpdateIndicator(nullptr);

        m_leftAreaContainer = new QWidget(this);
        m_middleAreaContainer = new QWidget(this);
        const int minimumControlWidth = 142;
        m_middleAreaContainer->setMinimumWidth(minimumControlWidth);
        m_mainLayout->addWidget(m_leftAreaContainer, LabelColumnStretch, Qt::AlignLeft);
        m_mainLayout->addWidget(m_middleAreaContainer, ValueColumnStretch);
        m_leftAreaContainer->setLayout(m_leftHandSideLayoutParent);
        m_middleAreaContainer->setLayout(m_middleLayout);

        m_middleLayout->addStretch();
        m_middleLayout->addLayout(m_rightHandSideLayout);


        m_indent = new QSpacerItem(1, 1, QSizePolicy::Fixed, QSizePolicy::Fixed);

        m_leftHandSideLayout->addWidget(m_indicatorButton);

        m_leftHandSideLayout->addSpacerItem(m_indent);

        m_nameLabel = new AzQtComponents::ElidingLabel(this);
        m_nameLabel->setObjectName("Name");

        m_defaultLabel = new QLabel(this);
        m_defaultLabel->setObjectName("DefaultLabel");
        m_middleLayout->insertWidget(0, m_defaultLabel, 1);
        m_defaultLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_defaultLabel->hide();

        m_leftHandSideLayout->addWidget(m_nameLabel);
        
        m_expanded = false;
        m_forbidExpansion = false;
        m_autoExpand = false;
        m_containerEditable = false;
        m_sourceNode = nullptr;
        m_isContainer = false;
        m_isMultiSizeContainer = false;
        m_isFixedSizeOrSmartPtrContainer = false;
        m_isSelected = false;
        m_selectionEnabled = false;
        m_readOnly = false;
        m_readOnlyDueToAttribute = false;
        m_readOnlyDueToFunction = false;
        m_initialized = false;
        m_handler = nullptr;
        m_containerSize = 0;

        setLayout(m_mainLayout);
    }

    void PropertyRowWidget::paintEvent(QPaintEvent* event)
    {
        QStylePainter p(this);

        if (CanBeReordered())
        {
            const QPen linePen(QColor(0x3B3E3F));
            p.setPen(linePen);
            int indent = m_treeDepth * m_treeIndentation;
            p.drawLine(event->rect().topLeft() + QPoint(indent, 0), event->rect().topRight());
        }
    }

    bool PropertyRowWidget::HasChildWidgetAlready() const
    {
        return m_childWidget != nullptr;
    }

    void PropertyRowWidget::Clear()
    {
        hide();
        m_treeDepth = 0;

        delete m_dropDownArrow;
        if (m_toggleSwitch != nullptr)
        {
            m_handler->DestroyGUI(m_toggleSwitch);
            m_toggleSwitch = nullptr;
        }

        if (m_childWidget)
        {
            m_handler->DestroyGUI(m_childWidget);
            m_childWidget = nullptr;
        }
        m_parentRow = nullptr;
        m_childrenRows.clear();

        AZ_Assert(m_initialized, "Cannot clear a property row unless it is initialized");
        m_sourceNode = nullptr;
        m_initialized = false;
        m_identifier = 0;
        m_containerSize = 0;
        m_expanded = false;
        m_forbidExpansion = false;
        m_autoExpand = false;
        m_containerEditable = false;
        m_isContainer = false;
        m_hadChildren = false;
        m_handler = nullptr;
        m_isMultiSizeContainer = false;
        m_isFixedSizeOrSmartPtrContainer = false;
        m_custom = false;
        if (m_selectionEnabled) 
        {
            SetSelected(false);
        }
        m_readOnly = false;
        m_readOnlyDueToAttribute = false;
        m_readOnlyDueToFunction = false;
        m_defaultValueString.clear();
        m_changeNotifiers.clear();
        m_changeValidators.clear();

        delete m_containerClearButton;
        delete m_containerAddButton;
        delete m_elementRemoveButton;

        setToolTip({});

        UpdateIndicator(nullptr);
    }

    void PropertyRowWidget::createContainerButtons()
    {
        static QIcon s_iconClear(QStringLiteral(":/stylesheet/img/UI20/delete-16.svg"));
        static QIcon s_iconAdd(QStringLiteral(":/stylesheet/img/UI20/add-16.svg"));

        if (!m_containerClearButton)
        {
            QIcon icon = QIcon(QStringLiteral(":/Cursors/Grab_release.svg"));
            this->setCursor(QCursor(icon.pixmap(16), 5, 2));

            // add those extra controls on the right hand side
            m_containerClearButton = new QToolButton(this);
            m_containerClearButton->setAutoRaise(true);
            m_containerClearButton->setIcon(s_iconClear);
            m_containerClearButton->setToolTip(tr("Remove all elements"));

            m_containerAddButton = new QToolButton(this);
            m_containerAddButton->setAutoRaise(true);
            m_containerAddButton->setIcon(s_iconAdd);
            m_containerAddButton->setToolTip(tr("Add new child element"));

            m_rightHandSideLayout->insertWidget(0, m_containerAddButton);
            m_rightHandSideLayout->insertWidget(1, m_containerClearButton);

            UpdateEnabledState();

            connect(m_containerClearButton, &QToolButton::clicked, this, &PropertyRowWidget::OnClickedClearContainerButton);
            connect(m_containerAddButton, &QToolButton::clicked, this, &PropertyRowWidget::OnClickedAddElementButton);
        }
    }


    void PropertyRowWidget::Initialize(PropertyRowWidget* pParent, InstanceDataNode* dataNode, int depth, int labelWidth)
    {
        setUpdatesEnabled(false);

        AZ_Assert(!m_initialized, "Cannot re-initialize a Property row without uninitializing it.");
        m_initialized = true;
        m_hadChildren = false;
        m_parentRow = pParent;
        m_isMultiSizeContainer = false;
        m_isSelected = false;
        m_selectionEnabled = false;
        m_changeNotifiers.clear();
        m_changeValidators.clear();
        m_containerSize = 0;
        m_defaultValueString.clear();
        m_leftAreaContainer->show();
        m_sourceNode = dataNode;
        m_treeDepth = depth;
        m_requestedLabelWidth = labelWidth;

        this->unsetCursor();

        // discover stuff about node.

        AZStd::string actualName;

        if (dataNode)
        {
            actualName = GetNodeDisplayName(*dataNode);
        }

        // If we're in a container, add our index in the container for the users visual ability to notice
        // but ALSO so that there is something unique about the element for our automatic selection and our tree rendering to hook onto
        // so that we know what elements to re-expand and contract
        SetNameLabel(actualName.c_str());

        // Reset tooltip description. If source node is valid then the description will be set when refreshing its attributes.
        SetDescription("");

        m_forbidExpansion = false;
        m_containerEditable = false;
        m_isMultiSizeContainer = false;
        m_isFixedSizeOrSmartPtrContainer = false;
        m_containerSize = 0;

        auto* container = (dataNode && dataNode->GetClassMetadata()) ? dataNode->GetClassMetadata()->m_container : nullptr;
        if (container)
        {
            m_isContainer = true;
            m_containerEditable = !container->IsFixedSize() || container->IsSmartPointer();
            m_isFixedSizeOrSmartPtrContainer = container->IsFixedSize() || container->IsSmartPointer();

            AZStd::size_t numElements = container->Size(dataNode->GetInstance(0));
            m_containerSize = AZStd::GetMax(m_containerSize, (int)numElements);

            for (AZStd::size_t pos = 1; pos < dataNode->GetNumInstances(); ++pos)
            {
                AZStd::size_t this_size = container->Size(dataNode->GetInstance(pos));
                if (this_size != numElements)
                {
                    m_forbidExpansion = true;
                    m_isMultiSizeContainer = true;
                    break;
                }
            }
        }

        if (dataNode)
        {
            AZ::Uuid typeUuid = AZ::Uuid::CreateNull();
            if (dataNode->GetClassMetadata())
            {
                typeUuid = dataNode->GetClassMetadata()->m_typeId;
            }

            m_handlerName = AZ_CRC_CE("Default");

            if (dataNode->GetElementEditMetadata())
            {
                m_handlerName = dataNode->GetElementEditMetadata()->m_elementId;
            }

            // If we're an enum type under the hood, our default handler should be the enum ComboBox handler
            if (!m_handlerName || m_handlerName == AZ_CRC_CE("Default"))
            {
                auto elementMetadata = dataNode->GetElementMetadata();
                if (elementMetadata)
                {
                    for (const auto& attributePair : elementMetadata->m_attributes)
                    {
                        if (attributePair.first == AZ::Edit::InternalAttributes::EnumType)
                        {
                            m_handlerName = AZ::Edit::UIHandlers::ComboBox;
                            break;
                        }
                    }
                }
            }

            if (m_parentRow && m_parentRow->m_isSceneSetting)
            {
                m_isSceneSetting = true;
                setStyleSheet("QFrame {background-color: #555555; color: white;}");
                setContentsMargins(0, 0, 8, 0);
                if (m_treeDepth > 0)
                {
                    m_treeDepth = m_treeDepth - 1;
                }
            }

            RefreshAttributesFromNode(true);

            // --------------------- HANDLER discovery:
            // in order to discover this, we need the property type and handler type.
            PropertyTypeRegistrationMessages::Bus::BroadcastResult(
                m_handler, &PropertyTypeRegistrationMessages::Bus::Events::ResolvePropertyHandler, m_handlerName, typeUuid);

            
            if (m_handler)
            {
                QString newNameLabel = label();
                if (m_handler->ModifyNameLabel(m_childWidget, newNameLabel))
                {
                    SetNameLabel(newNameLabel.toUtf8().constData());
                }
            }
        }

        if (m_forbidExpansion)
        {
            m_containerEditable = false;
            m_expanded = false;
        }

        UpdateDefaultLabel(dataNode);

        if (m_containerEditable)
        {
            createContainerButtons();
            // add those extra controls on the right hand side
            m_containerClearButton->setVisible(m_containerEditable && !m_isFixedSizeOrSmartPtrContainer);
            m_containerAddButton->setVisible(m_containerEditable && !m_isMultiSizeContainer && !m_isFixedSizeOrSmartPtrContainer);

            if (m_isMultiSizeContainer)
            {
                m_containerAddButton->hide();
            }

            if (m_containerSize > 0)
            {
                m_containerClearButton->setEnabled(true);
            }
            else
            {
                m_containerClearButton->setEnabled(false);
            }
        }
        else
        {
            delete m_containerClearButton;
            delete m_containerAddButton;
        }

        if ((m_parentRow) && (m_parentRow->IsContainerEditable()))
        {
            if (!m_elementRemoveButton)
            {
                QIcon icon = QIcon(QStringLiteral(":/Cursors/Grab_release.svg"));
                this->setCursor(QCursor(icon.pixmap(16), 5, 2));

                static QIcon s_iconRemove(QStringLiteral(":/stylesheet/img/UI20/delete-16.svg"));
                m_elementRemoveButton = new QToolButton(this);
                m_elementRemoveButton->setAutoRaise(true);
                m_elementRemoveButton->setIcon(s_iconRemove);
                m_elementRemoveButton->setToolTip(tr("Remove this element"));
                m_elementRemoveButton->setDisabled(m_readOnly);
                m_rightHandSideLayout->insertWidget(m_rightHandSideLayout->count(), m_elementRemoveButton);
                connect(m_elementRemoveButton, &QToolButton::clicked, this, &PropertyRowWidget::OnClickedRemoveElementButton);
                m_elementRemoveButton->setVisible(!m_parentRow->m_isFixedSizeOrSmartPtrContainer);
            }
        }
        else
        {
            delete m_elementRemoveButton;
        }

        OnValuesUpdated();

        setUpdatesEnabled(true);
    }

    void PropertyRowWidget::InitializeToggleGroup(const char* groupName, PropertyRowWidget* pParent, int depth, InstanceDataNode* node, int labelWidth)
    {
        Initialize(groupName, pParent, depth, labelWidth);
        ChangeSourceNode(node);

        // Need to invoke RefreshAttributesFromNode manually since it won't be called
        // by this version of Initialize so that any change notify (along with other attributes)
        // will be respected when toggling the group element
        RefreshAttributesFromNode(true);

        CreateGroupToggleSwitch();
    }

    void PropertyRowWidget::Initialize(const char* groupName, PropertyRowWidget* pParent, int depth, int labelWidth)
    {
        Initialize(pParent, nullptr, depth, labelWidth);

        SetNameLabel(groupName);
    }

    QString PropertyRowWidget::label() const
    {
        return m_nameLabel->text();
    }

    void PropertyRowWidget::SetNameLabel(const char* text)
    {
        QString label{ text };
        m_nameLabel->setText(label);
        m_nameLabel->setOpenExternalLinks(true);
        m_nameLabel->setVisible(!label.isEmpty());
        // setting the stretches to 0 in case of an empty label really hides the label (i.e. even the reserved space)
        m_mainLayout->setStretch(0, label.isEmpty() ? 0 : LabelColumnStretch);
        m_mainLayout->setStretch(1, label.isEmpty() ? 0 : ValueColumnStretch);
        m_identifier = AZ::Crc32(text);
    }

    void PropertyRowWidget::SetDescription(const QString& text)
    {
        setToolTip(text);
        m_nameLabel->setDescription(text);
    }

    void PropertyRowWidget::SetOverridden(bool overridden)
    {
        bool currentOverrideValue = property("IsOverridden").toBool();
        if (currentOverrideValue != overridden)
        {
            setProperty("IsOverridden", overridden);
            RefreshStyle();
        }
    }

    void PropertyRowWidget::RefreshStyle()
    {
        // This property affects styling, and this is a recycled/pooled control, so we have to ensure
        // the style is re-applied whenever dynamic properties change.

        style()->unpolish(this);
        style()->polish(this);
        update();

        AzQtComponents::StyleManager::repolishStyleSheet(m_nameLabel);
    }

    void PropertyRowWidget::OnValuesUpdated()
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (m_sourceNode)
        {
            bool showAsOverridden = false;

            // If we're a leaf in the property grid, observe comparison flags for all child data.
            if (m_handler)
            {
                AZStd::vector<InstanceDataNode*> nodeStack;
                nodeStack.reserve(m_sourceNode->GetChildren().size() + 1);
                nodeStack.push_back(m_sourceNode);
                while (!nodeStack.empty())
                {
                    InstanceDataNode* node = nodeStack.back();
                    nodeStack.pop_back();

                    if (node->IsNewVersusComparison() || node->IsDifferentVersusComparison())
                    {
                        showAsOverridden = true;
                        break;
                    }

                    for (InstanceDataNode& child : node->GetChildren())
                    {
                        nodeStack.push_back(&child);
                    }
                }
            }

            SetOverridden(showAsOverridden);
        }
        else
        {
            SetOverridden(false);
        }
    }

    void PropertyRowWidget::UpdateDefaultLabel(InstanceDataNode* dataNode)
    {
        // if there is no handler, we're going to show a default gui, just the type name.

        if (!m_handler)
        {
            if (!m_defaultValueString.empty())
            {
                m_defaultLabel->show();
                m_defaultLabel->setText(m_defaultValueString.c_str());
            }
            else
            {
                if (m_isContainer)
                {
                    m_defaultLabel->show();
                    if (m_isMultiSizeContainer)
                    {
                        m_defaultLabel->setText("(DIFFERING SIZES)");
                    }
                    else if (dataNode->GetClassMetadata()->m_container->IsSmartPointer())
                    {
                        auto classElement = dataNode->GetElementMetadata();
                        auto genericClassInfo = classElement->m_genericClassInfo;
                        AZ::SerializeContext::IDataContainer* container = dataNode->GetClassMetadata()->m_container;
                        if (genericClassInfo->GetNumTemplatedArguments() == 1)
                        {
                            void* ptrAddress = dataNode->GetInstance(0);
                            void* ptrValue = nullptr;
                            if (container->CanAccessElementsByIndex())
                            {
                                container->GetElementByIndex(ptrAddress, classElement, 0);
                            }

                            AZ::Uuid pointeeType;
                            // If the pointer is non-null, find the polymorphic type info
                            if (ptrValue)
                            {
                                auto ptrClassElement = container->GetElement(container->GetDefaultElementNameCrc());
                                pointeeType = (ptrClassElement && ptrClassElement->m_azRtti && ptrClassElement->m_azRtti->ProvidesFullRtti()) ? 
                                    ptrClassElement->m_azRtti->GetActualUuid(ptrValue) : genericClassInfo->GetTemplatedTypeId(0);
                            }
                            else
                            {
                                pointeeType = genericClassInfo->GetTemplatedTypeId(0);
                            }

                            auto elementClassData = dataNode->GetSerializeContext()->FindClassData(pointeeType);
                            AZ_Assert(elementClassData, "No class data found for type %s", pointeeType.ToString<AZStd::string>().c_str());
                            QString displayName = elementClassData->m_editData ? elementClassData->m_editData->m_name : elementClassData->m_name;
                            if (!ptrValue)
                            {
                                displayName += " (empty)";
                            }
                            m_defaultLabel->setText(displayName);
                        }
                        else
                        {
                            m_defaultLabel->setText(QString("%1 element%2").arg(m_containerSize).arg(m_containerSize > 1 ? "s" : ""));
                        }
                    }
                    else
                    {
                        m_defaultLabel->setText(QString("%1 element%2").arg(m_containerSize).arg(m_containerSize > 1 ? "s" : ""));
                    }
                }
                else
                {
                    // (leave it blank)
                    m_defaultLabel->hide();
                }
            }
        }
        else
        {
            m_defaultLabel->hide();
        }
    }

    void PropertyRowWidget::SetSelected(bool selected)
    {        
        AZ_Assert(m_selectionEnabled, "Property is not selectable");
        m_isSelected = selected;
        m_nameLabel->setProperty("selected", selected);
    }

    bool PropertyRowWidget::GetSelected()
    {
        return m_isSelected;
    }   
    
    void PropertyRowWidget::SetSelectionEnabled(bool selectionEnabled)
    {
        m_selectionEnabled = selectionEnabled;
        if (m_selectionEnabled)
        {
            installEventFilter(this);
        }
        else
        {
            removeEventFilter(this);
            if (m_isSelected)
            {
                SetSelected(false);
            }
        }
    }

    bool PropertyRowWidget::eventFilter(QObject *, QEvent *event)
    {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton)
            {
                if (mouseEvent->type() == QEvent::MouseButtonPress)
                {
                    m_clickPos = mouseEvent->pos();
                    m_clickStartTimer.restart();
                }
                else if (m_clickStartTimer.elapsed() < qApp->doubleClickInterval() / 2 && (m_clickPos - mouseEvent->pos()).manhattanLength() < 2)
                {
                    emit onRequestedSelection(m_sourceNode);
                }
            }
        }
        return false;
    }

    bool PropertyRowWidget::IsHidden(InstanceDataNode* node) const
    {
        if (node)
        {
            // Must use ResolveVisibilityAttribute otherwise class elements will be missed
            AZ::Crc32 visibility = ResolveVisibilityAttribute(*node);
            return visibility == AZ::Edit::PropertyVisibility::ShowChildrenOnly;
        }
        return false;
    }

    void PropertyRowWidget::RefreshAttributesFromNode(bool initial)
    {
        InstanceDataNode* parentNode = m_sourceNode->GetParent();
        bool isContainerElement = parentNode && parentNode->GetClassMetadata() && parentNode->GetClassMetadata()->m_container;

        if (initial && parentNode)
        {
            // First time through, see if we have any change notify on hidden parents and make sure to handle it appropriately
            InstanceDataNode* currentParent = parentNode;
            while (currentParent)
            {
                if (IsHidden(currentParent))
                {
                    if (auto editorData = currentParent->GetElementEditMetadata())
                    {
                        if (auto notifyAttribute = editorData->FindAttribute(AZ::Edit::Attributes::ChangeNotify))
                        {
                            PropertyAttributeReader reader(currentParent->FirstInstance(), notifyAttribute);
                            HandleChangeNotifyAttribute(reader, currentParent, m_changeNotifiers);
                        }
                    }
                    currentParent = currentParent->GetParent();
                }
                else
                {
                    // Need to handle derived classes here in case the base class isn't the one that is hidden
                    InstanceDataNode* nextParent = currentParent->GetParent();
                    if (nextParent && nextParent->FirstInstance() && nextParent->FirstInstance() == currentParent->FirstInstance())
                    {
                        currentParent = nextParent;
                    }
                    else
                    {
                        // We aren't a base class, so just bail
                        break;
                    }
                }
            }
        }

        if (isContainerElement)
        {
            // The way we assign attributes to members of a container is by assigning attributes with m_describesChildren
            // on the parent container. This next block is finding our parent-most container, and checking for attributes
            // that should be applied to the current node.

            // Find our parent-most container
            InstanceDataNode* containerParentNode = parentNode->GetParent();
            while (containerParentNode && containerParentNode->GetClassMetadata()->m_container)
            {
                containerParentNode = containerParentNode->GetParent();
            }

            // check for attributes that apply to this node
            if (parentNode->GetElementEditMetadata() && containerParentNode)
            {
                const auto& editData = parentNode->GetElementEditMetadata();
                for (size_t i = 0; i < editData->m_attributes.size(); ++i)
                {
                    if (editData->m_attributes[i].second->m_describesChildren)
                    {
                        if (editData->m_attributes[i].second->m_childClassOwned)
                        {
                            PropertyAttributeReader reader(m_sourceNode->FirstInstance(), editData->m_attributes[i].second);
                            ConsumeAttribute(editData->m_attributes[i].first, reader, true);
                        }
                        else
                        {
                            PropertyAttributeReader reader(containerParentNode->FirstInstance(), editData->m_attributes[i].second);
                            ConsumeAttribute(editData->m_attributes[i].first, reader, true);
                        }
                    }
                }
            }
        }

        // Tooltip may come from several sources. These are, in order of priority:
        // - "DescriptionTextOverride" attribute on the element
        // - element description
        // - "DescriptionTextOverride" attribute on the class
        // - class description
        bool foundDescription = false;
        QString newToolTip;

        if (m_sourceNode->GetElementMetadata())
        {
            auto consumeAttributes = [&](const auto& attributes)
            {
                for (size_t i = 0; i < attributes.size(); ++i)
                {
                    const auto& attrPair = attributes[i];

                    if (attrPair.second->m_describesChildren)
                    {
                        continue;
                    }

                    PropertyAttributeReader reader(m_sourceNode->GetParent()->FirstInstance(), &*attrPair.second);
                    ConsumeAttribute(attrPair.first, reader, initial, &newToolTip, &foundDescription);
                }
            };

            const AZ::SerializeContext::ClassElement* element = m_sourceNode->GetElementMetadata();
            if (element)
            {
                consumeAttributes(element->m_attributes);
            }

            const AZ::Edit::ElementData* elementEdit = m_sourceNode->GetElementEditMetadata();
            if (elementEdit)
            {
                consumeAttributes(elementEdit->m_attributes);

                if (!foundDescription && (elementEdit->m_description) && (strlen(elementEdit->m_description) > 0))
                {
                    newToolTip = elementEdit->m_description;
                    foundDescription = true;
                }
            }
        }

        UpdateDefaultLabel(m_sourceNode); // Container items display a default value

        if (m_sourceNode->GetClassMetadata())
        {
            const AZ::Edit::ClassData* classEditData = m_sourceNode->GetClassMetadata()->m_editData;
            if (classEditData)
            {
                for (const AZ::Edit::ElementData& element : classEditData->m_elements)
                {
                    if (element.m_elementId == AZ_CRC_CE("EditorData"))
                    {
                        for (const AZ::Edit::AttributePair& attrPair : element.m_attributes)
                        {
                            if (attrPair.second->m_describesChildren)
                            {
                                continue;
                            }

                            PropertyAttributeReader reader(m_sourceNode->FirstInstance(), attrPair.second);

                            if (foundDescription)
                            {
                                ConsumeAttribute(attrPair.first, reader, initial);
                            }
                            else
                            {
                                ConsumeAttribute(attrPair.first, reader, initial, &newToolTip, &foundDescription);
                            }
                        }
                    }
                }

                if (!foundDescription)
                {
                    if ((classEditData->m_description) && (strlen(classEditData->m_description) > 0))
                    {
                        newToolTip = classEditData->m_description;
                    }
                    foundDescription = true;
                }
            }
        }

        // Evaluate custom read-only function
        if (m_readOnlyQueryFunction)
        {
            m_readOnlyDueToFunction = m_readOnlyQueryFunction(m_sourceNode);
            UpdateReadOnlyState();
        }

        if (!initial && m_childWidget && m_handler)
        {
            m_handler->ModifyTooltip(m_childWidget, newToolTip);
        }

        if (m_sourceNode->IsDifferentVersusComparison())
        {
            newToolTip.append(tr(" (differs from source)"));
        }
        else if (m_sourceNode->IsNewVersusComparison())
        {
            newToolTip.append(tr(" (new versus source)"));
        }

        SetDescription(newToolTip);
    }

    void PropertyRowWidget::ConsumeChildWidget(QWidget* pChild)
    {
        AZ_Assert(m_initialized, "You must initialize a container before you can embed a child widget");
        m_childWidget = pChild;

        m_middleLayout->insertWidget(0, pChild, 1);
        pChild->show();

        m_childWidget->setDisabled(m_readOnly);

        QWidget* pfirstTarget = m_handler->GetFirstInTabOrder_Internal(pChild);
        if (pfirstTarget)
        {
            if ((m_dropDownArrow) && (m_dropDownArrow->isVisible()))
            {
                setTabOrder(m_dropDownArrow, pfirstTarget);
            }
        }

        QString newNameLabel = label();
        if (m_handler->ModifyNameLabel(m_childWidget, newNameLabel))
        {
            SetNameLabel(newNameLabel.toUtf8().constData());
        }

        QString newToolTip = toolTip();
        if (m_handler->ModifyTooltip(m_childWidget, newToolTip))
        {
            SetDescription(newToolTip);
        }
    }

    template<typename T>
    T* ValidateAndCastAttributeFunction(AZ::Edit::Attribute* attribute, InstanceDataNode* invocationTarget)
    {
        // Verify type safety for member function handlers.
        T* castedAttributeFunction = azdynamic_cast<T*>(attribute);

        const AZ::Uuid handlerTypeId = castedAttributeFunction ? castedAttributeFunction->GetInstanceType() : AZ::Uuid::CreateNull();

        if (handlerTypeId.IsNull())
        {
            return nullptr;
        }

        if (!invocationTarget)
        {
            AZ_Warning("Property Editor", false, "Attribute handler (%s) set for row with no invocation target!", castedAttributeFunction->RTTI_GetTypeName());
        }
        else if (invocationTarget->GetClassMetadata()->m_azRtti)
        {
            if (!invocationTarget->GetClassMetadata()->m_azRtti->IsTypeOf(handlerTypeId))
            {
                AZ_Warning("Property Editor", false, "Instance (%s) has RTTI, but does not derive from type (%s) expected by the handler.", invocationTarget->GetClassMetadata()->m_name, castedAttributeFunction->RTTI_GetTypeName());
                return nullptr;
            }
        }
        else if (handlerTypeId != invocationTarget->GetClassMetadata()->m_typeId)
        {
            AZ_Warning("Property Editor", false, "Instance does not have RTTI, and is not the type expected by the handler (%s).", castedAttributeFunction->RTTI_GetTypeName());
            return nullptr;
        }

        return castedAttributeFunction;
    }

    void PropertyRowWidget::ConsumeAttribute(AZ::u32 attributeName, PropertyAttributeReader& reader, bool initial,
        QString* descriptionOut/*=nullptr*/, bool* foundDescriptionOut/*=nullptr*/)
    {
        // Attribute types you are allowed to update at runtime
        if (attributeName == AZ_CRC_CE("ForbidExpansion"))
        {
            m_forbidExpansion = true;
        }
        else if (attributeName == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (reader.Read<bool>(value))
            {
                m_readOnlyDueToAttribute = value;
                UpdateReadOnlyState();
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("Property Editor", false, "Failed to read 'ReadOnly' attribute from property");
            }
        }
        else if (attributeName == AZ::Edit::Attributes::NameLabelOverride)
        {
            AZStd::string labelText;
            if (reader.Read<AZStd::string>(labelText))
            {
                SetNameLabel(labelText.c_str());
            }
        }
        else if (attributeName == AZ::Edit::Attributes::ContainerReorderAllow)
        {
            if (m_containerEditable)
            {
                reader.Read<bool>(m_reorderAllow);
            }
        }
        else if (attributeName == AZ::Edit::Attributes::DescriptionTextOverride)
        {
            AZStd::string labelText;
            if (reader.Read<AZStd::string>(labelText))
            {
                if (descriptionOut)
                {
                    *descriptionOut = labelText.c_str();
                    if (foundDescriptionOut)
                    {
                        *foundDescriptionOut = true;
                    }
                }
            }
        }
        else if (attributeName == AZ::Edit::Attributes::ValueText)
        {
            m_defaultValueString.clear();
            reader.Read<AZStd::string>(m_defaultValueString);
            if ((!initial) && (!m_handler))
            {
                m_defaultLabel->setText(m_defaultValueString.c_str());
                m_defaultLabel->setVisible(!m_defaultValueString.empty());
            }
        }
        else if (attributeName == AZ::Edit::Attributes::AutoExpand)
        {
            m_autoExpand = true;
            reader.Read<bool>(m_autoExpand);
        }
        else if (attributeName == AZ::Edit::Attributes::ForceAutoExpand)
        {
            m_forceAutoExpand = false; // Does not always expand by default
            reader.Read<bool>(m_forceAutoExpand);
        }
        else if (attributeName == AZ::Edit::Attributes::CategoryStyle)
        {
            //! The "display divider" attribute is used to separate the header of a group from the rest of the body in the scene settings tool,
            //! Set the style of this row to emulate the style of a component card header,
            //! And notify the PropertyRowWidget that we are parsing over the scene settings reflection data.
            AZStd::string categoryAttributeValue;
            reader.Read<AZStd::string>(categoryAttributeValue);
            if (categoryAttributeValue.compare("display divider") == 0)
            {
                m_isSceneSetting = true;
                setContentsMargins(8, 0, 4, 0);
                setStyleSheet("QFrame {background-color: #333333; margin-top: 5px; border-top-right-radius: 2px; border-top-left-radius: 2px;}");
            }
        }
        // Attribute types you are NOT allowed to update at runtime
        else if ((initial) && (attributeName == AZ::Edit::Attributes::ContainerCanBeModified))
        {
            if (m_sourceNode->GetClassMetadata()->m_container)
            {
                reader.Read<bool>(m_containerEditable);
            }
        }
        else if ((initial) && (attributeName == AZ_CRC_CE("Handler")))
        {
            AZ::u32 elementId = 0;
            if (reader.Read<AZ::u32>(elementId))
            {
                m_handlerName = elementId;
            }
        }
        else if ((initial) && (attributeName == AZ::Edit::Attributes::ChangeNotify))
        {
            HandleChangeNotifyAttribute(reader, m_sourceNode ? m_sourceNode->GetParent() : nullptr, m_changeNotifiers);
        }
        else if ((initial) && (attributeName == AZ::Edit::Attributes::ChangeValidate))
        {
            HandleChangeValidateAttribute(reader, m_sourceNode ? m_sourceNode->GetParent() : nullptr);
        }
        else if ((initial) && (attributeName == AZ::Edit::Attributes::StringLineEditingCompleteNotify))
        {
            HandleChangeNotifyAttribute(reader, m_sourceNode ? m_sourceNode->GetParent() : nullptr, m_editingCompleteNotifiers);
        }
    }

    void PropertyRowWidget::SetReadOnlyQueryFunction(const ReadOnlyQueryFunction& readOnlyQueryFunction)
    {
        m_readOnlyQueryFunction = readOnlyQueryFunction;
    }

    InstanceDataNode* PropertyRowWidget::ResolveToNodeByType(InstanceDataNode* startNode, const AZ::Uuid& typeId) const
    {
        InstanceDataNode* targetNode = startNode;

        if (!typeId.IsNull())
        {
            // Walk up the chain looking for the first correct class type to handle the callback
            while (targetNode)
            {
                if (targetNode->GetClassMetadata()->m_azRtti)
                {
                    if (targetNode->GetClassMetadata()->m_azRtti->IsTypeOf(typeId))
                    {
                        // Instance has RTTI, and derives from type expected by the handler.
                        break;
                    }
                }
                else
                {
                    if (typeId == targetNode->GetClassMetadata()->m_typeId)
                    {
                        // Instance does not have RTTI, and is the type expected by the handler.
                        break;
                    }
                }
                targetNode = targetNode->GetParent();
            }
        }

        return targetNode;
    }

    void PropertyRowWidget::HandleChangeValidateAttribute(PropertyAttributeReader& reader, InstanceDataNode* node)
    {
        // Verify type safety for member function handlers.
        // ChangeValidate handlers are invoked on the instance associated with the node owning
        // the field, but attributes are generically propagated to parents as well, which is not
        // safe for ChangeValidate events bound to member functions.
        // Here we'll avoid inheriting child ChangeValidate attributes unless it's actually
        // type-safe to do so.
        AZ::Attribute* functionObjectAttr = reader.GetAttribute();

        const AZ::Uuid handlerTypeId =
            functionObjectAttr != nullptr ? functionObjectAttr->GetPotentialClassTypeId() : AZ::Uuid::CreateNull();
        InstanceDataNode* targetNode = ResolveToNodeByType(node, handlerTypeId);

        if (targetNode)
        {
            m_changeValidators.emplace_back(targetNode, reader.GetAttribute());
        }

    }


    void PropertyRowWidget::HandleChangeNotifyAttribute(PropertyAttributeReader& reader, InstanceDataNode* node, AZStd::vector<ChangeNotification>& notifiers)
    {
        // Verify type safety for member function handlers.
        // ChangeNotify handlers are invoked on the instance associated with the node owning
        // the field, but attributes are generically propagated to parents as well, which is not
        // safe for ChangeNotify events bound to member functions.
        // Here we'll avoid inheriting child ChangeNotify attributes unless it's actually
        // type-safe to do so.
        AZ::Edit::AttributeFunction<void()>* funcVoid = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(reader.GetAttribute());
        AZ::Edit::AttributeFunction<AZ::u32()>* funcU32 = azdynamic_cast<AZ::Edit::AttributeFunction<AZ::u32()>*>(reader.GetAttribute());
        AZ::Edit::AttributeFunction<AZ::Crc32()>* funcCrc32 = azdynamic_cast<AZ::Edit::AttributeFunction<AZ::Crc32()>*>(reader.GetAttribute());

        const AZ::Uuid handlerTypeId = funcVoid ? funcVoid->GetInstanceType() : (funcU32 ? funcU32->GetInstanceType() : (funcCrc32 ? funcCrc32->GetInstanceType() : AZ::Uuid::CreateNull()));
        InstanceDataNode* targetNode = node;

        if (!handlerTypeId.IsNull())
        {
            // Walk up the chain looking for the first correct class type to handle the callback
            while (targetNode)
            {
                if (targetNode->GetClassMetadata()->m_azRtti)
                {
                    if (targetNode->GetClassMetadata()->m_azRtti->IsTypeOf(handlerTypeId))
                    {
                        // Instance has RTTI, and derives from type expected by the handler.
                        break;
                    }
                }
                else
                {
                    if (handlerTypeId == targetNode->GetClassMetadata()->m_typeId)
                    {
                        // Instance does not have RTTI, and is the type expected by the handler.
                        break;
                    }
                }
                targetNode = targetNode->GetParent();
            }
        }

        if (targetNode)
        {
            notifiers.emplace_back(targetNode, reader.GetAttribute());
        }
    }

    void PropertyRowWidget::AddedChild(PropertyRowWidget* child)
    {
        m_childrenRows.push_back(child);
        m_hadChildren = true;
    }

    void PropertyRowWidget::UpdateDropDownArrow()
    {
        if (m_custom)
        {
            return;
        }
        if ((!m_hadChildren) || (m_forbidExpansion) || m_forceAutoExpand)
        {
            if (m_dropDownArrow)
            {
                m_dropDownArrow->hide();
            }
            SetIndentSize(m_treeDepth * m_treeIndentation + m_leafIndentation);
        }
        else
        {
            if (!m_dropDownArrow)
            {
                m_dropDownArrow = new QCheckBox(this);
                AzQtComponents::CheckBox::applyExpanderStyle(m_dropDownArrow);
                m_leftHandSideLayout->insertWidget(2, m_dropDownArrow);
                connect(m_dropDownArrow, &QCheckBox::clicked, this, &PropertyRowWidget::OnClickedExpansionButton);
            }
            m_dropDownArrow->show();
            SetIndentSize(m_treeDepth * m_treeIndentation);
            m_dropDownArrow->setChecked(m_expanded);
        }
    }

    void PropertyRowWidget::CreateGroupToggleSwitch()
    {
        if (m_toggleSwitch == nullptr)
        {
            m_handlerName = AZ::Edit::UIHandlers::CheckBox;
            PropertyTypeRegistrationMessages::Bus::BroadcastResult(m_handler, &PropertyTypeRegistrationMessages::Bus::Events::ResolvePropertyHandler, m_handlerName, azrtti_typeid<bool>());
            m_toggleSwitch = m_handler->CreateGUI(this);
            m_middleLayout->insertWidget(0, m_toggleSwitch, 1);
            auto checkBoxCtrl = static_cast<AzToolsFramework::PropertyCheckBoxCtrl*>(m_toggleSwitch);
            QObject::connect(checkBoxCtrl, &AzToolsFramework::PropertyCheckBoxCtrl::valueChanged, this, &PropertyRowWidget::OnClickedToggleButton);
        }
    }

    void PropertyRowWidget::SetIndentSize(int w)
    {
        m_indent->changeSize(w, 1, QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_leftHandSideLayout->invalidate();
        m_leftHandSideLayout->update();
        m_leftHandSideLayout->activate();
    }

    void PropertyRowWidget::OnClickedToggleButton(bool checked)
    {
        if (m_expanded != checked)
        {
            DoExpandOrContract(!IsExpanded(), 0 != (QGuiApplication::keyboardModifiers() & Qt::ControlModifier));
        }
    }

    void PropertyRowWidget::ChangeSourceNode(InstanceDataNode* node)
    {
        m_sourceNode = node;
    }

    void PropertyRowWidget::SetExpanded(bool expanded)
    {
        m_expanded = expanded;

        UpdateDropDownArrow();
    }

    AZ::u32 PropertyRowWidget::GetIdentifier() const
    {
        AZ_Assert(m_initialized, "You may not call GetIdentifier on uninitialized rows.");
        return m_identifier;
    }

    bool PropertyRowWidget::IsForbidExpansion() const
    {
        AZ_Assert(m_initialized, "You may not call IsForbidExpansion on uninitialized rows.");
        return m_forbidExpansion;
    }

    bool PropertyRowWidget::ForceAutoExpand() const
    {
        AZ_Assert(m_initialized, "You may not call ForceAutoExpand on uninitialized rows.");
        return m_forceAutoExpand;
    }

    PropertyHandlerBase* PropertyRowWidget::GetHandler() const
    {
        AZ_Assert(m_initialized, "You may not call GetHandler on uninitialized rows.");
        return m_handler;
    }

    PropertyRowWidget::~PropertyRowWidget()
    {
    }

    void PropertyRowWidget::OnClickedExpansionButton()
    {
        if (!m_forbidExpansion && m_dropDownArrow && m_dropDownArrow->isVisible())
        {
            DoExpandOrContract(!IsExpanded(), 0 != (QGuiApplication::keyboardModifiers() & Qt::ControlModifier));
        }
    }

    PropertyModificationRefreshLevel PropertyRowWidget::DoPropertyNotify(size_t optionalIndex /*= 0*/)
    {
        // Notify from this node all the way up through parents recursively.
        // Maintain the highest level of requested refresh along the way.

        PropertyModificationRefreshLevel level = Refresh_None;
        if ((m_changeNotifiers.size() > 0) && (m_sourceNode))
        {
            for (size_t changeIndex = 0; changeIndex < m_changeNotifiers.size(); changeIndex++)
            {
                // execute the function or read the value.
                InstanceDataNode* nodeToNotify = m_changeNotifiers[changeIndex].m_node;
                if ((nodeToNotify) && (nodeToNotify->GetClassMetadata()->m_container))
                {
                    nodeToNotify = nodeToNotify->GetParent();
                }

                if (nodeToNotify)
                {
                    for (size_t idx = 0; idx < nodeToNotify->GetNumInstances(); ++idx)
                    {
                        PropertyAttributeReader reader(nodeToNotify->GetInstance(idx), m_changeNotifiers[changeIndex].m_attribute);
                        AZ::u32 refreshLevelCRC = 0;
                        if (!reader.Read<AZ::u32>(refreshLevelCRC))
                        {
                            AZ::Crc32 refreshLevelCrc32;
                            if (reader.Read<AZ::Crc32>(refreshLevelCrc32))
                            {
                                refreshLevelCRC = refreshLevelCrc32;
                            }
                        }

                        if (refreshLevelCRC != 0)
                        {
                            if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::None)
                            {
                                level = AZStd::GetMax(Refresh_None, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::ValuesOnly)
                            {
                                level = AZStd::GetMax(Refresh_Values, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::AttributesAndValues)
                            {
                                level = AZStd::GetMax(Refresh_AttributesAndValues, level);
                            }
                            else if (refreshLevelCRC == AZ::Edit::PropertyRefreshLevels::EntireTree)
                            {
                                level = AZStd::GetMax(Refresh_EntireTree, level);
                            }
                            else
                            {
                                AZ_WarningOnce("Property Editor", false,
                                    "Invalid value returned from change notification handler for %s. "
                                    "A CRC of one of the following refresh levels should be returned: "
                                    "RefreshNone, RefreshValues, RefreshAttributesAndValues, RefreshEntireTree.",
                                    nodeToNotify->GetElementEditMetadata() ? nodeToNotify->GetElementEditMetadata()->m_name : nodeToNotify->GetClassMetadata()->m_name);
                            }
                        }
                        else
                        {
                            // Support invoking a void handler (either taking no parameters or an index)
                            // (doesn't return a refresh level)
                            AZ::Edit::AttributeFunction<void()>* func =
                                azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(reader.GetAttribute());
                            AZ::Edit::AttributeFunction<void(size_t)>* funcWithIndex =
                                azdynamic_cast<AZ::Edit::AttributeFunction<void(size_t)>*>(reader.GetAttribute());

                            if (func)
                            {
                                func->Invoke(nodeToNotify->GetInstance(idx));
                            }
                            else if (funcWithIndex)
                            {
                                // if a function has been provided that takes an index, use this version
                                // passing through the index of the element being modified
                                funcWithIndex->Invoke(nodeToNotify->GetInstance(idx), optionalIndex);
                            }
                            else
                            {
                                AZ_WarningOnce("Property Editor", false,
                                    "Unable to invoke change notification handler for %s. "
                                    "Handler must return void, or the CRC of one of the following refresh levels: "
                                    "RefreshNone, RefreshValues, RefreshAttributesAndValues, RefreshEntireTree.",
                                    nodeToNotify->GetElementEditMetadata() ? nodeToNotify->GetElementEditMetadata()->m_name : nodeToNotify->GetClassMetadata()->m_name);
                            }
                        }
                    }
                }
            }
        }

        if (m_parentRow)
        {
            return (PropertyModificationRefreshLevel)AZStd::GetMax((int)m_parentRow->DoPropertyNotify(optionalIndex), (int)level);
        }

        return level;
    }

    void PropertyRowWidget::DoEditingCompleteNotify()
    {
        if ((m_editingCompleteNotifiers.size() > 0) && (m_sourceNode))
        {
            for (size_t changeIndex = 0; changeIndex < m_editingCompleteNotifiers.size(); changeIndex++)
            {
                // execute the function or read the value.
                InstanceDataNode* nodeToNotify = m_editingCompleteNotifiers[changeIndex].m_node;
                if ((nodeToNotify) && (nodeToNotify->GetClassMetadata()->m_container))
                {
                    nodeToNotify = nodeToNotify->GetParent();
                }

                if (nodeToNotify)
                {
                    for (size_t idx = 0; idx < nodeToNotify->GetNumInstances(); ++idx)
                    {
                        PropertyAttributeReader reader(nodeToNotify->GetInstance(idx), m_editingCompleteNotifiers[changeIndex].m_attribute);
                        
                        // Support invoking a void handler
                        AZ::Edit::AttributeFunction<void()>* func = azdynamic_cast<AZ::Edit::AttributeFunction<void()>*>(reader.GetAttribute());
                        if (func)
                        {
                            func->Invoke(nodeToNotify->GetInstance(idx));
                        }
                        else
                        {
                            AZ_WarningOnce("Property Editor", false,
                                "Unable to invoke editing complete notification handler for %s. "
                                "Handler must return void",
                                nodeToNotify->GetElementEditMetadata() ? nodeToNotify->GetElementEditMetadata()->m_name : nodeToNotify->GetClassMetadata()->m_name);
                        }
                    }
                }
            }
        }
    }

    int PropertyRowWidget::GetLevel() const
    {
        int level = 0;

        for (PropertyRowWidget* parent = GetParentRow(); parent; parent = parent->GetParentRow())
        {
            level++;
        }

        return level;
    }

    bool PropertyRowWidget::IsTopLevel() const
    {
        // We're a top level row if we're the first ancestor in the hierarchy with a visible, non-empty label
        auto canBeTopLevel = [](const PropertyRowWidget* widget)
        {
            if (widget->m_nameLabel->isHidden())
            {
                return false;
            }
            QString label = widget->m_nameLabel->text();
            return !label.isEmpty();
        };

        for (PropertyRowWidget* parent = GetParentRow(); parent; parent = parent->GetParentRow())
        {
            if (canBeTopLevel(parent))
            {
                return false;
            }
        }
        return canBeTopLevel(this);
    }

    bool PropertyRowWidget::GetAppendDefaultLabelToName()
    {
        return false;
    }

    void PropertyRowWidget::AppendDefaultLabelToName(bool doAppend)
    {
        if (!m_sourceNode || !doAppend || !m_isContainer)
        {
            return;
        }

        // Move the number of elements text to after the name.
        m_defaultLabel->hide();
        m_nameLabel->setText(QString("%1 (%2)").arg(GetNodeDisplayName(*m_sourceNode).c_str()).arg(QString("%1 elements").arg(m_containerSize)));

        // Give the name more room to prevent elision.
        m_defaultLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

        m_middleAreaContainer->setMinimumWidth(0);

        m_mainLayout->setStretch(0, 1);
        m_mainLayout->setStretch(1, 0);
    }

    bool PropertyRowWidget::HasChildRows() const
    {
        return !m_childrenRows.empty(); 
    }

    AZ::u32 PropertyRowWidget::GetChildRowCount() const
    {
        return static_cast<AZ::u32>(m_childrenRows.size());
    }

    PropertyRowWidget* PropertyRowWidget::GetChildRowByIndex(AZ::u32 index) const
    {
        if (index >= m_childrenRows.size())
        {
            return nullptr;
        }

        return m_childrenRows[index];
    }

    bool PropertyRowWidget::ShouldPreValidatePropertyChange() const
    {
        return (m_changeValidators.size() > 0);
    }

    bool PropertyRowWidget::ValidatePropertyChange(void* valueToValidate, const AZ::Uuid& valueType) const
    {
        if (m_sourceNode)
        {
            for (auto& changeValidator : m_changeValidators)
            {
                // execute the function or read the value.
                InstanceDataNode* nodeToNotify = changeValidator.m_node;
                if ((nodeToNotify) && (nodeToNotify->GetClassMetadata()->m_container))
                {
                    nodeToNotify = nodeToNotify->GetParent();
                }

                if (nodeToNotify)
                {
                    // The ChangeValidate attribute stores a lambda function accepts the actual C++ class type as the first argument
                    // however, the AZ::AttributeReader supplies a void* for the class type
                    // and assumes that the void* is of the instance of a class type.
                    // For example the ChangeValidate attribute associated with a member function such as
                    // `TerrainWorldConfig::ValidateHeightMin` would store a lambda in an AttributeInvocable with the following signature
                    // `AZ::Outcome<void, AZStd::string> (*)(TerrainWorldConfig*, void* Uuid)`
                    // The AZ::AttributeReader is technically unsafe for reading an attribute as it accepts a void* for the instance
                    // Internally the Attribute Get() or Invoke() member functions reinterpret_cast the void* to the template class type
                    // 
                    // In order to support this case, the AttributeInvocable provides the `GetVoidInstanceAttributeInvocable` function
                    // which wraps the original function object in a lambda that accepts the same parameters, except for the the first parameter.
                    // The first parameter type is replaced with `void*` and the function itself performs a reinterpret_cast of that void*
                    // to the original type of the first parameter
                    // For the example case, the `GetVoidInstanceAttributeInvocable` function takes the
                    // `AZ::Outcome<void, AZStd::string> (*)(TerrainWorldConfig*, void* Uuid)` lambda and then wraps it in another lambda
                    // with the signature of `AZ::Outcome<void, AZStd::string> (*)(void*, void* Uuid)` where "TerrainWorldConfig*" has been
                    // replaced with "void*"
                    AZ::AttributeUniquePtr attributeForVoidInstance = changeValidator.m_attribute->GetVoidInstanceAttributeInvocable();
                    for (size_t idx = 0; idx < nodeToNotify->GetNumInstances(); ++idx)
                    {
                        PropertyAttributeReader reader(nodeToNotify->GetInstance(idx), attributeForVoidInstance.get());
                        AZ::Outcome<void, AZStd::string> outcome;
                        if (reader.Read<AZ::Outcome<void, AZStd::string>>(outcome, valueToValidate, valueType))
                        {
                            if (!outcome.IsSuccess())
                            {
                                QMessageBox::warning(AzToolsFramework::GetActiveWindow(), "Invalid Assignment", outcome.GetError().c_str(), QMessageBox::Ok);
                                return false;
                            }
                        }
                    }
                }
            }
        }

        if (m_parentRow)
        {
            if (!m_parentRow->ValidatePropertyChange(valueToValidate, valueType))
            {
                return false;
            }
        }

        return true;
    }

    QWidget* PropertyRowWidget::GetFirstTabWidget()
    {
        if ((m_dropDownArrow != nullptr) && (m_dropDownArrow->isVisible()))
        {
            return m_dropDownArrow;
        }

        if ((m_childWidget) && (m_handler))
        {
            QWidget* target = m_handler->GetFirstInTabOrder_Internal(m_childWidget);
            if ((target) && (target->focusPolicy() != Qt::NoFocus))
            {
                return target;
            }
        }

        return nullptr;
    }

    void PropertyRowWidget::UpdateWidgetInternalTabbing()
    {
        if ((m_childWidget) && (m_handler))
        {
            m_handler->UpdateWidgetInternalTabbing_Internal(m_childWidget);
        }
    }

    QWidget* PropertyRowWidget::GetLastTabWidget()
    {
        if ((m_childWidget) && (m_handler))
        {
            QWidget* target = m_handler->GetLastInTabOrder_Internal(m_childWidget);
            if ((target) && (target->focusPolicy() != Qt::NoFocus))
            {
                return target;
            }
        }
        else
        {
            // we have no child widget, its going to be either the drop down arrow or the container buttons.
            if ((m_dropDownArrow) && (m_dropDownArrow->isVisible()))
            {
                return m_dropDownArrow;
            }
        }

        return nullptr;
    }

    int PropertyRowWidget::CalculateLabelWidth() const
    {
        if (m_defaultLabel->isHidden() && m_nameLabel->isHidden())
        {
            return (m_treeDepth * m_treeIndentation) + m_leafIndentation;
        }
        else
        {
            return m_requestedLabelWidth;
        }
    }

    QSize PropertyRowWidget::LabelSizeHint() const
    {
        return m_leftHandSideLayout->sizeHint();
    }

    void PropertyRowWidget::SetLabelWidth(int width)
    {
        m_requestedLabelWidth = width;
    }

    void PropertyRowWidget::SetTreeIndentation(int indentation)
    {
        m_treeIndentation = indentation;
    }

    void PropertyRowWidget::SetLeafIndentation(int indentation)
    {
        m_leafIndentation = indentation;
    }

    void PropertyRowWidget::OnClickedAddElementButton()
    {
        // add an element to all
        if (!IsContainerEditable())
        {
            return;
        }

        if (m_isMultiSizeContainer)
        {
            return;
        }

        // add an element to EVERY container.
        emit onRequestedContainerAdd(m_sourceNode);
    }

    void PropertyRowWidget::OnClickedRemoveElementButton()
    {
        if (!m_parentRow)
        {
            return;
        }

        if (!m_parentRow->IsContainerEditable())
        {
            return;
        }

        emit onRequestedContainerElementRemove(m_sourceNode);
    }

    void PropertyRowWidget::OnClickedClearContainerButton()
    {
        // add an element to all
        if (!IsContainerEditable())
        {
            return;
        }

        emit onRequestedContainerClear(m_sourceNode);
    }

    void PropertyRowWidget::OnContextMenuRequested(const QPoint& point)
    {
        const QPoint globalPoint = mapToGlobal(point);
        emit onRequestedContextMenu(m_sourceNode, globalPoint);
    }

    void PropertyRowWidget::contextMenuEvent(QContextMenuEvent* event)
    {
        emit onRequestedContextMenu(m_sourceNode, event->globalPos());
        event->accept();
    }

    void PropertyRowWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::LeftButton)
        {
            OnClickedExpansionButton();
        }

        QWidget::mouseDoubleClickEvent(event);
    }

    void PropertyRowWidget::DoExpandOrContract(bool expand, bool includeDescendents /*= false*/)
    {
        setUpdatesEnabled(false);

        AZStd::vector<PropertyRowWidget*> expandWidgets;
        expandWidgets.push_back(this);

        while (!expandWidgets.empty())
        {
            PropertyRowWidget* widget = expandWidgets.back();
            expandWidgets.pop_back();

            widget->SetExpanded(expand);
            emit onUserExpandedOrContracted(m_sourceNode, m_expanded);

            if (includeDescendents)
            {
                for (PropertyRowWidget* child : widget->GetChildrenRows())
                {
                    expandWidgets.push_back(child);
                }
            }
        }

        setUpdatesEnabled(true);
    }

    void PropertyRowWidget::HideContent()
    {
        SetExpanded(true);
        m_leftAreaContainer->hide();

        if (m_dropDownArrow)
        {
            m_dropDownArrow->hide();
        }

        if (m_containerClearButton)
        {
            m_containerClearButton->hide();
        }

        if (m_containerAddButton)
        {
            m_containerAddButton->hide();
        }

        if (m_elementRemoveButton)
        {
            m_elementRemoveButton->hide();
        }

        m_nameLabel->hide();
        m_defaultLabel->hide();
    }

    void PropertyRowWidget::UpdateReadOnlyState()
    {
        bool prevReadOnly = m_readOnly;
        m_readOnly = m_readOnlyDueToAttribute || m_readOnlyDueToFunction;
        if (prevReadOnly != m_readOnly)
        {
            UpdateEnabledState();
        }
    }

    void PropertyRowWidget::UpdateEnabledState()
    {
        if (m_containerClearButton)
        {
            m_containerClearButton->setEnabled(!m_readOnly && m_containerEditable && m_containerSize > 0);
        }

        if (m_containerAddButton)
        {
            m_containerAddButton->setDisabled(m_readOnly);
        }

        if (m_elementRemoveButton)
        {
            m_elementRemoveButton->setDisabled(m_readOnly);
        }

        if (m_childWidget)
        {
            m_childWidget->setDisabled(m_readOnly);
        }
    }

    void PropertyRowWidget::UpdateIndicator(const char* imagePath)
    {
        bool fileExists = false;

        if (imagePath != nullptr && imagePath[0] != 0)
        {
            fileExists = QFile::exists(imagePath);
            Q_ASSERT(fileExists && "An invalid file path was specified as an indicator!");
        }

        if (!fileExists)
        {
            // Hide the indicator
            m_indicatorButton->setVisible(false);
        }
        else
        {
            QPixmap pixmap(imagePath);
            m_indicatorButton->setIcon(pixmap);
            m_indicatorButton->setVisible(true);
        };
    }

    void PropertyRowWidget::SetFilterString(const AZStd::string& str)
    {
        m_currentFilterString = str.c_str();
        m_nameLabel->setFilter(m_currentFilterString);
    }

    bool PropertyRowWidget::CanChildrenBeReordered() const
    {
        return m_containerEditable;
    }

    bool PropertyRowWidget::CanBeReordered() const
    {
        return m_parentRow && m_parentRow->m_reorderAllow && m_parentRow->CanChildrenBeReordered();
    }

    int PropertyRowWidget::GetIndexInParent() const
    {
        if (!GetParentRow())
        {
            return -1;
        }

        for (AZ::u32 index = 0; index < GetParentRow()->GetChildRowCount(); index++)
        {
            if (GetParentRow()->GetChildrenRows()[index] == this)
            {
                return index;
            }
        }

        return -1;
    }

    bool PropertyRowWidget::CanMoveUp() const
    {
        if (!CanBeReordered())
        {
            return false;
        }

        return this != m_parentRow->GetChildRowByIndex(0);
    }

    bool PropertyRowWidget::CanMoveDown() const
    {
        if (!CanBeReordered())
        {
            return false;
        }

        AZ::u32 numChildrenOfParent = m_parentRow->GetChildRowCount();

        return this != m_parentRow->GetChildRowByIndex(numChildrenOfParent - 1);
    }

    int PropertyRowWidget::GetContainingEditorFrameWidth()
    {
        QWidget* parent = parentWidget();

        // Find the first ancestor that can be cast to a QFrame, this will be the RPE.
        while (!qobject_cast<QFrame*>(parent))
        {
            parent = parent->parentWidget();
        }

        if (!parent)
        {
            return 0;
        }

        // The parent of the RPE is the size we want.
        parent = parent->parentWidget();
        
        return parent->rect().width();
    }

    int PropertyRowWidget::GetHeightOfRowAndVisibleChildren()
    {
        int height = rect().height();

        if (!GetChildRowCount() || !IsExpanded())
        {
            return height;
        }

        for (auto childRow : GetChildrenRows())
        {
            height += childRow->GetHeightOfRowAndVisibleChildren();
        }

        return height;
    }

    int PropertyRowWidget::DrawDragImageAndVisibleChildrenInto(QPainter& painter, int xpos, int ypos)
    {
        // Render our image into the given painter.
        int ystart = ypos;

        render(&painter, QPoint(xpos, ypos));

        if (!GetChildRowCount() || !IsExpanded())
        {
            return rect().height();
        }

        ypos += rect().height();

        // Recursively draw any children.
        for (auto childRow : GetChildrenRows())
        {
            ypos += childRow->DrawDragImageAndVisibleChildrenInto(painter, xpos, ypos);
        }

        return ypos - ystart;
    }

    QPixmap PropertyRowWidget::createDragImage(
        const QColor backgroundColor, const QColor borderColor, const float alpha, DragImageType imageType)
    {
        // Make the drag box as wide as the containing editor minus a gap each side for the border.
        static constexpr int ParentEditorBorderSize = 2;
        int width = GetContainingEditorFrameWidth() - ParentEditorBorderSize * 2;
        int height = 0;

        if (imageType == DragImageType::IncludeVisibleChildren)
        {
            height = GetHeightOfRowAndVisibleChildren();
        }
        else
        {
            height = rect().height();
        }

        const auto dpr = devicePixelRatioF();
        QPixmap dragImage(static_cast<int>(width * dpr), static_cast<int>(height * dpr));
        dragImage.setDevicePixelRatio(dpr);
        dragImage.fill(Qt::transparent);

        QRect imageRect = QRect(0, 0, width, height);

        QPainter dragPainter(&dragImage);
        dragPainter.setCompositionMode(QPainter::CompositionMode_Source);
        dragPainter.fillRect(imageRect, Qt::transparent);
        dragPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        dragPainter.setOpacity(alpha);
        dragPainter.fillRect(imageRect, backgroundColor);

        dragPainter.setOpacity(1.0f);

        int marginWidth = (imageRect.width() - rect().width()) / 2 + ParentEditorBorderSize - 1;

        if (imageType == DragImageType::IncludeVisibleChildren)
        {
            DrawDragImageAndVisibleChildrenInto(dragPainter, marginWidth, 0);
        }
        else
        {
            render(&dragPainter, QPoint(marginWidth, 0));
        }

        QPen pen;
        pen.setColor(QColor(borderColor));
        pen.setWidth(1);
        dragPainter.setPen(pen);
        dragPainter.drawRect(0, 0, imageRect.width() - 1, imageRect.height() - 1);

        dragPainter.end();

        return dragImage;
    }
}

#include "UI/PropertyEditor/moc_PropertyRowWidget.cpp"
