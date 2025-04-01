/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/PropertyWidgets/MotionSetMotionIdHandler.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionSetSelectionWindow.h>
#include <Editor/AnimGraphEditorBus.h>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QLocale>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>

namespace EMotionFX
{
    float MotionSelectionIdWidgetController::s_displayedRoundingError = 0.0f;

    AZ_CLASS_ALLOCATOR_IMPL(MotionSetMotionIdPicker, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(MotionIdRandomSelectionWeightsHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(MotionSetMultiMotionIdHandler, EditorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(MotionSelectionIdWidgetController, EditorAllocator)

    const float MotionSetMotionIdPicker::s_defaultWeight = 1.0f;

    void MotionSelectionIdWidgetController::ResetDisplayedRoundingError()
    {
        s_displayedRoundingError = 0.0f;
    }

    MotionSelectionIdWidgetController::MotionSelectionIdWidgetController(QGridLayout* layout,
        int graphicLayoutRowIndex, 
        const IRandomMotionSelectionDataContainer* dataContainer,
        bool displayMotionSelectionWeight)
        : m_dataContainer(dataContainer),
        m_displayMotionSelectionWeight(displayMotionSelectionWeight)
    {
        int column = 0;
        // Motion name
        m_labelMotion = new QLabel();
        m_labelMotion->setObjectName("m_labelMotion");
        m_labelMotion->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout->addWidget(m_labelMotion, graphicLayoutRowIndex, column);
        column++;
        // Motion position x
        QHBoxLayout* layoutX = new QHBoxLayout();
        layoutX->setAlignment(Qt::AlignRight);
        layoutX->setSpacing(2);
        layoutX->setMargin(2);
        m_randomWeightSpinbox = new AzQtComponents::DoubleSpinBox();
        m_randomWeightSpinbox->setSingleStep(0.1);
        m_randomWeightSpinbox->setDecimals(1);

        m_randomWeightSpinbox->setRange(0, FLT_MAX);

        layoutX->addWidget(m_randomWeightSpinbox);

        layout->addLayout(layoutX, graphicLayoutRowIndex, column);
        column++;
        m_normalizedProbabilityText = new QLineEdit();

        // The read only text for the normalized probabilities does not need the space for the SpinBox buttons
        m_normalizedProbabilityText->setMaximumWidth(aznumeric_cast<int>(m_randomWeightSpinbox->maximumWidth() * 0.5));
        m_normalizedProbabilityText->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_normalizedProbabilityText->setEnabled(false);

        layout->addWidget(m_normalizedProbabilityText, graphicLayoutRowIndex, column);
        column++;

        const int iconSize = 20;

        // Remove motion
        m_removeButton = new QPushButton();
        m_removeButton->setToolTip("Remove motion");
        m_removeButton->setMinimumSize(iconSize, iconSize);
        m_removeButton->setMaximumSize(iconSize, iconSize);
        m_removeButton->setIcon(QIcon(":/EMotionFX/Trash.svg"));
        layout->addWidget(m_removeButton, graphicLayoutRowIndex, column);

        if (!m_displayMotionSelectionWeight)
        {
            m_randomWeightSpinbox->setVisible(false);
            m_normalizedProbabilityText->setVisible(false);
        }
    }

    void MotionSelectionIdWidgetController::Hide()
    {
        m_labelMotion->hide();
        if (m_displayMotionSelectionWeight)
        {
            m_randomWeightSpinbox->hide();
            m_normalizedProbabilityText->hide();
        }
        m_removeButton->hide();
    }

    void MotionSelectionIdWidgetController::Show()
    {
        m_labelMotion->show();
        if (m_displayMotionSelectionWeight)
        {
            m_randomWeightSpinbox->show();
            m_normalizedProbabilityText->show();
        }
        m_removeButton->show();
    }

    void MotionSelectionIdWidgetController::UpdateId(size_t id)
    {
        m_id = id;
    }

    void MotionSelectionIdWidgetController::DestroyGuis()
    {
        m_labelMotion->deleteLater();
        m_removeButton->deleteLater();
        m_randomWeightSpinbox->deleteLater();
        m_normalizedProbabilityText->deleteLater();
    }

    size_t MotionSelectionIdWidgetController::GetId() const
    {
        return m_id;
    }

    void MotionSelectionIdWidgetController::Update()
    {
        const double weight = m_dataContainer->GetWeight(m_id);
        m_randomWeightSpinbox->setValue(weight);
        const double actualPercentage = 100.0 * weight / m_dataContainer->GetWeightSum();
        const double compensatedValue = actualPercentage - s_displayedRoundingError;
        const double roundedValue = qRound(compensatedValue);
        s_displayedRoundingError = aznumeric_cast<float>(roundedValue - compensatedValue);
        QString str = m_normalizedProbabilityText->locale().toString(roundedValue, 'f', 1);
        m_normalizedProbabilityText->setText(str);
        m_labelMotion->setText(m_dataContainer->GetMotionId(m_id).c_str());
    }

    MotionSetMotionIdPicker::MotionSetMotionIdPicker(QWidget* parent, bool displaySelectionWeights)
        : QWidget(parent)
        , m_displaySelectionWeights(displaySelectionWeights)
    {
        QVBoxLayout* vLayout = new QVBoxLayout();
        vLayout->setMargin(0);
        setLayout(vLayout);
    }

    MotionSetMotionIdPicker::~MotionSetMotionIdPicker()
    {
        // Destroying the dynamically allocated controller of each row
        m_motionWidgetControllers.clear();
    }

    void MotionSetMotionIdPicker::SetMotionIds(const AZStd::vector<AZStd::string>& motionIds)
    {
        HandleSelectedMotionsUpdate(motionIds);
        InitializeWidgets();
        UpdateGui();
    }

    void MotionSetMotionIdPicker::SetMotions(const AZStd::vector<AZStd::pair<AZStd::string, float>>& motions)
    {
        // Display the weights (not the cumulative non normalized probability that is passed from the serialized data)
        float cumulativeWeight = 0.0f;
        m_motions.clear();
        m_motions.reserve(motions.size());
        for (unsigned int i = 0; i < motions.size(); ++i)
        {
            m_motions.emplace_back(motions[i].first, motions[i].second - cumulativeWeight);
            cumulativeWeight = motions[i].second;
        }
        m_weightsSum = cumulativeWeight;
        InitializeWidgets();
        UpdateGui();
    }

    const AZStd::vector<AZStd::pair<AZStd::string, float>>& MotionSetMotionIdPicker::GetMotions() const
    {
        return m_motions;
    }

    AZStd::vector<AZStd::string> MotionSetMotionIdPicker::GetMotionIds() const
    {
        AZStd::vector<AZStd::string> motionIds;
        motionIds.reserve(m_motions.size());
        for (const auto& motionPair : m_motions)
        {
            motionIds.emplace_back(motionPair.first);
        }
        return motionIds;
    }

    /// This method updates the motion random selection weights
    /// setting default weights for those motions which were not in the data
    /// and deletes the motions that have not been selected if they were in the data
    /// exsisting motions will keep their current weight as set by the user with the GUI
    void MotionSetMotionIdPicker::HandleSelectedMotionsUpdate(const AZStd::vector<AZStd::string>& motionIds)
    {
        AZStd::unordered_map<AZStd::string, float> tmpRandomWeightsTable;

        for (size_t i = 0; i < m_motions.size(); ++i)
        {
            tmpRandomWeightsTable.emplace(m_motions[i]);
        }
        m_weightsSum = 0;
        m_motions.clear();
        m_motions.reserve(motionIds.size());

        AZStd::unordered_map<AZStd::string, float>::iterator tmpWeightTableIterator;
        for (const AZStd::string& motionId : motionIds)
        {
            float weight = s_defaultWeight;
            tmpWeightTableIterator = tmpRandomWeightsTable.find(motionId);
            if (tmpWeightTableIterator != tmpRandomWeightsTable.end())
            {
                weight = tmpWeightTableIterator->second;
            }
            m_weightsSum += weight;
            m_motions.emplace_back(motionId, weight);
        }
    }

    void MotionSetMotionIdPicker::OnPickClicked()
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        AnimGraphEditorRequestBus::BroadcastResult(motionSet, &AnimGraphEditorRequests::GetSelectedMotionSet);
        if (!motionSet)
        {
            QMessageBox::warning(this, "No Motion Set", "Cannot open motion selection window. No valid motion set selected.");
            return;
        }

        // Create and show the motion picker window
        m_motionPickWindow = new EMStudio::MotionSetSelectionWindow(this);
        m_motionPickWindow->GetHierarchyWidget()->SetSelectionMode(false);
        m_motionPickWindow->Update(motionSet);
        m_motionPickWindow->setModal(true);
        AZStd::vector<AZStd::string> motionIds;
        motionIds.reserve(m_motions.size());
        for (const auto& motionIdRandomWeightPair : m_motions)
        { 
            motionIds.emplace_back(motionIdRandomWeightPair.first);
        }
        m_motionPickWindow->Select(motionIds, motionSet);
        m_motionPickWindow->setAttribute(Qt::WA_DeleteOnClose);

        connect(m_motionPickWindow, &QDialog::accepted, this, &MotionSetMotionIdPicker::OnPickDialogAccept);
        connect(m_motionPickWindow, &QDialog::rejected, this, &MotionSetMotionIdPicker::OnPickDialogReject);

        m_motionPickWindow->open();
    }

    void MotionSetMotionIdPicker::OnPickDialogAccept()
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        AnimGraphEditorRequestBus::BroadcastResult(motionSet, &AnimGraphEditorRequests::GetSelectedMotionSet);
        if (!motionSet)
        {
            QMessageBox::warning(this, "No Motion Set", "Cannot open motion selection window. No valid motion set selected.");
            m_motionPickWindow->close();
            m_motionPickWindow = nullptr;
            return;
        }

        HandleSelectedMotionsUpdate(m_motionPickWindow->GetHierarchyWidget()->GetSelectedMotionIds(motionSet));

        InitializeWidgets();

        UpdateGui();

        emit SelectionChanged();

        m_motionPickWindow->close();
        m_motionPickWindow = nullptr;
    }

    void MotionSetMotionIdPicker::OnPickDialogReject()
    {
        m_motionPickWindow->close();
        m_motionPickWindow = nullptr;
    }

    void MotionSetMotionIdPicker::InitializeWidgets()
    {
        if (!m_containerWidget)
        {
            m_containerWidget = new QWidget();
            m_containerWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
            QVBoxLayout* widgetLayout = new QVBoxLayout();
            QHBoxLayout* topRowLayout = new QHBoxLayout();

            // Add helper label left of the add button.
            m_addMotionsLabel = new QLineEdit("");
            m_addMotionsLabel->setEnabled(false);

            topRowLayout->addWidget(m_addMotionsLabel);

            m_pickButton = new QPushButton(this);
            EMStudio::EMStudioManager::MakeTransparentButton(m_pickButton, "Images/Icons/Plus.svg", "Add motions to blend space");
            m_pickButton->setObjectName("EMFX.MotionSetMotionIdPicker.PickButton");
            connect(m_pickButton, &QPushButton::clicked, this, &MotionSetMotionIdPicker::OnPickClicked);
            topRowLayout->addWidget(m_pickButton);
            m_pickButton->setToolTip(QString("Add motions"));

            widgetLayout->addLayout(topRowLayout);

            QWidget* motionsWidget = new QWidget(m_containerWidget);

            motionsWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            QGridLayout* motionsLayout = new QGridLayout();
            motionsLayout->setHorizontalSpacing(0);
            AzQtComponents::ElidingLabel* labelColumn0 = new AzQtComponents::ElidingLabel();
            motionsLayout->addWidget(labelColumn0, 0, 0);
            AzQtComponents::ElidingLabel* labelColumn1 = new AzQtComponents::ElidingLabel("Probability weight");
            motionsLayout->addWidget(labelColumn1, 0, 1);
            AzQtComponents::ElidingLabel* labelColumn2 = new AzQtComponents::ElidingLabel("Probability (100%)");
            motionsLayout->addWidget(labelColumn2, 0, 2);
            if (!m_displaySelectionWeights)
            {
                labelColumn0->setVisible(false);
                labelColumn1->setVisible(false);
                labelColumn2->setVisible(false);
            }
            motionsWidget->setLayout(motionsLayout);
            widgetLayout->addWidget(motionsWidget);

            m_motionsLayout = motionsLayout;

            m_containerWidget->setLayout(widgetLayout);
            layout()->addWidget(m_containerWidget);
        }
        size_t layoutRowIndex = m_motionWidgetControllers.size();
        if (!m_displaySelectionWeights)
        {
            m_motionsLayout->setAlignment(Qt::AlignLeft);
        }
        else
        {
            // Making room for the grid header row
            layoutRowIndex++;
        }
        // Build more rows if needed
        if (m_motions.size() > m_motionWidgetControllers.size())
        {
            for (size_t widgetcontrollerCounter = m_motions.size() - m_motionWidgetControllers.size(); widgetcontrollerCounter > 0; --widgetcontrollerCounter)
            {
                m_motionWidgetControllers.emplace_back(AZStd::make_unique<MotionSelectionIdWidgetController>(m_motionsLayout, static_cast<int>(layoutRowIndex++), this, m_displaySelectionWeights));
                MotionSelectionIdWidgetController* motionWidget = m_motionWidgetControllers.back().get();
                connect(motionWidget->m_randomWeightSpinbox, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
                    [this, motionWidget](double value)
                    {
                        OnRandomWeightChanged(motionWidget->GetId(), value);
                    }
                );
                connect(motionWidget->m_removeButton, &QPushButton::clicked, [this, motionWidget]()
                {
                    OnRemoveMotion(motionWidget->GetId());
                });
            }
        }
        // Bind the row to the data and hide those that are not needed
        size_t id = 0;
        for(auto widgetsControllerIterator = m_motionWidgetControllers.begin(); widgetsControllerIterator != m_motionWidgetControllers.end(); ++widgetsControllerIterator)
        {
            if (id < m_motions.size())
            {
                (*widgetsControllerIterator)->UpdateId(id++);
                (*widgetsControllerIterator)->Show();
            }
            else
            {
                (*widgetsControllerIterator)->Hide();
            }
        }
    }

    void MotionSetMotionIdPicker::OnRandomWeightChanged(size_t id, double value)
    {
        m_weightsSum = aznumeric_cast<float>(m_weightsSum + (value - m_motions[id].second));
        m_motions[id].second = aznumeric_cast<float>(value);
        UpdateGui();
        emit SelectionChanged();
    }

    float MotionSetMotionIdPicker::GetWeight(size_t id) const
    {
        return m_motions[id].second;
    }

    float MotionSetMotionIdPicker::GetWeightSum() const
    {
        return m_weightsSum;
    }

    const AZStd::string& MotionSetMotionIdPicker::GetMotionId(size_t id) const
    {
        return m_motions[id].first;
    }

    void MotionSetMotionIdPicker::UpdateGui()
    {
        MotionSelectionIdWidgetController::ResetDisplayedRoundingError();
        auto widgetsIterator = m_motionWidgetControllers.begin();
        size_t validGuisCount = 0;
        for(; validGuisCount < m_motions.size() && widgetsIterator != m_motionWidgetControllers.end(); ++widgetsIterator, ++validGuisCount)
        {
            (*widgetsIterator)->Update();
        }

        if (m_motions.size() > 0)
        {
            m_addMotionsLabel->setText(AZStd::string::format("%zu motions selected", m_motions.size()).c_str());
        }
        else
        {
            m_addMotionsLabel->setText("Select motions");
        }
    }

    void MotionSetMotionIdPicker::OnRemoveMotion(size_t id)
    {
        m_weightsSum -= m_motions[id].second;
        m_motions.erase(m_motions.begin() + id);
        MotionSelectionIdWidgetController::ResetDisplayedRoundingError();
        InitializeWidgets();

        UpdateGui();
        emit SelectionChanged();
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 MotionIdRandomSelectionWeightsHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("MotionSetMotionIdsRandomSelectionWeights");
    }


    QWidget* MotionIdRandomSelectionWeightsHandler::CreateGUI(QWidget* parent)
    {
        MotionSetMotionIdPicker* picker = aznew MotionSetMotionIdPicker(parent, true);

        connect(picker, &MotionSetMotionIdPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void MotionIdRandomSelectionWeightsHandler::ConsumeAttribute(MotionSetMotionIdPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }


    void MotionIdRandomSelectionWeightsHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, MotionSetMotionIdPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        // Please Note: the values stored in the serialized data that will be used to randomly select the motion to play
        // contain the cumulative non normalized probability
        // whilst the data in the GUIs contain the randomselection weights
        instance.clear();
        const auto& motions = GUI->GetMotions();
        instance.reserve(motions.size());

        // Store in the node's data the cumulative non normalized probability (not the weights)
        float cumulativeWeight = 0.0f;
        for (size_t i = 0; i < motions.size(); ++i)
        {
            cumulativeWeight += motions[i].second;
            instance.emplace_back(motions[i].first, cumulativeWeight);
        }
    }


    bool MotionIdRandomSelectionWeightsHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, MotionSetMotionIdPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetMotions(instance);
        return true;
    }

    //---------------------------------------------------------------------------------------------------------------------------------------------------------

    AZ::u32 MotionSetMultiMotionIdHandler::GetHandlerName() const
    {
        return AZ_CRC_CE("MotionSetMotionIds");
    }


    QWidget* MotionSetMultiMotionIdHandler::CreateGUI(QWidget* parent)
    {
        MotionSetMotionIdPicker* picker = aznew MotionSetMotionIdPicker(parent, false);

        connect(picker, &MotionSetMotionIdPicker::SelectionChanged, this, [picker]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyEditorGUIMessages::Bus::Events::RequestWrite, picker);
        });

        return picker;
    }


    void MotionSetMultiMotionIdHandler::ConsumeAttribute(MotionSetMotionIdPicker* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ReadOnly)
        {
            bool value;
            if (attrValue->Read<bool>(value))
            {
                GUI->setEnabled(!value);
            }
        }
    }


    void MotionSetMultiMotionIdHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, MotionSetMotionIdPicker* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetMotionIds();
    }


    bool MotionSetMultiMotionIdHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, MotionSetMotionIdPicker* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        QSignalBlocker signalBlocker(GUI);
        GUI->SetMotionIds(instance);
        return true;
    }
} // namespace EMotionFX

#include <Source/Editor/PropertyWidgets/moc_MotionSetMotionIdHandler.cpp>
