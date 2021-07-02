/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/string_view.h>

#include <ACETypes.h>

#include <platform.h>
#include <IXml.h>

namespace AudioControls
{
    class IAudioSystemEditor;
}

namespace AudioControlsEditor
{
    class EditorImplPluginEvents
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        virtual void InitializeEditorImplPlugin() = 0;
        virtual void ReleaseEditorImplPlugin() = 0;

        virtual AudioControls::IAudioSystemEditor* GetEditorImplPlugin() = 0;
    };

    using EditorImplPluginEventBus = AZ::EBus<EditorImplPluginEvents>;

} // namespace AudioControlsEditor


namespace AudioControls
{
    class IAudioSystemControl;

    typedef AZ::u32 TImplControlTypeMask;

    //-------------------------------------------------------------------------------------------//
    struct SControlDef
    {
        TImplControlType m_type;                //! Middleware type of the control
        AZStd::string m_name;                   //! Name of the control
        AZStd::string m_path;                   //! Subfolder/path of the control
        bool m_isLocalized;                     //! If the control is localized.
        IAudioSystemControl* m_parentControl;   //! Pointer to the parent

        SControlDef(const AZStd::string& name, TImplControlType type, bool localized = false, IAudioSystemControl* parent = nullptr, const AZStd::string& path = "")
            : m_type(type)
            , m_name(name)
            , m_path(path)
            , m_isLocalized(localized)
            , m_parentControl(parent)
        {}
    };

    //-------------------------------------------------------------------------------------------//
    class IAudioSystemEditor
    {
    public:
        IAudioSystemEditor() {}
        virtual ~IAudioSystemEditor() {}

        //! Reloads all the middleware control data
        virtual void Reload() = 0;

        //! Creates a new middleware control given the specifications passed in as a parameter.
        //! The control is owned by this class.
        //! @param controlDefinition Values to use to initialize the new control.
        //! @return A pointer to the newly created control.
        virtual IAudioSystemControl* CreateControl(const SControlDef& controlDefinition) = 0;

        //! This function returns the root of the tree to allow for traversing the tree manually.
        //! Middleware controls are organized in a tree structure.
        //! @return A pointer to the root of the control tree.
        virtual IAudioSystemControl* GetRoot() = 0;

        //! Gets the middleware control given its unique id.
        //! @param id Unique ID of the control.
        //! @return A pointer to the control that corresponds to the passed id. If none is found nullptr is returned.
        virtual IAudioSystemControl* GetControl(CID id) const  = 0;

        //! Convert a middleware control type to an ATL control type.
        //! @param type Middleware control type
        //! @return An ATL control type that corresponds to the middleware control type passed as argument.
        virtual EACEControlType ImplTypeToATLType(TImplControlType type) const = 0;

        //! Given an ATL control type this function returns all the middleware control types that can be connected to it.
        //! @param atlControlType An ATL control type.
        //! @return A mask representing all the middleware control types that can be connected to the ATL control type passed as argument.
        virtual TImplControlTypeMask GetCompatibleTypes(EACEControlType atlControlType) const = 0;

        //! Creates and returns a connection to a middleware control.
        //! The connection object is owned by this class.
        //! @param atlControlType The type of the ATL control you are connecting to pMiddlewareControl.
        //! @param middlewareControl Middleware control for which to make the connection.
        //! @return A pointer to the newly created connection
        virtual TConnectionPtr CreateConnectionToControl(EACEControlType atlControlType, IAudioSystemControl* middlewareControl) = 0;

        //! Creates and returns a connection defined in an XML node.
        //! The format of the XML node should be in sync with the WriteConnectionToXMLNode function which is in charge of writing the node during serialization.
        //! If the XML node is unknown to the system NULL should be returned.
        //! If the middleware control referenced in the XML node does not exist it should be created and marked as "placeholder".
        //! @param node XML node where the connection is defined.
        //! @param atlControlType The type of the ATL control you are connecting to.
        //! @return A pointer to the newly created connection.
        virtual TConnectionPtr CreateConnectionFromXMLNode(XmlNodeRef node, EACEControlType atlControlType) = 0;

        //! When serializing connections between controls this function will be called once per connection to serialize its properties.
        //! This function should be in sync with CreateConnectionToControl as whatever it's written here will have to be read there.
        //! @param connection Connection to serialize.
        //! @param atlControlType Type of the ATL control that has this connection.
        //! @return XML node with the connection serialized.
        virtual XmlNodeRef CreateXMLNodeFromConnection(const TConnectionPtr connection, const EACEControlType atlControlType) = 0;

        //! Whenever a connection is removed from an ATL control this function should be called.
        //! To keep the system informed of which controls have been connected and which ones haven't.
        //! @param middlewareControl Middleware control that was disconnected.
        virtual void ConnectionRemoved([[maybe_unused]] IAudioSystemControl* middlewareControl) {}

        //! Returns the icon corresponding to the middleware control type passed as argument.
        //! @param type Middleware control type.
        //! @return A string with the path to the icon corresponding to the control type.
        virtual const AZStd::string_view GetTypeIcon(TImplControlType type) const = 0;

        //! Returns the selected state icon corresponding to the middleware control type passed as argument.
        //! @param type Middleware control type.
        //! @return A string with the path to the icon corresponding to the control type.
        virtual const AZStd::string_view GetTypeIconSelected(TImplControlType type) const = 0;

        //! Gets the name of the implementation which might be used in the ACE UI.
        //! @return String with the name of the implementation.
        virtual AZStd::string GetName() const = 0;

        //! Gets the folder where the implementation specific controls data are stored.
        //! This is used by the ACE to update if controls are changed while the editor is open.
        //! @return String with the path to the folder where the implementation specific controls are stored.
        virtual AZ::IO::FixedMaxPath GetDataPath() const = 0;

        //! Informs the plugin that the ACE has saved the data in case it needs to do any clean up.
        virtual void DataSaved() = 0;
    };

} // namespace AudioControls
