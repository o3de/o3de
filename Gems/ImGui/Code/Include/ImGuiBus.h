/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/EBus/Event.h>

// Forward Declares
struct ImVec2;

namespace ImGui
{
    // Notes:   Hidden - ImGui Off, Input goes to Game
    //          Visible - ImGui Visible, Input goes to ImGui and consumed from game ( iff discrete input mode is on, else it is not consumed )
    //          VisibleNoMouse - ImGui Visible, Input goes to Game ( only a state iff discrete input mode is on )
    enum class DisplayState
    {
        Hidden,
        Visible,
        VisibleNoMouse
    };

    // Notes:   LockToResolution - Lock ImGui Render to a supplied resolution, regardless of LY Render Resolution
    //          MatchRenderResolution - Render ImGui at Render Resolution
    //          MatchToMaxRenderResolution - Render ImGui at Render Resolution, up to some maximum resolution, then Render at that max resolution.
    enum class ImGuiResolutionMode
    {
        LockToResolution = 0,
        MatchRenderResolution,
        MatchToMaxRenderResolution
    };

    // Notes:   Contextual - Use the Controller Stick and buttons to navigate ImGui as a contextual menu
    //          Mouse - Use the Controller stick and buttons as a virtual mouse within ImGui.
    namespace ImGuiControllerModeFlags
    {
        typedef AZ::u8 FlagType;

        constexpr FlagType
            Contextual = 1 << 0,
            Mouse = 1 << 1;
    }

    // Bus for getting updates from ImGui manager
    class IImGuiUpdateListener : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiUpdateListener"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiUpdateListener>;

