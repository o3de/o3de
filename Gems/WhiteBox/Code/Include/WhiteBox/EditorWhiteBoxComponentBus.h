/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "EditorWhiteBoxDefaultShapeTypes.h"

#include <AzCore/Component/ComponentBus.h>

namespace WhiteBox
{
    struct WhiteBoxMesh;

    //! Wrapper around WhiteBoxMesh address.
    struct WhiteBoxMeshHandle
    {
        uintptr_t m_whiteBoxMeshAddress; //!< The raw address of the WhiteBoxMesh pointer.
    };

    //! EditorWhiteBoxComponent requests.
    class EditorWhiteBoxComponentRequests : public AZ::EntityComponentBus
    {
    public:
        // EBusTraits overrides ...
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Return a pointer to the WhiteBoxMesh.
        virtual WhiteBoxMesh* GetWhiteBoxMesh() = 0;

        //! Return a handle wrapping the raw address of the WhiteBoxMesh pointer.
        //! @note This is currently used to address the WhiteBoxMesh via script.
        virtual WhiteBoxMeshHandle GetWhiteBoxMeshHandle();

        //! Serialize the current mesh.
        //! Take the in-memory representation of the WhiteBoxMesh and write it to
        //! an output stream.
        //! @note The data is either stored directly on the Component or in an Asset.
        virtual void SerializeWhiteBox() = 0;

        //! Deserialize the stored mesh data.
        //! Take the previously serialized (stored) WhiteBoxMesh data and create a new
        //! WhiteBoxMesh from it.
        //! @note The data is either loaded directly from the Component or from an Asset.
        virtual void DeserializeWhiteBox() = 0;

        //! If an Asset is in use, write the data from it back to be stored directly on the Component.
        virtual void WriteAssetToComponent() = 0;

        //! Rebuild the White Box representation.
        //! @note Includes the render mesh and physics mesh (if present).
        virtual void RebuildWhiteBox() = 0;

        //! Set the white box mesh default shape.
        virtual void SetDefaultShape(DefaultShapeType defaultShape) = 0;

    protected:
        ~EditorWhiteBoxComponentRequests() = default;
    };

    inline WhiteBoxMeshHandle EditorWhiteBoxComponentRequests::GetWhiteBoxMeshHandle()
    {
        return WhiteBoxMeshHandle{reinterpret_cast<uintptr_t>(static_cast<void*>(GetWhiteBoxMesh()))};
    }

    using EditorWhiteBoxComponentRequestBus = AZ::EBus<EditorWhiteBoxComponentRequests>;

    //! EditorWhiteBoxComponent notifications.
    class EditorWhiteBoxComponentNotifications : public AZ::EntityComponentBus
    {
    public:
        //! Notify the component the mesh has been modified.
        virtual void OnWhiteBoxMeshModified() {}

        //! Notify listeners when the default shape of the white box mesh changes.
        virtual void OnDefaultShapeTypeChanged([[maybe_unused]] DefaultShapeType defaultShape) {}

    protected:
        ~EditorWhiteBoxComponentNotifications() = default;
    };

    using EditorWhiteBoxComponentNotificationBus = AZ::EBus<EditorWhiteBoxComponentNotifications>;
} // namespace WhiteBox
