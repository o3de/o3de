/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Module/Module.h>
#include <RPI.Private/Module.h>

#include <RPI.Private/RPISystemComponent.h>

namespace AZ
{
    namespace RPI
    {
        class EditorModule final
            : public AZ::RPI::Module
        {
        public:
            AZ_RTTI(EditorModule, "{F2DF5DD9-1323-436C-B0E5-B200B8709CE5}", AZ::RPI::Module);

            EditorModule()
            : AZ::RPI::Module()
            {
            }

            AZ::ComponentTypeList GetRequiredSystemComponents() const override
            {
                AZ::ComponentTypeList ctl = AZ::RPI::Module::GetRequiredSystemComponents();
                // add any editor module required systems here.
                return ctl;
            }
        };
    } // namespace RPI
} // namespace RPI


  // DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
  // The first parameter should be GemName_GemIdLower
  // The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_Atom_RPI_Editor, AZ::RPI::EditorModule);
