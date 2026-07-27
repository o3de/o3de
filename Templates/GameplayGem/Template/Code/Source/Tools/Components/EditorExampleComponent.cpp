// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include <Tools/Components/Editor${SanitizedCppName}Component.h>
#include <Components/${SanitizedCppName}Component.h>
#include <${Name}/${SanitizedCppName}TypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${SanitizedCppName}
{
    void Editor${SanitizedCppName}Component::Reflect(AZ::ReflectContext* context)
    {
        ${SanitizedCppName}ComponentController::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<Editor${SanitizedCppName}Component, AzToolsFramework::Components::EditorComponentBase>()
                ->Version(1)
                ->Field("Controller", &Editor${SanitizedCppName}Component::m_controller)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<Editor${SanitizedCppName}Component>("${SanitizedCppName}", "Controls gameplay behavior and attributes for an entity")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Gameplay")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &Editor${SanitizedCppName}Component::m_controller, "Controller", "Controller configuration for gameplay actions")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                    ;
            }
        }
    }

    void Editor${SanitizedCppName}Component::Init()
    {
        AzToolsFramework::Components::EditorComponentBase::Init();
        m_controller.Init();
    }

    void Editor${SanitizedCppName}Component::Activate()
    {
        AzToolsFramework::Components::EditorComponentBase::Activate();
        m_controller.Activate(GetEntityId());
    }

    void Editor${SanitizedCppName}Component::Deactivate()
    {
        m_controller.Deactivate();
        AzToolsFramework::Components::EditorComponentBase::Deactivate();
    }

    void Editor${SanitizedCppName}Component::BuildGameEntity(AZ::Entity* gameEntity)
    {
        gameEntity->CreateComponent<${SanitizedCppName}Component>(m_controller.GetConfiguration());
    }
} // namespace ${SanitizedCppName}
