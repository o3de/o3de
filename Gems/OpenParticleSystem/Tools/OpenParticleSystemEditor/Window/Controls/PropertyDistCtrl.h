/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <Window/Controls/InfoLineEdit.h>
#include <Window/Controls/InfoRadioButton.h>
#include <Window/Controls/CurveEditor.h>
#include <Window/Controls/DistIndexUtil.h>
#include <Window/Controls/PropertyGradientColorCtrl.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Serializer/ParticleBase.h>
#endif

#include <QWidget>
#include <QComboBox>

namespace OpenParticleSystemEditor
{
    class PropertyDistCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyDistCtrl, AZ::SystemAllocator, 0);

        explicit PropertyDistCtrl(const AZ::TypeId& id, QWidget* parent = nullptr);
        ~PropertyDistCtrl();

        template<typename ValueType>
        void SetValue(ValueType& value);
        template<typename ValueType>
        ValueType GetValue();

        void SetMinimum(double min);
        void SetMaximum(double max);
        void SetSuffix(const AZStd::string& suffix);

        void SetModuleId(AZ::TypeId id)
        {
            m_moduleId = id;
            m_curveEditor->m_moduleId = id;
        }

        void SetParamId(AZ::TypeId id)
        {
            m_paramId = id;
            m_curveEditor->m_paramId = id;
        }

        int GetElementCount(AZ::TypeId id);

        AZ::TypeId m_moduleId;
        AZ::TypeId m_valueTypeId;
        AZ::Uuid m_paramId;
        int m_elementCount = 1;
        void* m_valuePtr = nullptr;

    signals:
        void valueChanged();

    private:
        void SetupUI();
        void ConnectWidget();
        void ConnectKeyPointWidget();
        void RandomValueChanged() const;
        template<typename ValueType>
        void RandomValueChangedFunc(OpenParticle::ParticleSourceData* sourceData) const;
        void GradientValueChanged(QGradientStops stops) const;

        size_t GetDistIndex(const OpenParticle::DistributionType& distType, int index = 0) const;
        void SetDistIndex(const OpenParticle::DistributionType& distType, size_t value, int index = 0) const;
        OpenParticle::DistributionType GetDistType() const;
        void SetDistType(const OpenParticle::DistributionType& distType) const;
        void UpdateDistControls();
        template<typename ValueType>
        void UpdateDistControlsFunc(const OpenParticle::Distribution& distribution) const;
        int UpdateDistControlsCurve(const OpenParticle::Distribution& distribution) const;
        bool UpdateConstant();
        bool UpdateRandom();
        template<typename ValueType>
        bool UpdateRandomFunc();
        bool UpdateCurve();
        void ClearDistIndex() const;
        void ShowCurve() const;
        bool IsRandom() const;
        bool IsCurve() const;

        bool UpdateRandomTickMode(int index);
        bool UpdateUniformMode(int index);

        void StashCurrentRandom();
        void StashCurrentCurve();
        void DistChanged(int index);

    private slots:
        void OnDistChanged(int index);
        void OnRandomTickModeChanged(int index);
        void OnCurveTickModeChanged(int index);
        void OnUniformChanged(int index);

    private:
        QComboBox* m_comboType = nullptr;
        InfoLineEdit* m_editConstant = nullptr;
        QComboBox* m_randomTickMode = nullptr;
        InfoLineEdit* m_randomMin = nullptr;
        InfoLineEdit* m_randomMax = nullptr;
        InfoLineEdit* m_valueFactor = nullptr;
        QComboBox* m_curveTickMode = nullptr;
        InfoRadioButton* m_curveSelector = nullptr;
        CurveEditor* m_curveEditor = nullptr;
        PropertyGradientColorCtrl* m_gradientColorCtrl = nullptr;
        QComboBox* m_uniformButton = nullptr;

        static constexpr int m_keyPointEditType = 1;
        InfoLineEdit* m_keyPointXEditor = nullptr;
        InfoLineEdit* m_keyPointYEditor = nullptr;
    };

    template<class ValueType>
    class DistHandlerCommon
        : public AzToolsFramework::PropertyHandler<ValueType, PropertyDistCtrl>
    {
    public:
        AZ::u32 GetHandlerName(void) const override
        {
            return AZ_CRC("DistCtrlHandler");
        }
        virtual bool AutoDelete() const override
        {
            return false;
        }
        virtual void ConsumeAttribute(
            PropertyDistCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void ConsumeCustomAttribute(
            PropertyDistCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName);
    };

    class ValueObjFloatDistPropertyHandler
        : public QObject
        , public DistHandlerCommon<OpenParticle::ValueObjFloat>
    {
    public:
        AZ_CLASS_ALLOCATOR(ValueObjFloatDistPropertyHandler, AZ::SystemAllocator, 0);
        QWidget* CreateGUI(QWidget* pParent) override
        {
            PropertyDistCtrl* newCtrl = aznew PropertyDistCtrl(azrtti_typeid<OpenParticle::ValueObjFloat>(), pParent);
            connect(
                newCtrl, &PropertyDistCtrl::valueChanged, this,
                [newCtrl]()
                {
                    EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
                });
            return newCtrl;
        }

        void WriteGUIValuesIntoProperty(
            size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(
            size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    class ValueObjVec2DistPropertyHandler
        : public QObject
        , public DistHandlerCommon<OpenParticle::ValueObjVec2>
    {
    public:
        AZ_CLASS_ALLOCATOR(ValueObjVec2DistPropertyHandler, AZ::SystemAllocator, 0);
        QWidget* CreateGUI(QWidget* pParent) override
        {
            PropertyDistCtrl* newCtrl = aznew PropertyDistCtrl(azrtti_typeid<OpenParticle::ValueObjVec2>(), pParent);
            connect(
                newCtrl, &PropertyDistCtrl::valueChanged, this,
                [newCtrl]()
                {
                    EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
                });
            return newCtrl;
        }
    
        void WriteGUIValuesIntoProperty(
            size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(
            size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
    
    class ValueObjVec3DistPropertyHandler
        : public QObject
        , public DistHandlerCommon<OpenParticle::ValueObjVec3>
    {
    public:
        AZ_CLASS_ALLOCATOR(ValueObjVec3DistPropertyHandler, AZ::SystemAllocator, 0);
        QWidget* CreateGUI(QWidget* pParent) override
        {
            PropertyDistCtrl* newCtrl = aznew PropertyDistCtrl(azrtti_typeid<OpenParticle::ValueObjVec3>(), pParent);
            connect(
                newCtrl, &PropertyDistCtrl::valueChanged, this,
                [newCtrl]()
                {
                    EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
                });
            return newCtrl;
        }
    
        void WriteGUIValuesIntoProperty(
            size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(
            size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };
    
    class ValueObjVec4DistPropertyHandler
        : public QObject
        , public DistHandlerCommon<OpenParticle::ValueObjVec4>
    {
    public:
        AZ_CLASS_ALLOCATOR(ValueObjVec4DistPropertyHandler, AZ::SystemAllocator, 0);
        QWidget* CreateGUI(QWidget* pParent) override
        {
            PropertyDistCtrl* newCtrl = aznew PropertyDistCtrl(azrtti_typeid<OpenParticle::ValueObjVec4>(), pParent);
            connect(
                newCtrl, &PropertyDistCtrl::valueChanged, this,
                [newCtrl]()
                {
                    EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
                });
            return newCtrl;
        }
    
        void WriteGUIValuesIntoProperty(
            size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(
            size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    class ValueObjColorDistPropertyHandler
        : public QObject
        , public DistHandlerCommon<OpenParticle::ValueObjColor>
    {
    public:
        AZ_CLASS_ALLOCATOR(ValueObjColorDistPropertyHandler, AZ::SystemAllocator, 0);
        QWidget* CreateGUI(QWidget* pParent) override
        {
            PropertyDistCtrl* newCtrl = aznew PropertyDistCtrl(azrtti_typeid<OpenParticle::ValueObjColor>(), pParent);
            connect(
                newCtrl, &PropertyDistCtrl::valueChanged, this,
                [newCtrl]()
                {
                    EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
                });
            return newCtrl;
        }

        void WriteGUIValuesIntoProperty(
            size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(
            size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    class ValueObjLinearDistPropertyHandler
        : public QObject
        , public DistHandlerCommon<OpenParticle::ValueObjLinear>
    {
    public:
        AZ_CLASS_ALLOCATOR(ValueObjLinearDistPropertyHandler, AZ::SystemAllocator, 0);
        QWidget* CreateGUI(QWidget* pParent) override
        {
            PropertyDistCtrl* newCtrl = aznew PropertyDistCtrl(azrtti_typeid<OpenParticle::ValueObjLinear>(), pParent);
            connect(
                newCtrl, &PropertyDistCtrl::valueChanged, this,
                [newCtrl]()
                {
                    EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
                });
            return newCtrl;
        }

        void WriteGUIValuesIntoProperty(
            size_t index, PropertyDistCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(
            size_t index, PropertyDistCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    void RegisterDistCtrlHandlers();
    void UnregisterDistCtrlHandlers();
    void SetDistCtrlBusIDName(AZStd::string widgetName = "");
} // namespace OpenParticleSystemEditor

