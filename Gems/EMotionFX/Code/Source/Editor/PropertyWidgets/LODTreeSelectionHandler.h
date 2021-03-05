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