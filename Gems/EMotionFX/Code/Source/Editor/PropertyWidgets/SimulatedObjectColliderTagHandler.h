/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Editor/TagSelector.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <QWidget>
#endif


namespace EMotionFX
{
    class SimulatedObjectColliderTagSelector
        : public TagSelector
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        SimulatedObjectColliderTagSelector(QWidget* parent);
        void SetSimulatedObject(SimulatedObject* simulatedObject) { m_simulatedObject = simulatedObject; }

    private:
        void GetAvailableTags(QVector<QString>& outTags) const override;

        SimulatedObject* m_simulatedObject = nullptr;
    };

    class SimulatedObjectColliderTagHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, SimulatedObjectColliderTagSelector>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        SimulatedObjectColliderTagHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(SimulatedObjectColliderTagSelector* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, SimulatedObjectColliderTagSelector* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, SimulatedObjectColliderTagSelector* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        SimulatedObject* m_simulatedObject = nullptr;
    };

    class SimulatedJointColliderExclusionTagSelector
        : public TagSelector
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR_DECL

        SimulatedJointColliderExclusionTagSelector(QWidget* parent);
        void SetSimulatedObject(SimulatedObject* simulatedObject) { m_simulatedObject = simulatedObject; }

    private:
        void GetAvailableTags(QVector<QString>& outTags) const override;

        SimulatedObject* m_simulatedObject = nullptr;
    };

    class SimulatedJointColliderExclusionTagHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::vector<AZStd::string>, SimulatedJointColliderExclusionTagSelector>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        SimulatedJointColliderExclusionTagHandler();

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(SimulatedJointColliderExclusionTagSelector* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, SimulatedJointColliderExclusionTagSelector* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, SimulatedJointColliderExclusionTagSelector* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    protected:
        SimulatedObject* m_simulatedObject = nullptr;
    };
} // namespace EMotionFX
