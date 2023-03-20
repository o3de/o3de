/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Components/Widgets/VectorInput.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzCore/Math/Matrix3x3.h>
#endif

namespace PhysX
{
    namespace Editor
    {
        static const AZ::Crc32 InertiaHandler = AZ_CRC_CE("RigidBodyInertia");

        class InertiaPropertyHandler
            : public QObject
            , public AzToolsFramework::PropertyHandler<AZ::Matrix3x3, AzQtComponents::VectorInput>
        {
            Q_OBJECT //AUTOMOC
        public:
            AZ_CLASS_ALLOCATOR(InertiaPropertyHandler, AZ::SystemAllocator);

            AZ::u32 GetHandlerName(void) const override;
            QWidget* CreateGUI(QWidget* parent) override;
            void ConsumeAttribute(AzQtComponents::VectorInput* GUI, AZ::u32 attrib,
                AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
            void WriteGUIValuesIntoProperty(size_t index, AzQtComponents::VectorInput* GUI,
                AZ::Matrix3x3& instance, AzToolsFramework::InstanceDataNode* node) override;
            bool ReadValuesIntoGUI(size_t index, AzQtComponents::VectorInput* GUI,
                const AZ::Matrix3x3& instance, AzToolsFramework::InstanceDataNode* node) override;
        };
    } // namespace Editor
} // namespace PhysX
