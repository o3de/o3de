/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeTreeSelectionWidget.h>
#include <SceneAPI/SceneUI/RowWidgets/NodeTreeSelectionHandler.h>

#include <SceneAPIExt/Data/LodNodeSelectionList.h>
#include <Editor/PropertyWidgets/LODTreeSelectionWidget.h>
#endif
class QWidget;

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace UI
        {
            class LODTreeSelectionHandler 
                : public SceneUI::NodeTreeSelectionHandler
            {
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR_DECL

                QWidget* CreateGUI(QWidget* parent) override;
                AZ::u32 GetHandlerName() const override;
                bool IsDefaultHandler() const override;

                void ConsumeAttribute(SceneUI::NodeTreeSelectionWidget* widget, AZ::u32 attrib,
                    AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;

            protected:
                void ConsumeHideUncheckableAttribute(SceneUI::NodeTreeSelectionWidget* widget,
                    AzToolsFramework::PropertyAttributeReader* attrValue);
            };
        }
    }
}