        // ImGui Lifecycle Callbacks
        virtual void OnImGuiInitialize() {}
        virtual void OnImGuiUpdate() {}
        virtual void OnImGuiMainMenuUpdate() {}
        virtual void OnOpenEditorWindow() {}
    };
    typedef AZ::EBus<IImGuiUpdateListener> ImGuiUpdateListenerBus;

    // Bus for sending events and getting state from the ImGui manager
    class IImGuiManager
    {
    public:
        AZ_RTTI(IImGuiManager, "{F5A0F08B-F2DA-43B7-8CD2-C6FC71E1A712}");

        virtual ~IImGuiManager() = default;

        static const char* GetUniqueName() { return "IImGuiManager"; }

        virtual DisplayState GetEditorWindowState() const = 0;
        virtual void SetEditorWindowState(DisplayState state) = 0;
        virtual DisplayState GetClientMenuBarState() const = 0;
        virtual void SetClientMenuBarState(DisplayState state) = 0;  
        virtual bool IsControllerSupportModeEnabled(ImGuiControllerModeFlags::FlagType controllerMode) const = 0;
        virtual void EnableControllerSupportMode(ImGuiControllerModeFlags::FlagType controllerMode, bool enable) = 0;
        virtual void SetControllerMouseSensitivity(float sensitivity) = 0;
        virtual float GetControllerMouseSensitivity() const = 0;
        virtual bool GetEnableDiscreteInputMode() const = 0;
        virtual void SetEnableDiscreteInputMode(bool enabled) = 0;
        virtual ImGuiResolutionMode GetResolutionMode() const = 0;
        virtual void SetResolutionMode(ImGuiResolutionMode state) = 0;
        virtual const ImVec2& GetImGuiRenderResolution() const = 0;
        virtual void SetImGuiRenderResolution(const ImVec2& res) = 0;
        virtual void OverrideRenderWindowSize(uint32_t width, uint32_t height) = 0;
        virtual void RestoreRenderWindowSizeToDefault() = 0;
        virtual void ToggleThroughImGuiVisibleState() = 0;
        virtual void SetDpiScalingFactor(float dpiScalingFactor) = 0;
        virtual float GetDpiScalingFactor() const = 0;
        virtual void Render() = 0;

        using ImGuiSetEnabledEvent = AZ::Event<bool>;
        ImGuiSetEnabledEvent m_setEnabledEvent;

        // interface
        void ConnectImGuiSetEnabledChangedHandler(ImGuiSetEnabledEvent::Handler& handler)
        {
            handler.Connect(m_setEnabledEvent);
        }
    };

    class IImGuiManagerRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiManager>;
    };
    using ImGuiManagerBus = AZ::EBus<IImGuiManager, IImGuiManagerRequests>;

    class IImGuiManagerNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiManagerNotifications>;

        virtual void ImGuiSetEnabled( [[maybe_unused]] bool enabled) {}
    };
    using ImGuiManagerNotificationBus = AZ::EBus<IImGuiManagerNotifications>;

    // Bus for getting notifications from the IMGUI Entity Outliner
    class IImGuiEntityOutlinerNotifications : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiEntityOutlinerNotifications"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiEntityOutlinerNotifications>;

        // Callback for game code to handle targetting an IMGUI entity
        virtual void OnImGuiEntityOutlinerTarget(AZ::EntityId target) { (void)target;  }
    };
    typedef AZ::EBus<IImGuiEntityOutlinerNotifications> ImGuiEntityOutlinerNotificationBus;

    // a pair of an entity id, and a typeid, used to represent component rtti type info
    typedef AZStd::pair<AZ::EntityId, AZ::TypeId> ImGuiEntComponentId;
    // Bus for requests to the IMGUI Entity Outliner
    class IImGuiEntityOutlinerRequests : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiEntityOutlinerRequests"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiEntityOutlinerRequests>;

        // Requests for the ImGui Entity Outliner
        virtual void RemoveEntityView(AZ::EntityId entity) = 0;
        virtual void RequestEntityView(AZ::EntityId entity) = 0;
        virtual void RemoveComponentView(ImGuiEntComponentId component) = 0;
        virtual void RequestComponentView(ImGuiEntComponentId component) = 0;
        virtual void RequestAllViewsForComponent(const AZ::TypeId& comType) = 0;
        virtual void EnableTargetViewMode(bool enabled) = 0;
        virtual void EnableComponentDebug(const AZ::TypeId& comType, int priority = 1, bool enableMenuBar = false) = 0;
        virtual void SetEnabled(bool enabled) = 0;
        virtual void AddAutoEnableSearchString(const AZStd::string& searchString) = 0;
    };
    typedef AZ::EBus<IImGuiEntityOutlinerRequests> ImGuiEntityOutlinerRequestBus;

    // Bus for requests to the IMGUI Asset Explorer
    class IImGuiAssetExplorerRequests : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiAssetExplorerRequests"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiAssetExplorerRequests>;

        // Requests for the ImGui Asset Explorer
        virtual void SetEnabled(bool enabled) = 0;
    };
    typedef AZ::EBus<IImGuiAssetExplorerRequests> ImGuiAssetExplorerRequestBus;

    // Bus for requests to the IMGUI Camera Monitor
    class IImGuiCameraMonitorRequests : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiCameraMonitorRequests"; }
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        using Bus = AZ::EBus<IImGuiCameraMonitorRequests>;

        // Requests for the ImGui Camera Monitor
        virtual void SetEnabled(bool enabled) = 0;
    };
    typedef AZ::EBus<IImGuiCameraMonitorRequests> ImGuiCameraMonitorRequestBus;

    // Bus for getting debug Component updates from ImGui manager
    class IImGuiUpdateDebugComponentListener : public AZ::EBusTraits
    {
    public:
        static const char* GetUniqueName() { return "IImGuiUpdateDebugComponentListener"; }
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ImGuiEntComponentId;
        using Bus = AZ::EBus<IImGuiUpdateDebugComponentListener>;

        // AZ_RTTI required on this EBUS, this allows us to iterate through the handlers of this EBUS and deduce their type.
        AZ_RTTI(IImGuiUpdateDebugComponentListener, "{825B883F-A806-4304-AF82-C412AC5EC27B}");

        // OnImGuiDebugLYComponentUpdate - must implement this, this is the callback for a componenet instance
        //      to draw it's required debugging information. 
        virtual void OnImGuiDebugLYComponentUpdate() = 0;

        // GetComponentDebugPriority - an optional implementation. The Entity Outliner will ask components what their debug priority
        //      is, no override on the handler will return the below value of 1, you can optionally override in the handler to give it 
        //      a higher priority. Priority only really matters for giving a shortcut to the highest priority debugging component on a given entity
        virtual int GetComponentDebugPriority() { return 1; }

        // GetEnableMenuBar - an optional implementation. Components can define if their debug view uses a menu bar. False by default
        virtual bool GetEnableMenuBar() { return false; }

        // Connection Policy, at component connect time, Ask the component what priority they are via ebus, then
        //      register that component type with the priority returned with the Entity Outliner
        template<class Bus>
        struct ConnectionPolicy
            : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                // Get the debug priority for the component
                int priority = 1;
                AZ::EBus<IImGuiUpdateDebugComponentListener>::EventResult( priority, id, &AZ::EBus<IImGuiUpdateDebugComponentListener>::Events::GetComponentDebugPriority);

                // Get the debug priority for the component
                bool enableMenuBar = false;
                AZ::EBus<IImGuiUpdateDebugComponentListener>::EventResult(enableMenuBar, id, &AZ::EBus<IImGuiUpdateDebugComponentListener>::Events::GetEnableMenuBar);

                // Register
                ImGuiEntityOutlinerRequestBus::Broadcast(&ImGuiEntityOutlinerRequestBus::Events::EnableComponentDebug, id.second, priority, enableMenuBar);
            }

            static void Disconnect(typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::BusPtr& busPtr)
            {
                AZ::EBusConnectionPolicy<Bus>::Disconnect(context, handler, busPtr);
                if (busPtr)
                {
                    ImGuiEntityOutlinerRequestBus::Broadcast(&ImGuiEntityOutlinerRequestBus::Events::RemoveComponentView, busPtr->m_busId);
                }
            }
        };
    };
    typedef AZ::EBus<IImGuiUpdateDebugComponentListener> ImGuiUpdateDebugComponentListenerBus;

    
}
