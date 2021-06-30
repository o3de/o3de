/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once



#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace Data
    {
        class AssetHandler;
    }
}

namespace ScriptCanvasBuilder
{
    class BuilderSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BuilderSystemComponent, "{2FB1C848-B863-4562-9C4B-01E18BD61583}");

        BuilderSystemComponent();
        ~BuilderSystemComponent() override;

        static void Reflect(AZ::ReflectContext* context);

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component...
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        BuilderSystemComponent(const BuilderSystemComponent&) = delete;

        AZStd::unique_ptr<AZ::Data::AssetHandler> m_scriptCanvasAssetHandler;
    };
}
