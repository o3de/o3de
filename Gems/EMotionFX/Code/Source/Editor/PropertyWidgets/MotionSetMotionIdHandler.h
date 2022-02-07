/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/list.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionSetSelectionWindow.h>
#endif

namespace EMotionFX
{
    class AnimGraphMotionNode;

    class IRandomMotionSelectionDataContainer
    {
    public:
        virtual float GetWeight(size_t id) const = 0;
        virtual float GetWeightSum() const = 0;
        virtual const AZStd::string& GetMotionId(size_t id) const = 0;
    };

    class MotionSelectionIdWidgetController
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        MotionSelectionIdWidgetController(QGridLayout* layout,
            int graphicRowIndex,
            const IRandomMotionSelectionDataContainer* dataContainer,
            bool displayMotionSelectionWeight);

        size_t GetId() const;

        void Hide();
        void Show();
        void Update();
        void UpdateId(size_t id);

        void DestroyGuis();

        QLabel*                         m_labelMotion;
        AzQtComponents::DoubleSpinBox*  m_randomWeightSpinbox;
        QLineEdit*                      m_normalizedProbabilityText;
        QPushButton*                    m_removeButton;
        
        static void ResetDisplayedRoundingError();

    private:
        size_t                                              m_id = std::numeric_limits<size_t>::max();
        bool                                                m_displayMotionSelectionWeight = false;
        const IRandomMotionSelectionDataContainer*          m_dataContainer = nullptr;
        static float                                        s_displayedRoundingError;
    };

    class MotionSetMotionIdPicker
        : public QWidget
        , public IRandomMotionSelectionDataContainer
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR_DECL

        MotionSetMotionIdPicker(QWidget* parent, bool displaySelectionWeights);
        ~MotionSetMotionIdPicker();

        void SetMotionIds(const AZStd::vector<AZStd::string>& motions);
        void SetMotions(const AZStd::vector<AZStd::pair<AZStd::string, float>>& motions);
        const AZStd::vector<AZStd::pair<AZStd::string, float>>& GetMotions() const;

        AZStd::vector<AZStd::string> GetMotionIds() const;

        /// IDataContainer Implementation
        float GetWeight(size_t id) const override;
        float GetWeightSum() const override;
        const AZStd::string& GetMotionId(size_t id) const override;

    signals:
        void SelectionChanged();
    private slots:
        void OnPickClicked();
        void OnPickDialogAccept();
        void OnPickDialogReject();
    private:
        void InitializeWidgets();
        void HandleSelectedMotionsUpdate(const AZStd::vector<AZStd::string>& motionIds);
        void OnRemoveMotion(size_t id);
        void OnRandomWeightChanged(size_t id, double value);
        void UpdateGui();

        AZStd::list<AZStd::unique_ptr<MotionSelectionIdWidgetController>>                      m_motionWidgetControllers;

        /// Contains a list of pair of which the key is the motion Id, and the value is the random selection weight
        /// Please Note: this list contains the weights that are displayed in the GUIs, the stored data will contain 
        /// the cumulative probability 
        AZStd::vector<AZStd::pair<AZStd::string, float>>    m_motions;

        QPushButton*                                        m_pickButton = nullptr;
        QWidget*                                            m_containerWidget = nullptr;
        QLineEdit*                                          m_addMotionsLabel = nullptr;
        QGridLayout*                                        m_motionsLayout = nullptr;
        float                                               m_weightsSum = 0.0f;
        bool                                                m_displaySelectionWeights = false;
        static const float                                  s_defaultWeight;
        EMStudio::MotionSetSelectionWindow*                 m_motionPickWindow = nullptr;
    };


    class MotionIdRandomSelectionWeightsHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::pair<AZStd::string, float>>, MotionSetMotionIdPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(MotionSetMotionIdPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, MotionSetMotionIdPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, MotionSetMotionIdPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    class MotionSetMultiMotionIdHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, MotionSetMotionIdPicker>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(MotionSetMotionIdPicker* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, MotionSetMotionIdPicker* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, MotionSetMotionIdPicker* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
} // namespace EMotionFX
