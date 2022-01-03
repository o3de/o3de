#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeListSelectionWidget.h>
#endif

class QWidget;


/*
=============================================================
= Handler Documentation                                     =
=============================================================

Handler Name: "NodeListSelection"

Available Attributes:
    o "DisabledOption" - Option presented to the user as the first option which will generally be interpreted 
                            as the default or disabled option. For instance, "Disable Vertex Coloring" as the 
                            default for selecting available vertex coloring options.
    o "ClassTypeIdFilter" - The UUID of the graph object class type to be listed. If not set all available graph 
                            objects will be listed.
    o "RequiresExactTypeId" - When 'ClassTypeIdFilter' is set setting this to true will cause only instances of 
                                the exact class to be listed, otherwise any class derived from the given UUID will be used.
    o "UseShortNames" - Whether or not to display the full scene graph path or only the short name.
    o "ExcludeEndPoints" - Whether or not graph nodes marked as end-points should be considered for displaying.
    o "DefaultToDisabled" - Whether or not the default option is the disabled option or the first entry if the value 
                            hasn't not been set or has become invalid. This requires 'DisabledOption' to be set, 
                            otherwise the first entry will be choosen.
*/


namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            class NodeListSelectionHandler 
                : public QObject, public AzToolsFramework::PropertyHandler<AZStd::string, NodeListSelectionWidget>
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                u32 GetHandlerName() const override;
                bool AutoDelete() const override;
                
                void ConsumeAttribute(NodeListSelectionWidget* widget, u32 attrib, 
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
                void WriteGUIValuesIntoProperty(size_t index, NodeListSelectionWidget* GUI, property_t& instance, 
                    AzToolsFramework::InstanceDataNode* node) override;
                bool ReadValuesIntoGUI(size_t index, NodeListSelectionWidget* GUI, const property_t& instance, 
                    AzToolsFramework::InstanceDataNode* node) override;

                static void Register();
                static void Unregister();

            protected:
                virtual void ConsumeDisabledOptionAttribute(NodeListSelectionWidget* widget, 
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeClassTypeIdAttribute(NodeListSelectionWidget* widget, 
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeRequiredExactTypeIdAttribute(NodeListSelectionWidget* widget, 
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeUseShortNameAttribute(NodeListSelectionWidget* widget, 
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeExcludeEndPointsAttribute(NodeListSelectionWidget* widget, 
                    AzToolsFramework::PropertyAttributeReader* attrValue);
                virtual void ConsumeDefaultToDisabledAttribute(NodeListSelectionWidget* widget, 
                    AzToolsFramework::PropertyAttributeReader* attrValue);

            private:
                static NodeListSelectionHandler* s_instance;
            };
        } // UI
    } // SceneAPI
} // AZ
