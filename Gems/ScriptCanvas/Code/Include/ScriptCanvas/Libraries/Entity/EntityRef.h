/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Libraries/Entity/Entity.h>
#include <ScriptCanvas/Core/NativeDatumNode.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Entity
        {
            class EntityRef
                : public NativeDatumNode<EntityRef, Data::EntityIDType>
            {
            public:
                using ParentType = NativeDatumNode<EntityRef, Data::EntityIDType>;
                AZ_COMPONENT(EntityRef, "{0EE5782F-B241-4127-AE53-E6746B00447F}", ParentType);
                
                static void Reflect(AZ::ReflectContext* reflection)
                {
                    if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
                    {
                        serializeContext->Class<EntityRef, PureData>()
                            ->Version(2)
                            ;

                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            // EntityRef node is special in that it's only created when we drag in an entity from the main scene.
                            // And is unmodifiable(essentially an external constant). As such, we hide it from the node palette.
                            editContext->Class<EntityRef>("EntityID", "Stores a reference to an entity")
                                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                                ->Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/EntityRef.png")
                                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::List)
                                ;
                        }
                    }
                }
                                
                AZ_INLINE void SetEntityRef(const AZ::EntityId& id)
                {
                    ModifiableDatumView datumView;
                    FindModifiableDatumView(GetSlotId(k_setThis), datumView);

                    if (datumView.IsValid())
                    {
                        // only called on edit time creation, so no need to push out the data, consider cutting this function if possible
                        datumView.SetAs(id);
                        OnOutputChanged((*datumView.GetDatum()));
                    }
                }

                AZ_INLINE AZ::EntityId GetEntityRef() const
                {
                    AZ::EntityId retVal;
                    if (auto input = FindDatum(GetSlotId(k_setThis)))
                    {
                        const AZ::EntityId* inputId = input->GetAs<AZ::EntityId>();

                        if (inputId)
                        {
                            retVal = (*inputId);
                        }
                    }

                    return retVal;
                }
                
            };
        }
    }
}
