/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorLockComponentBus.h"
#include "EditorComponentBase.h"

namespace AzToolsFramework
{
    namespace Components
    {
         //! Controls whether an Entity is frozen/locked in the Editor.
        class EditorLockComponent
            : public AzToolsFramework::Components::EditorComponentBase
            , public EditorLockComponentRequestBus::Handler
        {
        public:
            AZ_COMPONENT(EditorLockComponent, "{C3A169C9-7EFB-4D6C-8710-3591680D0936}", EditorComponentBase);
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services);

            ~EditorLockComponent();

            // EditorLockComponentRequestBus ...
            void SetLocked(bool locked) override;
            bool GetLocked() override;

        private:
            // AZ::Entity ...
            void Init() override;

            bool m_locked = false; //!< Whether this entity is individually set to be locked.
        };
    } // namespace Components
} // namespace AzToolsFramework
