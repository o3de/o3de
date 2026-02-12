// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${SanitizedCppName}Asset.h"

namespace ${GemName}
{
    /*
     * Reflect registers ${SanitizedCppName}Asset with the serialization and edit contexts.
     * This method is called by ${GemName}DataAssetSystemComponent::Reflect() during
     * engine startup, before any assets of this type are loaded.
     *
     * SerializeContext block:
     *   - The Class<> registration with ->Version(1) is required for the serialization
     *     system to read and write .azasset files for this type.
     *   - AZ::Edit::Attributes::EnableForAssetEditor(true) marks this class as editable
     *     in the standalone Asset Editor tool (accessible via Tools > Asset Editor).
     *     Without this attribute the Asset Editor will not display the asset's fields.
     *   - Each ->Field() call registers a member variable for serialization. The first
     *     argument is the serialized key name (used in the .azasset JSON file), the second
     *     is a pointer-to-member. Add one ->Field() for every data member you want persisted.
     *
     * EditContext block:
     *   - The ->Class<>() call sets the display name and tooltip shown in the Asset Editor.
     *   - Each ->DataElement() exposes a field in the Asset Editor UI. The UIHandlers::Default
     *     argument selects an appropriate widget based on the field type. Replace the field
     *     name and tooltip strings with descriptions meaningful to the asset's purpose.
     *
     * When you add new fields to this asset, bump the ->Version() number and add a
     * VersionConverter lambda to migrate old serialized data if the field layout changes.
     */
    void ${SanitizedCppName}Asset::Reflect(AZ::ReflectContext* context)
    {
        if (auto sc = azrtti_cast<AZ::SerializeContext*>(context))
        {
            sc->Class<${SanitizedCppName}Asset>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::EnableForAssetEditor, true)
                ->Field("SingleVariable", &${SanitizedCppName}Asset::m_singleVariable)
                ;

            if (AZ::EditContext* ec = sc->GetEditContext())
            {
                ec->Class<${SanitizedCppName}Asset>("${SanitizedCppName}Asset", "[Description of functionality provided by this asset]")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &${SanitizedCppName}Asset::m_singleVariable, "Single Variable", "Replace this variable with anything.")
                ;
            }
        }

    }
} // namespace ${GemName}
