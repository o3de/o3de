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

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <QComboBox>
#endif

namespace EMotionFX
{
    namespace Integration
    {
        class MotionSetAsset;
    }
    
    class MotionSetNameHandler
        : public QObject
        , public AzToolsFramework::PropertyHandler<AZStd::string, QComboBox>
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR_DECL

        MotionSetNameHandler()
            : m_motionSetAsset(nullptr)
        {}

        AZ::u32 GetHandlerName() const override;
        QWidget* CreateGUI(QWidget* parent) override;
        bool AutoDelete() const override { return false; }

        void ConsumeAttribute(QComboBox* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

        void WriteGUIValuesIntoProperty(size_t index, QComboBox* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, QComboBox* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;

    private:
        AZ::Data::Asset<Integration::MotionSetAsset>* m_motionSetAsset;
    };
} // namespace EMotionFX