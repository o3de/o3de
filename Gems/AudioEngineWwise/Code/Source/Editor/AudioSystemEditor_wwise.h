/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <IAudioSystemEditor.h>
#include <IAudioConnection.h>
#include <IAudioSystemControl.h>
#include <AudioWwiseLoader.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    class CRtpcConnection
        : public IAudioConnection
    {
    public:
        explicit CRtpcConnection(CID id)
            : IAudioConnection(id)
            , m_mult(1.0f)
            , m_shift(0.0f)
        {}

        ~CRtpcConnection() override = default;

        bool HasProperties() override { return true; }

        float m_mult;
        float m_shift;
    };

    using TRtpcConnectionPtr = AZStd::shared_ptr<CRtpcConnection>;

    //-------------------------------------------------------------------------------------------//
    class CStateToRtpcConnection
        : public IAudioConnection
    {
    public:
        explicit CStateToRtpcConnection(CID id)
            : IAudioConnection(id)
            , m_value(0.0f)
        {}

        ~CStateToRtpcConnection() override = default;

        bool HasProperties() override { return true; }

        float m_value;
    };

    using TStateConnectionPtr = AZStd::shared_ptr<CStateToRtpcConnection>;


    //-------------------------------------------------------------------------------------------//
    class CAudioSystemEditor_wwise
        : public IAudioSystemEditor
    {
        friend class CAudioWwiseLoader;

    public:
        CAudioSystemEditor_wwise();
        ~CAudioSystemEditor_wwise() override = default;

        //////////////////////////////////////////////////////////
        // IAudioSystemEditor implementation
        /////////////////////////////////////////////////////////
        void Reload() override;
        IAudioSystemControl* CreateControl(const SControlDef& controlDefinition) override;
        IAudioSystemControl* GetRoot() override { return &m_rootControl; }
        IAudioSystemControl* GetControl(CID id) const override;
        EACEControlType ImplTypeToATLType(TImplControlType type) const override;
        TImplControlTypeMask GetCompatibleTypes(EACEControlType atlControlType) const override;
        TConnectionPtr CreateConnectionToControl(EACEControlType atlControlType, IAudioSystemControl* middlewareControl) override;
        TConnectionPtr CreateConnectionFromXMLNode(XmlNodeRef node, EACEControlType atlControlType) override;
        XmlNodeRef CreateXMLNodeFromConnection(const TConnectionPtr connection, const EACEControlType atlControlType) override;
        const AZStd::string_view GetTypeIcon(TImplControlType type) const override;
        const AZStd::string_view GetTypeIconSelected(TImplControlType type) const override;
        AZStd::string GetName() const override;
        AZ::IO::FixedMaxPath GetDataPath() const override;
        void DataSaved() override {}
        void ConnectionRemoved(IAudioSystemControl* control) override;
        //////////////////////////////////////////////////////////

    private:
        IAudioSystemControl* GetControlByName(AZStd::string name, bool isLocalized = false, IAudioSystemControl* parent = nullptr) const;

        // Gets the ID of the control given its name. As controls can have the same name
        // if they're under different parents, the name of the parent is also needed (if there is one)
        CID GetID(const AZStd::string_view name) const;

        void UpdateConnectedStatus();

        IAudioSystemControl m_rootControl;

        using TControlPtr = AZStd::shared_ptr<IAudioSystemControl>;
        using TControlMap = AZStd::unordered_map<CID, TControlPtr>;
        TControlMap m_controls;

        using TConnectionsMap = AZStd::unordered_map<CID, int>;
        TConnectionsMap m_connectionsByID;
        CAudioWwiseLoader m_loader;
    };

} // namespace AudioControls
