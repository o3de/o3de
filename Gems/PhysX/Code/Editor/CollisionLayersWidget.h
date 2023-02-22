/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <QWidget>
#endif

namespace AzToolsFramework
{
    class ReflectedPropertyEditor;
}

namespace PhysX
{
    namespace Editor
    {
        class CollisionLayersWidget
            : public QWidget
            , private AzToolsFramework::IPropertyEditorNotify
        {
            Q_OBJECT

        public:
            AZ_CLASS_ALLOCATOR(CollisionLayersWidget, AZ::SystemAllocator);

            static const AZ::u32 s_maxCollisionLayerNameLength = 32;
            static const AZStd::string s_defaultCollisionLayerName;

            explicit CollisionLayersWidget(QWidget* parent = nullptr);

            void SetValue(const AzPhysics::CollisionLayers& layers);
            const AzPhysics::CollisionLayers& GetValue() const;

        signals:
            void onValueChanged(const AzPhysics::CollisionLayers& newValue);

        private:
            void CreatePropertyEditor(QWidget* parent);

            void BeforePropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void AfterPropertyModified(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingActive(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode* /*node*/) override;
            void SealUndoStack() override;
            void SetWidgetParameters();

            /**
             * Checks if layer name at specified index is unique in the given array of layer names.
             * @return true if the  layer name modified to be unique, false otherwise.
             */
            bool ForceUniqueLayerName(AZ::u32 layerIndex
                , const AZStd::array<AZStd::string, AzPhysics::CollisionLayers::MaxCollisionLayers>& layerNames
                , AZStd::string& uniqueLayerNameOut);

            /**
             * Checks if layer name at specified index is unique in the given array of layer names.
             * @return true if the layer name is unique, false otherwise.
             */
            bool IsLayerNameUnique(AZ::u32 layerIndex,
                const AZStd::array<AZStd::string, AzPhysics::CollisionLayers::MaxCollisionLayers>& layerNames);

            AzToolsFramework::ReflectedPropertyEditor* m_propertyEditor;
            AzPhysics::CollisionLayers m_value;
        };
    } // namespace Editor
} // namespace PhysX
