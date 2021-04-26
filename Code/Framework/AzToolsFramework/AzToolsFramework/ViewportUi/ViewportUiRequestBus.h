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

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Event.h>
#include <AzToolsFramework/Picking/BoundInterface.h>

namespace AzToolsFramework::ViewportUi
{
    //! Used to track individual widgets from the Viewport UI.
    using ViewportUiElementId = IdType<struct ViewportUiIdType>;
    using ButtonId = IdType<struct ButtonIdType>;
    using ClusterId = IdType<struct ClusterIdType>;
    using TextFieldId = IdType<struct TextFieldIdType>;

    inline const ViewportUiElementId InvalidViewportUiElementId = ViewportUiElementId(0);
    inline const ButtonId InvalidButtonId = ButtonId(0);
    inline const ClusterId InvalidClusterId = ClusterId(0);

    inline const int DefaultViewportId = 0;

    //! Used to specify the desired validation type for the text field widget.
    enum class TextFieldValidationType
    {
        Int,
        Double,
        String
    };

    //! Viewport requests to interact with the Viewport UI. Viewport UI refers to the entire UI overlay (one per viewport).
    //! Each widget on the Viewport UI is referred to as an element.
    class ViewportUiRequests
    {
    public:
        //! Creates and registers a cluster with the Viewport UI system.
        virtual const ClusterId CreateCluster() = 0;
        //! Creates and registers a switcher with the Viewport UI system.
        virtual const ClusterId CreateSwitcher() = 0;
        //! Sets the active button of the cluster. This is the button which will display as highlighted.
        virtual void SetClusterActiveButton(ClusterId clusterId, ButtonId buttonId) = 0;
        //! Sets the active button of the switcher. This is the button which has a text label.
        virtual void SetSwitcherActiveButton(ClusterId clusterId, ButtonId buttonId) = 0;
        //! Registers a new button onto a cluster.
        virtual const ButtonId CreateClusterButton(const ClusterId clusterId, const AZStd::string& icon) = 0;
        //! Registers a new button onto a switcher.
        virtual const ButtonId CreateSwitcherButton(const ClusterId clusterId, const AZStd::string& icon, const AZStd::string& name = AZStd::string()) = 0;
        //! Registers an event handler to handle events from the cluster.
        virtual void RegisterClusterEventHandler(ClusterId clusterId, AZ::Event<ButtonId>::Handler& handler) = 0;
        //! Removes a cluster from the Viewport UI system.
        virtual void RemoveCluster(ClusterId clusterId) = 0;
        //! Sets the visibility of the cluster.
        virtual void SetClusterVisible(ClusterId clusterId, bool visible) = 0;
        //! Sets the visibility of multiple clusters.
        virtual void SetClusterGroupVisible(const AZStd::vector<ClusterId>& clusterGroup, bool visible) = 0;
        //! Creates and registers a text field with the Viewport UI system.
        virtual const TextFieldId CreateTextField(
            const AZStd::string& labelText, const AZStd::string& textFieldDefaultText,
            TextFieldValidationType validationType) = 0;
        //! Set the text that will go inside the text field.
        virtual void SetTextFieldText(TextFieldId textFieldId, const AZStd::string& text) = 0;
        //! Register an event handler to handle when the text field text changes.
        virtual void RegisterTextFieldCallback(
            TextFieldId textFieldId, AZ::Event<AZStd::string>::Handler& handler) = 0;
        //! Removes a text field from the Viewport UI system.
        virtual void RemoveTextField(TextFieldId textFieldId) = 0;
        //! Sets the visibility of the text field.
        virtual void SetTextFieldVisible(TextFieldId textFieldId, bool visible) = 0;
        //! Create the highlight border for Component Mode.
        virtual void CreateComponentModeBorder(const AZStd::string& borderTitle) = 0;
        //! Remove the highlight border for Component Mode.
        virtual void RemoveComponentModeBorder() = 0;
        //! Invoke a button press in a cluster.
        virtual void PressButton(ClusterId clusterId, ButtonId buttonId) = 0;
    };

    /// The EBusTraits for ViewportInteractionRequests.
    class ViewportUiBusTraits
        : public AZ::EBusTraits
    {
    public:
        using BusIdType = int; ///< ViewportId - used to address requests to this EBus.
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
    };

    /// Type to inherit to implement ViewportUiRequests
    using ViewportUiRequestBus = AZ::EBus<ViewportUiRequests, ViewportUiBusTraits>;
} // namespace AzToolsFramework::ViewportUi
