/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/Module.h>
#include <Editor/MaterialCanvasEditorSystemComponent.h>

namespace MaterialCanvas
{
    //! Gem module for the Editor-hosted Material Canvas pane.
    //!
    //! This is a second target in the existing MaterialCanvas gem rather than a gem of its own. The gem already owns the node
    //! configurations, graph templates, style sheet and registry files, and every @gemroot:MaterialCanvas@ path in the asset
    //! data resolves here, so a separate gem would either duplicate all of that or reach across gems for it. The standalone
    //! MaterialCanvas APPLICATION target is unchanged and continues to build and run exactly as before.
    class MaterialCanvasEditorModule : public AZ::Module
    {
    public:
        AZ_RTTI(MaterialCanvasEditorModule, "{9A7D194B-C913-4E31-A8BE-614488C51F0D}", AZ::Module);
        AZ_CLASS_ALLOCATOR(MaterialCanvasEditorModule, AZ::SystemAllocator);

        MaterialCanvasEditorModule()
        {
            m_descriptors.insert(
                m_descriptors.end(),
                {
                    MaterialCanvasEditorSystemComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<MaterialCanvasEditorSystemComponent>(),
            };
        }
    };
} // namespace MaterialCanvas

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), MaterialCanvas::MaterialCanvasEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_MaterialCanvas, MaterialCanvas::MaterialCanvasEditorModule)
#endif
