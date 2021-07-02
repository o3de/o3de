/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <QWidget>
#include <EMotionFX/Source/AnimGraphBus.h>
#include <EMotionFX/Source/BlendTreeBlendNNode.h>
#include <QLabel>
#include <QLineEdit>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#endif


namespace EMotionFX
{
    class BlendNParamWeightElementWidget;
    class BlendNParamWeightContainerWidget;

    /// GUI data corresponding to the reflected element of the property container.
    /// Used to perform validation on the data in order to 
    /// Write valid data to the property only after successful validation
    class BlendNParamWeightGuiEntry
    {
    public:
        AZ_RTTI(BlendNParamWeightGuiEntry, "{5267D4D9-60AD-49D3-85D5-84419F7C4569}")
        AZ_CLASS_ALLOCATOR_DECL

        BlendNParamWeightGuiEntry(AZ::u32 portId, float weightRange, const char* sourceNodeName);
        virtual ~BlendNParamWeightGuiEntry() = default;

        const char* GetPortLabel() const;
        AZ::u32 GetPortId() const;
        float GetWeightRange() const;
        void SetWeightRange(float value);
        void SetValid(bool valid);
        bool IsValid() const;
        const char* GetSourceNodeName() const;
        const AZStd::string& GetTooltipText() const;
        void SetTooltipText(const AZStd::string& text);

    private:
        AZStd::string   m_tooltipText;
        const char*     m_sourceNodeName    = nullptr;
        bool            m_isValid           = false;
        AZ::u32         m_portId            = MCORE_INVALIDINDEX32;
        float           m_weightRange       = 0;
    };

    /// Every time a widget is created by the element property handler (a single entry in the container)
    /// The widget container is notified in order to be able to connect to its child widget
    class BlendNParemWeightWidgetBus
        : public AZ::EBusTraits
    {
    public:

        virtual void OnRequestDataBind(BlendNParamWeightElementWidget* /*elementWidget*/) { }
    };

    using BlendNParemWeightWidgetNotificationBus = AZ::EBus<BlendNParemWeightWidgetBus>;


    /// Widget that displays a single element of the property container
    class BlendNParamWeightElementWidget
        : public QWidget
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendNParamWeightElementWidget(QWidget* parent);
        ~BlendNParamWeightElementWidget();

        void SetParentContainerWidget(BlendNParamWeightContainerWidget* parent) { m_parentContainerWidget = parent; }
        void SetDataSource(const BlendNParamWeightGuiEntry& paramWeight);
        float GetWeightRange() const;
        void SetId(size_t index);
        size_t GetId() const;
        void UpdateGui();

        static const int s_decimalPlaces;

    signals:
        void DataChanged(BlendNParamWeightElementWidget* childWidget);
    
    private slots:
        void OnWeightRangeEdited(double value);

    private:
        BlendNParamWeightContainerWidget*   m_parentContainerWidget = nullptr;
        const BlendNParamWeightGuiEntry*    m_paramWeight           = nullptr;
        QLabel*                             m_sourceNodeNameLabel   = nullptr;
        AzQtComponents::DoubleSpinBox*      m_weightField           = nullptr;
        size_t                              m_dataElementIndex      = MCORE_INVALIDINDEX32;
    };


    /// Widget of the property container
    class BlendNParamWeightContainerWidget
        : public QWidget
        , private EMotionFX::AnimGraphNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendNParamWeightContainerWidget(QWidget* parent);
        ~BlendNParamWeightContainerWidget();

        void SetParamWeights(const AZStd::vector<BlendNParamWeight>& paramWeights, const AnimGraphNode* node);
        const AZStd::vector<BlendNParamWeightGuiEntry>& GetParamWeights() const;

        void ConnectWidgetToDataSource(BlendNParamWeightElementWidget* elementWidget);

        void AddElementWidget(BlendNParamWeightElementWidget* widget);
        void RemoveElementWidget(BlendNParamWeightElementWidget* widget);

        void Update();

    signals:
        void DataChanged();

    private slots:
        void HandleOnChildWidgetDataChanged(BlendNParamWeightElementWidget* elementWidget);

    private:
        bool CheckAllElementsValidation();
        bool CheckValidation();
        bool CheckElementValidation(size_t index);
        void UpdateDataValidation();
        void EqualizeWeightRanges();
        void EqualizeWeightRanges(float min, float max);
        void SetAllValid();
        ///////////////////////////////////////////////////////////////////
        // AnimGraphNotificationBus implementation
        void OnSyncVisualObject(AnimGraphObject* object) override;

        AZStd::vector<BlendNParamWeightElementWidget*>          m_elementWidgets;
        AZStd::vector<BlendNParamWeightGuiEntry>                m_paramWeights;
        size_t                                                  m_widgetBoundToDataCount = 0;
    };


    class BlendNParamWeightElementHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<BlendNParamWeight, BlendNParamWeightElementWidget>
    {
        Q_OBJECT //AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute([[maybe_unused]] BlendNParamWeightElementWidget* widget, [[maybe_unused]] AZ::u32 attrib, [[maybe_unused]] AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName) override { }

        void WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, [[maybe_unused]] BlendNParamWeightElementWidget* GUI, [[maybe_unused]] property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node) override { }
        bool ReadValuesIntoGUI(size_t index, BlendNParamWeightElementWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };


    class BlendNParamWeightsHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<BlendNParamWeight>, BlendNParamWeightContainerWidget>
        , private BlendNParemWeightWidgetNotificationBus::Handler
    {
        Q_OBJECT //AUTOMOC

    public:
        AZ_CLASS_ALLOCATOR_DECL

        BlendNParamWeightsHandler();
        ~BlendNParamWeightsHandler();
        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(BlendNParamWeightContainerWidget* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, BlendNParamWeightContainerWidget* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, BlendNParamWeightContainerWidget* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    private:

        //////////////////////////////////////////////////////////////////////////
        // BlendNParemWeightWidgetNotificationBus::Handler implementation
        void OnRequestDataBind(BlendNParamWeightElementWidget* elementWidget) override;

        BlendNParamWeightContainerWidget*   m_containerWidget   = nullptr;
        AnimGraphNode*                      m_node              = nullptr;
    };
}
