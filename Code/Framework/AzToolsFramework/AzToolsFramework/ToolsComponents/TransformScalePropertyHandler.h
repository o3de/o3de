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
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Math/Vector3.h>
#endif

namespace AzToolsFramework
{
    namespace Components
    {
        static const AZ::Crc32 TransformScaleHandler = AZ_CRC_CE("TransformScale");

        //! Handler to allow  the scale field inside the Transform Component to be represented as a single value in
        //! the editor, but stored internally as a Vector3.
        //! The purpose for this is to prevent any new entities being created with non-uniform scale on the Transform
        //! Component, but preserve the data required for migrating any existing entities to use the Non-Uniform Scale
        //! Component, until all migration work is completed.
        //! The value shown in the editor will be the maximum value from the scale vector, and changing the value in
        //! the editor will update the vector so that its maximum value matches the newly edited value, but its
        //! components retain their existing proportion.
        //! For example, if the current vector scale is (2, 3, 4), the value in the editor will appear as 4. If the value
        //! in the editor is updated to 2, then the vector scale will update to (1, 1.5, 2), keeping the same proportion
        //! between the x, y and z components.
        class TransformScalePropertyHandler
            : public QObject
            , public AzToolsFramework::PropertyHandler<AZ::Vector3, AzQtComponents::DoubleSpinBox>
        {
            Q_OBJECT //AUTOMOC
        public:
            AZ_CLASS_ALLOCATOR(TransformScalePropertyHandler, AZ::SystemAllocator, 0);

            AZ::u32 GetHandlerName(void) const override;
            QWidget* CreateGUI(QWidget* parent) override;
            void ConsumeAttribute(AzQtComponents::DoubleSpinBox* GUI, AZ::u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
            void WriteGUIValuesIntoProperty(size_t index, AzQtComponents::DoubleSpinBox* GUI,
                AZ::Vector3& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, AzQtComponents::DoubleSpinBox* GUI,
                const AZ::Vector3& instance, AzToolsFramework::InstanceDataNode* node) override;
        };
    } // namespace Components
} // namespace AzToolsFramework
