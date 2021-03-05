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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    namespace MaterialBrowser
    {
        //! MaterialBrowserComponent allows initialization of MaterialBrowser systems, such as thumbnails
        class MaterialBrowserComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(MaterialBrowserComponent, "{121F3F3B-2412-490D-9E3E-C205C677F476}")

            MaterialBrowserComponent();
            virtual ~MaterialBrowserComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component
            //////////////////////////////////////////////////////////////////////////
            void Activate() override;
            void Deactivate() override;
            static void Reflect(AZ::ReflectContext* context);

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        };
    } // namespace MaterialBrowser
} // namespace AzToolsFramework