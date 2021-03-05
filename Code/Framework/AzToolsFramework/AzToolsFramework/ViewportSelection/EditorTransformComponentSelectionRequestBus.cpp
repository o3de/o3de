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

#include <AzCore/RTTI/BehaviorContext.h>
#include "EditorTransformComponentSelectionRequestBus.h"

namespace AzToolsFramework
{
    void EditorTransformComponentSelectionRequests::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            #define TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests() \
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common) \
                ->Attribute(AZ::Script::Attributes::Category, "Editor") \
                ->Attribute(AZ::Script::Attributes::Module, "editor")                              

            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::Mode::Rotation)>("TransformMode_Rotation")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();
            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::Mode::Translation)>("TransformMode_Translation")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();
            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::Mode::Scale)>("TransformMode_Scale")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();

            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::RefreshType::Translation)>("TransformRefreshType_Translation")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();
            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::RefreshType::Orientation)>("TransformRefreshType_Orientation")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();
            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::RefreshType::All)>("TransformRefreshType_All")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();

            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::Pivot::Object)>("TransformPivot_Object")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();
            behaviorContext->EnumProperty<static_cast<int>(EditorTransformComponentSelectionRequests::Pivot::Center)>("TransformPivot_Center")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests();

            behaviorContext->EBus<EditorTransformComponentSelectionRequestBus>("EditorTransformComponentSelectionRequestBus")
                TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests()
                ->Event("SetTransformMode", &EditorTransformComponentSelectionRequestBus::Events::SetTransformMode)
                ->Event("GetTransformMode", &EditorTransformComponentSelectionRequestBus::Events::GetTransformMode)
                // Reflecting GetManipulatorTransform will require hash<AZ::Transform> to be implemented, a pending task.
                //->Event("GetManipulatorTransform", &EditorTransformComponentSelectionRequestBus::Events::GetManipulatorTransform)
                ->Event("RefreshManipulators", &EditorTransformComponentSelectionRequestBus::Events::RefreshManipulators)
                ->Event("OverrideManipulatorOrientation", &EditorTransformComponentSelectionRequestBus::Events::OverrideManipulatorOrientation)
                ->Event("OverrideManipulatorTranslation", &EditorTransformComponentSelectionRequestBus::Events::OverrideManipulatorTranslation)
                ->Event("CopyTranslationToSelectedEntitiesIndividual", &EditorTransformComponentSelectionRequestBus::Events::CopyTranslationToSelectedEntitiesIndividual)
                ->Event("CopyTranslationToSelectedEntitiesGroup", &EditorTransformComponentSelectionRequestBus::Events::CopyTranslationToSelectedEntitiesGroup)
                ->Event("ResetTranslationForSelectedEntitiesLocal", &EditorTransformComponentSelectionRequestBus::Events::ResetTranslationForSelectedEntitiesLocal)
                ->Event("CopyOrientationToSelectedEntitiesIndividual", &EditorTransformComponentSelectionRequestBus::Events::CopyOrientationToSelectedEntitiesIndividual)
                ->Event("CopyOrientationToSelectedEntitiesGroup", &EditorTransformComponentSelectionRequestBus::Events::CopyOrientationToSelectedEntitiesGroup)
                ->Event("ResetOrientationForSelectedEntitiesLocal", &EditorTransformComponentSelectionRequestBus::Events::ResetOrientationForSelectedEntitiesLocal)
                ->Event("CopyScaleToSelectedEntitiesIndividualLocal", &EditorTransformComponentSelectionRequestBus::Events::CopyScaleToSelectedEntitiesIndividualLocal)
                ->Event("CopyScaleToSelectedEntitiesIndividualWorld", &EditorTransformComponentSelectionRequestBus::Events::CopyScaleToSelectedEntitiesIndividualWorld)
                ;

            #undef TEMP_SET_ATTRIBUTES_FOR_EditorTransformComponentSelectionRequests
        }
    }
} // namespace AzToolsFramework
