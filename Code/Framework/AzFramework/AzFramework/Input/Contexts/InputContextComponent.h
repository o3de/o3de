/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Input/Contexts/InputContext.h>
#include <AzFramework/Input/Mappings/InputMapping.h>
#include <AzFramework/Input/User/LocalUserId.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class InputContextComponentRequests : public AZ::ComponentBus
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Set the local user id that the InputContextComponent should process input from
        //! \param[in] localUserId Local user id the InputContextComponent should process input from
        virtual void SetLocalUserId(LocalUserId localUserId) = 0;
    };
    using InputContextComponentRequestBus = AZ::EBus<InputContextComponentRequests>;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! An InputContextComponent is used to configure (at edit time) the data necessary to create an
    //! InputContext (at run time). The life cycle of any InputContextComponent is controlled by the
    //! AZ::Entity it is attached to, adhering to the same rules as any other AZ::Component, and the
    //! InputContext which it owns is created/destroyed when the component is activated/deactivated.
    class InputContextComponent : public AZ::Component
                                , public InputContextComponentRequestBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // AZ::Component Setup
        AZ_COMPONENT(InputContextComponent, "{321689F8-A572-47D7-9D1C-EF9E0D2CD472}");

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::ComponentDescriptor::Reflect
        static void Reflect(AZ::ReflectContext* context);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default Constructor
        InputContextComponent() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputContextComponent() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Init
        void Init() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Activate
        void Activate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AZ::Component::Deactivate
        void Deactivate() override;
        
        ////////////////////////////////////////////////////////////////////////////////////////////
        // \ref AzFramework::InputContextComponentRequests::SetLocalUserId
        void SetLocalUserId(LocalUserId localUserId) override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Create the input context.
        void CreateInputContext();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Reset the input context.
        void ResetInputContext();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The list of all input mappings that will be created when the input context is activated.
        //! Reflected to EditContext, then used to create and add input mapping classes in Activate.
        AZStd::vector<InputMapping::ConfigBase*> m_inputMappings;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The name of the input context, unique among all active input contexts and input devices.
        //! This will be truncated if its length exceeds that of InputDeviceId::MAX_NAME_LENGTH = 64
        //! Reflected to EditContext, then used to create the unique input context class in Activate.
        AZStd::string m_uniqueName;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Input context that is created and owned by this component. Not reflected to EditContext.
        AZStd::unique_ptr<InputContext> m_inputContext;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Filter used to determine whether an input event should be handled by this input context.
        //! Not reflected, but created inside SetLocalUserId if needed to fliter by a local user id.
        AZStd::shared_ptr<InputChannelEventFilterInclusionList> m_localUserIdFilter;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The local player index that this component will receive input from (0 base, -1 wildcard).
        //! Will only work on platforms where the local user id corresponds to the local player index.
        //! For other platforms, SetLocalUserId must be called at runtime with id of a logged in user.
        //! Reflected to EditContext, then used if needed to create the local user id filter in Init.
        AZ::s32 m_localPlayerIndex = -1;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The priority used to sort the input context relative to all other input event listeners.
        //! Reflected to EditContext, then used to create the unique input context class in Activate.
        AZ::s32 m_inputListenerPriority = InputChannelEventListener::GetPriorityDefault();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Should the input context consume input that is processed by any of its input mappings?
        //! Reflected to EditContext, then used to create the unique input context class in Activate.
        bool m_consumesProcessedInput = false;
    };
} // namespace AzFramework
