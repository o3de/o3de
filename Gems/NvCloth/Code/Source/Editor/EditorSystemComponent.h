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

#include <AzCore/Component/Component.h>

namespace AzToolsFramework
{
    class PropertyHandlerBase;
}

namespace NvCloth
{
    class EditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(EditorSystemComponent, "{4EABD010-B50D-45C6-AE3D-A617B26B14CA}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

    protected:
        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

    private:
        AZStd::vector<AzToolsFramework::PropertyHandlerBase*> m_propertyHandlers;
    };
} // namespace NvCloth
