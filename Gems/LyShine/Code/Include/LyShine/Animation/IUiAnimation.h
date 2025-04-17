/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Range.h>
#include <AnimKey.h>
#include <LyShine/ILyShine.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Color.h>

// forward declaration.
struct IUiAnimTrack;
struct IUiAnimNode;
struct IUiAnimSequence;
struct IUiAnimationSystem;
struct IKey;
class XmlNodeRef;
struct ISplineInterpolator;

namespace AZ
{
    class Component;
    class Entity;
    class SerializeContext;
}

#define MAX_ANIM_NAME_LENGTH 64

#if !defined(CONSOLE)
#define UI_ANIMATION_SYSTEM_SUPPORT_EDITING
#endif

typedef AZStd::vector<IUiAnimSequence*> UiAnimSequences;
typedef AZStd::vector<AZStd::string> UiTrackEvents;

//////////////////////////////////////////////////////////////////////////
//! Node-Types
//
// You need to register new types in UiAnimationSystem.cpp/RegisterNodeTypes for serialization.
// Note: Enums are serialized by string now, there is no need for specific IDs
// anymore for new parameters. Values are for backward compatibility.
enum EUiAnimNodeType
{
    eUiAnimNodeType_Invalid                 = 0x00,
    eUiAnimNodeType_Entity                  = 0x01,
    eUiAnimNodeType_Director                = 0x02,
    eUiAnimNodeType_Camera                  = 0x03,
    eUiAnimNodeType_CVar                    = 0x04,
    eUiAnimNodeType_ScriptVar               = 0x05,
    eUiAnimNodeType_Material                = 0x06,
    eUiAnimNodeType_Event                   = 0x07,
    eUiAnimNodeType_Group                   = 0x08,
    eUiAnimNodeType_Layer                   = 0x09,
    eUiAnimNodeType_Comment                 = 0x10,
    eUiAnimNodeType_RadialBlur              = 0x11,
    eUiAnimNodeType_ColorCorrection         = 0x12,
    eUiAnimNodeType_DepthOfField            = 0x13,
    eUiAnimNodeType_ScreenFader             = 0x14,
    eUiAnimNodeType_Light                   = 0x15,
    eUiAnimNodeType_HDRSetup                = 0x16,
    eUiAnimNodeType_ShadowSetup             = 0x17,
    eUiAnimNodeType_Alembic                 = 0x18, // Used in cinebox, added so nobody uses that number
    eUiAnimNodeType_GeomCache               = 0x19,
    eUiAnimNodeType_Environment,
    eUiAnimNodeType_ScreenDropsSetup,
    eUiAnimNodeType_AzEntity,
    eUiAnimNodeType_Num
};

//! Flags that can be set on animation node.
enum EUiAnimNodeFlags
{
    eUiAnimNodeFlags_Expanded               = BIT(0), //!< Deprecated, handled by sandbox now
    eUiAnimNodeFlags_EntitySelected         = BIT(1), //!< Set if the referenced entity is selected in the editor
    eUiAnimNodeFlags_CanChangeName          = BIT(2), //!< Set if this node allow changing of its name.
    eUiAnimNodeFlags_Disabled               = BIT(3), //!< Disable this node.
};

// Static common parameters IDs of animation node.
//
// You need to register new params in UiAnimationSystem.cpp/RegisterParamTypes for serialization.
// Note: Enums are serialized by string now, there is no need for specific IDs
// anymore for new parameters. Values are for backward compatibility.
//
// If you want to expand UI Animation system to control new stuff this is probably  the enum you want to change.
// For named params see eUiAnimParamType_ByString & CUiAnimParamType
enum EUiAnimParamType : int
{
    eUiAnimParamType_AzComponentField,
    eUiAnimParamType_Event,
    eUiAnimParamType_TrackEvent,
    eUiAnimParamType_Float,
    eUiAnimParamType_ByString,
    eUiAnimParamType_User                               = 100000, //!< User node params.

    eUiAnimParamType_Invalid                            = -1
};

// Common parameters of animation node.
class CUiAnimParamType
{
    friend class UiAnimationSystem;

public:
    AZ_TYPE_INFO(CUiAnimParamType, "{3D4ADD98-DFDD-4984-AC59-E59C5D718CFC}")

    static const uint kParamTypeVersion = 8;

    CUiAnimParamType()
        : m_type(eUiAnimParamType_Invalid) {}

    CUiAnimParamType(const AZStd::string& name)
    {
        *this = name;
    }

    CUiAnimParamType(const EUiAnimParamType type)
    {
        *this = type;
    }

    // Convert from old enum or int
    void operator =(const int type)
    {
        m_type = (EUiAnimParamType)type;
    }

    void operator =(const AZStd::string& name)
    {
        m_type = eUiAnimParamType_ByString;
        m_name = name.c_str();
    }

    // Convert to enum. This needs to be explicit,
    // otherwise operator== will be ambiguous
    EUiAnimParamType GetType() const { return m_type; }

    // Get name
    const char* GetName() const { return m_name.c_str(); }

    bool operator ==(const CUiAnimParamType& animParamType) const
    {
        if (m_type == eUiAnimParamType_ByString && animParamType.m_type == eUiAnimParamType_ByString)
        {
            return m_name == animParamType.m_name;
        }

        return m_type == animParamType.m_type;
    }

    bool operator !=(const CUiAnimParamType& animParamType) const
    {
        return !(*this == animParamType);
    }

    bool operator <(const CUiAnimParamType& animParamType) const
    {
        if (m_type == eUiAnimParamType_ByString && animParamType.m_type == eUiAnimParamType_ByString)
        {
            return m_name < animParamType.m_name;
        }
        else if (m_type == eUiAnimParamType_ByString)
        {
            return false; // Always sort named params last
        }
        else if (animParamType.m_type == eUiAnimParamType_ByString)
        {
            return true; // Always sort named params last
        }
        if (m_type < animParamType.m_type)
        {
            return true;
        }

        return false;
    }

    // Serialization. Defined in UiAnimationSystem.cpp
    inline void Serialize(IUiAnimationSystem* animationSystem, XmlNodeRef& xmlNode, bool bLoading, const uint version = kParamTypeVersion);

private:
    EUiAnimParamType m_type;
    AZStd::string m_name;
};

// The data required to identify a specific parameter/property on an AZ component on an AZ entity
class UiAnimParamData
{
    friend class UiAnimationSystem;

public:

    AZ_TYPE_INFO(UiAnimParamData, "{B2E9F22C-8DB5-40FA-9DF1-276E91BA8B95}")

    UiAnimParamData()
        : m_componentId(AZ::InvalidComponentId)
        , m_offset(0)
    {}

    UiAnimParamData(AZ::ComponentId componentId, const char* name, AZ::Uuid typeId, size_t offset)
        : m_componentId(componentId)
        , m_typeId(typeId)
        , m_name(name)
        , m_offset(offset)
    {}

    AZ::ComponentId GetComponentId() const
    {
        return m_componentId;
    }

    AZ::Uuid GetTypeId() const
    {
        return m_typeId;
    }

    const char* GetName() const
    {
        return m_name.c_str();
    }

    size_t GetOffset() const
    {
        return m_offset;
    }

    AZ::Component* GetComponent(AZ::Entity* entity) const
    {
        return entity->FindComponent(m_componentId);
    }

    bool operator ==(const UiAnimParamData& animParamData) const
    {
        return m_componentId == animParamData.m_componentId &&
               m_typeId == animParamData.m_typeId &&
               m_offset == animParamData.m_offset;
    }

    bool operator !=(const UiAnimParamData& animParamData) const
    {
        return !(*this == animParamData);
    }

    bool operator <(const UiAnimParamData& animParamData) const
    {
        if (m_componentId != animParamData.m_componentId)
        {
            return m_componentId < animParamData.m_componentId;
        }
        else if (m_name != animParamData.m_name)
        {
            return m_name < animParamData.m_name;
        }
        else if (m_typeId != animParamData.m_typeId)
        {
            return m_typeId < animParamData.m_typeId;
        }
        else
        {
            return m_offset < animParamData.m_offset;
        }
    }


    // Serialization. Defined in UiAnimationSystem.cpp
    inline void Serialize(IUiAnimationSystem* animationSystem, XmlNodeRef& xmlNode, bool bLoading);

private:
    AZ::ComponentId     m_componentId;  // unique within the owning entity
    AZ::Uuid            m_typeId;
    AZStd::string       m_name;     // the name of the element in serialization data
    size_t              m_offset;
};

//! Types of animation track.
// Do not change values! they are serialized
//
// Attention: This should only be expanded if you add a completely new way how tracks store data!
// If you just want to control a new parameter of an entity etc. extend EParamType
//
// Note: TCB splines are only for backward compatibility, Bezier is the default
//
enum EUiAnimCurveType : unsigned int
{
    eUiAnimCurveType_TCBFloat           = 1,
    eUiAnimCurveType_TCBVector          = 2,
    eUiAnimCurveType_TCBQuat            = 3,
    eUiAnimCurveType_BezierFloat        = 4,

    eUiAnimCurveType_Unknown            = 0xFFFFFFFF
};

//! Values that animation track can hold.
// Do not change values! they are serialized
//
// Attention: This should only be expanded if you add a completely new value type that tracks can control!
// If you just want to control a new parameter of an entity etc. extend EParamType
//
// Note: If the param type of a track is known and valid these can be derived from the node.
//       These are serialized in case the parameter got invalid (for example for material nodes)
//
enum EUiAnimValue : unsigned int
{
    eUiAnimValue_Float          = 0,
    eUiAnimValue_Vector         = 1,
    eUiAnimValue_Quat           = 2,
    eUiAnimValue_Bool           = 3,
    eUiAnimValue_Select         = 5,
    eUiAnimValue_Vector2        = 13,
    eUiAnimValue_Vector3        = 14,
    eUiAnimValue_Vector4        = 15,
    eUiAnimValue_DiscreteFloat  = 16,
    eUiAnimValue_RGB            = 20,
    eUiAnimValue_CharacterAnim  = 21,

    eUiAnimValue_Unknown        = 0xFFFFFFFF
};

enum EUiTrackMask
{
    eUiTrackMask_MaskSound = 1 << 11, // Old: 1 << ATRACK_SOUND
    eUiTrackMask_MaskMusic = 1 << 14, // Old: 1 << ATRACK_MUSIC
};

//! Structure passed to Animate function.
struct SUiAnimContext
{
    SUiAnimContext()
    {
        bSingleFrame = false;
        bForcePlay = false;
        bResetting = false;
        time = 0;
        dt = 0;
        fps = 0;
        pSequence = NULL;
        trackMask = 0;
        startTime = 0;
    }

    float time;                     //!< Current time in seconds.
    float   dt;                     //!< Delta of time from previous animation frame in seconds.
    float   fps;                    //!< Last calculated frames per second value.
    bool bSingleFrame;              //!< This is not a playing animation, more a single-frame update
    bool bForcePlay;                //!< Set when force playing animation
    bool bResetting;                //!< Set when animation sequence is resetted.

    IUiAnimSequence* pSequence;     //!< Sequence in which animation performed.

    // TODO: Replace trackMask with something more type safe
    // TODO: Mask should be stored with dynamic length
    uint32 trackMask;               //!< To update certain types of tracks only
    float startTime;                //!< The start time of this playing sequence

    void Serialize(IUiAnimationSystem* animationSystem, XmlNodeRef& xmlNode, bool bLoading);
};

/** Parameters for cut-scene cameras
*/
//! Callback-class
struct IUiAnimationCallback
{
    //! Callback-reasons
    enum ECallbackReason
    {
        CBR_CHANGENODE, // Node is changing.
        CBR_CHANGETRACK // Track of the node is changing.
    };

    // <interfuscator:shuffle>
    virtual ~IUiAnimationCallback(){}
    // Called by UI animation system.
    virtual void OnUiAnimationCallback(ECallbackReason reason, IUiAnimNode* pNode) = 0;
    // </interfuscator:shuffle>
};

/** Interface of Animation Track.
*/
struct IUiAnimTrack
{
    AZ_RTTI(IUiAnimTrack, "{11C16CEC-4C03-4342-B4A7-62790E48CBD5}")

    //! Flags that can be set on animation track.
    enum EUiAnimTrackFlags
    {
        eUiAnimTrackFlags_Linear   = BIT(1), //!< Use only linear interpolation between keys.
        eUiAnimTrackFlags_Loop     = BIT(2), //!< Play this track in a loop.
        eUiAnimTrackFlags_Cycle    = BIT(3), //!< Cycle track.
        eUiAnimTrackFlags_Disabled = BIT(4), //!< Disable this track.

        //////////////////////////////////////////////////////////////////////////
        // Used by editor.
        //////////////////////////////////////////////////////////////////////////
        eUiAnimTrackFlags_Hidden   = BIT(5), //!< Set when track is hidden in UI Animation editor.
        eUiAnimTrackFlags_Muted    = BIT(8), //!< Mute this sound track. This only affects the playback in editor.
    };

    // <interfuscator:shuffle>
    virtual ~IUiAnimTrack() {};

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual EUiAnimCurveType GetCurveType() = 0;
    virtual EUiAnimValue     GetValueType() = 0;

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    // This color is used for the editor.
    virtual ColorB GetCustomColor() const = 0;
    virtual void SetCustomColor(ColorB color) = 0;
    virtual bool HasCustomColor() const = 0;
    virtual void ClearCustomColor() = 0;
#endif

    // Return what parameter of the node, this track is attached to.
    virtual const CUiAnimParamType& GetParameterType() const = 0;

    // Assign node parameter ID for this track.
    virtual void SetParameterType(CUiAnimParamType type) = 0;

    virtual const UiAnimParamData& GetParamData() const = 0;

    virtual void SetParamData(const UiAnimParamData& param) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Animation track can contain sub-tracks (Position XYZ anim track have sub-tracks for x,y,z)
    // Get count of sub tracks.
    virtual int GetSubTrackCount() const = 0;
    // Retrieve pointer the specfied sub track.
    virtual IUiAnimTrack* GetSubTrack(int nIndex) const = 0;
    virtual AZStd::string GetSubTrackName(int nIndex) const = 0;
    virtual void SetSubTrackName(int nIndex, const char* name) = 0;
    //////////////////////////////////////////////////////////////////////////

    virtual void GetKeyValueRange(float& fMin, float& fMax) const = 0;
    virtual void SetKeyValueRange(float fMin, float fMax) = 0;

    //! Return number of keys in track.
    virtual int GetNumKeys() const = 0;

    //! Return true if keys exists in this track
    virtual bool HasKeys() const = 0;

    //! Set number of keys in track.
    //! If needed adds empty keys at end or remove keys from end.
    virtual void SetNumKeys(int numKeys) = 0;

    //! Remove specified key.
    virtual void RemoveKey(int num) = 0;

    //! Get key at specified location.
    //! @param key Must be valid pointer to compatible key structure, to be filled with specified key location.
    virtual void GetKey(int index, IKey* key) const = 0;

    //! Get time of specified key.
    //! @return key time.
    virtual float GetKeyTime(int index) const = 0;

    //! Find key at given time.
    //! @return Index of found key, or -1 if key with this time not found.
    virtual int FindKey(float time) = 0;

    //! Get flags of specified key.
    //! @return key time.
    virtual int GetKeyFlags(int index) = 0;

    //! Set key at specified location.
    //! @param key Must be valid pointer to compatible key structure.
    virtual void SetKey(int index, IKey* key) = 0;

    //! Set time of specified key.
    virtual void SetKeyTime(int index, float time) = 0;

    //! Set flags of specified key.
    virtual void SetKeyFlags(int index, int flags) = 0;

    //! Sort keys in track (after time of keys was modified).
    virtual void SortKeys() = 0;

    //! Get track flags.
    virtual int GetFlags() = 0;

    //! Check if track is masked by mask
    // TODO: Mask should be stored with dynamic length
    virtual bool IsMasked(const uint32 mask) const = 0;

    //! Set track flags.
    virtual void SetFlags(int flags) = 0;

    //! Create key at given time, and return its index.
    //! @return Index of new key.
    virtual int CreateKey(float time) = 0;

    //! Clone key at specified index.
    //! @retun Index of new key.
    virtual int CloneKey(int key) = 0;

    //! Clone key at specified index from another track of SAME TYPE.
    //! @retun Index of new key.
    virtual int CopyKey(IUiAnimTrack* pFromTrack, int nFromKey) = 0;

    //! Get info about specified key.
    //! @param Short human readable text description of this key.
    //! @param duration of this key in seconds.
    virtual void GetKeyInfo(int key, const char*& description, float& duration) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Get track value at specified time.
    // Interpolates keys if needed.
    //////////////////////////////////////////////////////////////////////////
    virtual void GetValue(float time, float& value) = 0;
    virtual void GetValue(float time, Vec3& value) = 0;
    virtual void GetValue(float time, Vec4& value) = 0;
    virtual void GetValue(float time, Quat& value) = 0;
    virtual void GetValue(float time, bool& value) = 0;
    virtual void GetValue(float time, AZ::Vector2& value) = 0;
    virtual void GetValue(float time, AZ::Vector3& value) = 0;
    virtual void GetValue(float time, AZ::Vector4& value) = 0;
    virtual void GetValue(float time, AZ::Color& value) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Set track value at specified time.
    // Adds new keys if required.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetValue(float time, const float& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const Vec3& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const Vec4& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const Quat& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const bool& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const AZ::Vector2& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const AZ::Vector3& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const AZ::Vector4& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const AZ::Color& value, bool bDefault = false) = 0;

    // Only for position tracks, offset all track keys by this amount.
    virtual void OffsetKeyPosition(const AZ::Vector3& value) = 0;

    // Assign active time range for this track.
    virtual void SetTimeRange(const Range& timeRange) = 0;

    // Serialize this animation track to XML.
    virtual bool Serialize(IUiAnimationSystem* uiAnimationSystem, XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) = 0;
    virtual bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) = 0;

    virtual void InitPostLoad(IUiAnimSequence* /*sequence*/) {};

    //! For custom track animate parameters.
    virtual void Animate([[maybe_unused]] SUiAnimContext& ec) {};

    // Get access to the internal spline of the track.
    virtual ISplineInterpolator* GetSpline() const { return 0; };

    virtual bool IsKeySelected([[maybe_unused]] int key) const { return false; }

    virtual void SelectKey([[maybe_unused]] int key, [[maybe_unused]] bool select) {}

    //! Return the index of the key which lies right after the given key in time.
    //! @param key Index of of key.
    //! @return Index of the next key in time. If the last key given, this returns -1.
    // In case of keys sorted, it's just 'key+1', but if not sorted, it can be another value.
    virtual int NextKeyByTime(int key) const
    {
        if (key + 1 < GetNumKeys())
        {
            return key + 1;
        }
        else
        {
            return -1;
        }
    }

    //! Get the animation layer index assigned. (only for character/look-at tracks ATM)
    virtual int GetAnimationLayerIndex() const { return -1; }
    //! Set the animation layer index. (only for character/look-at tracks ATM)
    virtual void SetAnimationLayerIndex([[maybe_unused]] int index) { }

    // </interfuscator:shuffle>
};


/** Callback called by animation node when its animated.
*/
struct IUiAnimNodeOwner
{
    // <interfuscator:shuffle>
    virtual ~IUiAnimNodeOwner(){}
    virtual void OnNodeUiAnimated(IUiAnimNode* pNode) = 0;
    virtual void OnNodeVisibilityChanged(IUiAnimNode* pNode, const bool bHidden) = 0;
    virtual void OnNodeReset([[maybe_unused]] IUiAnimNode* pNode) {}

    // </interfuscator:shuffle>
};

/** Base class for all Animation nodes,
        can host multiple animation tracks, and execute them other time.
        Animation node is reference counted.
*/
struct IUiAnimNode
{
public:
    AZ_RTTI(IUiAnimNode, "{298180CC-B577-440C-8466-A01ABC8CC00A}")

    //////////////////////////////////////////////////////////////////////////
    // Supported params.
    //////////////////////////////////////////////////////////////////////////
    enum ESupportedParamFlags
    {
        eSupportedParamFlags_MultipleTracks = 0x01, // Set if parameter can be assigned multiple tracks.
    };

    // <interfuscator:shuffle>
    virtual ~IUiAnimNode() {};

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    //! Set node name.
    virtual void SetName(const char* name) = 0;

    //! Get node name.
    virtual AZStd::string GetName() = 0;

    // Get Type of this node.
    virtual EUiAnimNodeType GetType() const = 0;

    // Return Animation Sequence that owns this node.
    virtual IUiAnimSequence* GetSequence() const = 0;

    // Set the Animation Sequence that owns this node.
    virtual void SetSequence(IUiAnimSequence* sequence) = 0;

    // Called when sequence is activated / deactivated
    virtual void Activate(bool bActivate) = 0;

    // Set AnimNode flags.
    // @param flags One or more flags from EUiAnimNodeFlags.
    // @see EUiAnimNodeFlags
    virtual void SetFlags(int flags) = 0;

    // Get AnimNode flags.
    // @return flags set on node.
    // @see EUiAnimNodeFlags
    virtual int GetFlags() const = 0;

    // Return animation system that created this node.
    virtual IUiAnimationSystem* GetUiAnimationSystem() const = 0;

    // General Set param.
    // Set float/vec3/vec4 parameter at given time.
    // @return true if parameter set, false if this parameter not exist in node.
    virtual bool SetParamValue(float time, CUiAnimParamType param, float value) = 0;
    virtual bool SetParamValue(float time, CUiAnimParamType param, const Vec3& value) = 0;
    virtual bool SetParamValue(float time, CUiAnimParamType param, const Vec4& value) = 0;
    // Get float/vec3/vec4 parameter at given time.
    // @return true if parameter exist, false if this parameter not exist in node.
    virtual bool GetParamValue(float time, CUiAnimParamType param, float& value) = 0;
    virtual bool GetParamValue(float time, CUiAnimParamType param, Vec3& value) = 0;
    virtual bool GetParamValue(float time, CUiAnimParamType param, Vec4& value) = 0;

    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] float value) { return false; }
    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] bool value) { return false; }
    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] int value) { return false; }
    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] unsigned int value) { return false; }
    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] const AZ::Vector2& value) { return false; }
    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] const AZ::Vector3& value) { return false; }
    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] const AZ::Vector4& value) { return false; }
    virtual bool SetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] const AZ::Color& value) { return false; }

    virtual bool GetParamValueAz([[maybe_unused]] float time, [[maybe_unused]] const UiAnimParamData& param, [[maybe_unused]] float& value) { return false; }

    //! Evaluate animation node while not playing animation.
    virtual void StillUpdate() = 0;

    //! Evaluate animation to the given time.
    virtual void Animate(SUiAnimContext& ec) = 0;

    // Description:
    //      Returns number of supported parameters by this animation node (position,rotation,scale,etc..).
    // Returns:
    //      Number of supported parameters.
    virtual unsigned int GetParamCount() const = 0;

    // Description:
    //      Returns the type of a param by index
    // Arguments:
    //      nIndex - parameter index in range 0 <= nIndex < GetSupportedParamCount()
    virtual CUiAnimParamType GetParamType(unsigned int nIndex) const = 0;

    // Description:
    //      Check if parameter is supported by this node.
    virtual bool IsParamValid(const CUiAnimParamType& paramType) const = 0;

    // Description:
    //      Returns name of supported parameter of this animation node or NULL if not available
    // Arguments:
    //          paramType - parameter id
    virtual AZStd::string GetParamName(const CUiAnimParamType& paramType) const = 0;

    // Description:
    //      Returns name of supported parameter of this animation node or NULL if not available
    // Arguments:
    //          paramType - parameter id
    virtual AZStd::string GetParamNameForTrack(const CUiAnimParamType& paramType, [[maybe_unused]] const IUiAnimTrack* track) const { return GetParamName(paramType); }

    // Description:
    //      Returns the params value type
    virtual EUiAnimValue GetParamValueType(const CUiAnimParamType& paramType) const = 0;

    // Description:
    //      Returns the params value type
    virtual ESupportedParamFlags GetParamFlags(const CUiAnimParamType& paramType) const = 0;

    // Called node data is re-initialized, such as when changing the entity associated with it.
    virtual void OnReset() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Working with Tracks.
    //////////////////////////////////////////////////////////////////////////
    virtual int GetTrackCount() const = 0;

    // Return track assigned to the specified parameter.
    virtual IUiAnimTrack* GetTrackByIndex(int nIndex) const = 0;

    // Return first track assigned to the specified parameter.
    virtual IUiAnimTrack* GetTrackForParameter(const CUiAnimParamType& paramType) const = 0;

    // Return the i-th track assigned to the specified parameter in case of multiple tracks.
    virtual IUiAnimTrack* GetTrackForParameter(const CUiAnimParamType& paramType, uint32 index) const = 0;

    // Get the index of a given track among tracks with the same parameter type in this node.
    virtual uint32 GetTrackParamIndex(const IUiAnimTrack* pTrack) const = 0;

    // Creates a new track for given parameter.
    virtual IUiAnimTrack* CreateTrack(const CUiAnimParamType& paramType) = 0;

    // Return track assigned to the specified class element.
    // Maybe we should pass nameCrc rather than a void*
    virtual IUiAnimTrack* GetTrackForAzField(const UiAnimParamData& param) const = 0;

    // Creates a new track for given parameter.
    // Maybe we should pass nameCrc rather than a void*
    virtual IUiAnimTrack* CreateTrackForAzField(const UiAnimParamData& param) = 0;

    // Assign animation track to parameter.
    // if track parameter is NULL track with parameter id param will be removed.
    virtual void SetTrack(const CUiAnimParamType& paramType, IUiAnimTrack* track) = 0;

    // Set time range for all tracks in this sequence.
    virtual void SetTimeRange(Range timeRange) = 0;

    // Remove track from anim node.
    virtual void AddTrack(IUiAnimTrack* pTrack) = 0;

    // Remove track from anim node.
    virtual bool RemoveTrack(IUiAnimTrack* pTrack) = 0;

    // Description:
    //      Creates default set of tracks supported by this node.
    virtual void CreateDefaultTracks() = 0;
    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Callback for animation node used by editor.
    // Register notification callback with animation node.
    virtual void SetNodeOwner(IUiAnimNodeOwner* pOwner) = 0;
    virtual IUiAnimNodeOwner* GetNodeOwner() = 0;

    // Serialize this animation node to XML.
    virtual void SerializeUiAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) = 0;
    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) = 0;

    // Sets up internal pointers post load from Sequence Component
    virtual void InitPostLoad(IUiAnimSequence* pSequence, bool remapIds, LyShine::EntityIdMap* entityIdMap) = 0;

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Groups interface
    //////////////////////////////////////////////////////////////////////////
    virtual void SetParent(IUiAnimNode* pParent) = 0;
    virtual IUiAnimNode* GetParent() const = 0;
    virtual IUiAnimNode* HasDirectorAsParent() const = 0;
    //////////////////////////////////////////////////////////////////////////

    virtual void Render() = 0;

    virtual bool NeedToRender() const = 0;

    // Called from editor if dynamic params need updating
    virtual void UpdateDynamicParams() = 0;
    // </interfuscator:shuffle>
};

//! Track event listener
struct IUiTrackEventListener
{
    //! Reasons
    enum ETrackEventReason
    {
        eTrackEventReason_Added,
        eTrackEventReason_Removed,
        eTrackEventReason_Renamed,
        eTrackEventReason_Triggered,
        eTrackEventReason_MovedUp,
        eTrackEventReason_MovedDown,
    };

    // <interfuscator:shuffle>
    virtual ~IUiTrackEventListener(){}
    // Description:
    //      Called when track event is updated
    // Arguments:
    //      pSeq - Animation sequence
    //      reason - Reason for update (see EReason)
    //      event - Track event added
    //      pUserData - Data to accompany reason
    virtual void OnTrackEvent(IUiAnimSequence* pSequence, int reason, const char* event, void* pUserData) = 0;
    // </interfuscator:shuffle>
};

struct IUiAnimSequenceOwner
{
    // <interfuscator:shuffle>
    virtual ~IUiAnimSequenceOwner() {}
    virtual void OnModified() = 0;
    // </interfuscator:shuffle>
};

struct IUiAnimStringTable
{
    AZ_RTTI(IUiAnimStringTable, "{5B60054D-0D67-4DB5-B867-9869DAB95B83}")
    virtual ~IUiAnimStringTable() {}

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    virtual const char* Add(const char* p) = 0;
};

/** Animation sequence, operates on animation nodes contained in it.
 */
struct IUiAnimSequence
{
    AZ_RTTI(IUiAnimSequence, "{74EFA085-7758-4275-98A1-4D40DC6F55B8}")

    static const int kSequenceVersion = 3;

    //! Flags used for SetFlags(),GetFlags(),SetParentFlags(),GetParentFlags() methods.
    enum EUiAnimSequenceFlags
    {
        eSeqFlags_PlayOnReset          = BIT(0),   //!< Start playing this sequence immediately after reset of UI animation system(Level load).
        eSeqFlags_OutOfRangeConstant   = BIT(1),   //!< Constant Out-Of-Range,time continues normally past sequence time range.
        eSeqFlags_OutOfRangeLoop       = BIT(2),   //!< Loop Out-Of-Range,time wraps back to the start of range when reaching end of range.
        eSeqFlags_CutScene             = BIT(3),   //!< Cut scene sequence.
        eSeqFlags_NoHUD                = BIT(4),   //!< @deprecated - Don`t display HUD
        eSeqFlags_NoPlayer             = BIT(5),   //!< Disable input and drawing of player
        eSeqFlags_16To9                = BIT(8),   //!< 16:9 bars in sequence
        eSeqFlags_NoGameSounds         = BIT(9),   //!< Suppress all game sounds.
        eSeqFlags_NoSeek               = BIT(10),  //!< Cannot seek in sequence.
        eSeqFlags_NoAbort              = BIT(11),  //!< Cutscene can not be aborted
        eSeqFlags_NoSpeed              = BIT(13),  //!< Cannot modify sequence speed - TODO: add interface control if required
     // eSeqFlags_CanWarpInFixedTime   = BIT(14),  //!< @deprecated - Timewarp by scaling a fixed time step - removed July 2017
        eSeqFlags_EarlyAnimationUpdate = BIT(15),  //!< Turn the 'sys_earlyUiAnimationUpdate' on during the sequence.
        eSeqFlags_LightAnimationSet    = BIT(16),  //!< A special unique sequence for light animations
        eSeqFlags_NoMPSyncingNeeded    = BIT(17),  //!< this sequence doesn't require MP net syncing
    };

    virtual ~IUiAnimSequence() {};

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    //! Set the animation system for the canvas containing this sequence
    virtual IUiAnimationSystem* GetUiAnimationSystem() const = 0;

    //! Set the name of this sequence. (ex. "Intro" in the same case as above)
    virtual void SetName(const char* name) = 0;
    //! Get the name of this sequence. (ex. "Intro" in the same case as above)
    virtual const char* GetName() const = 0;
    //! Get the ID (unique in a level and consistent across renaming) of this sequence.
    virtual uint32 GetId () const = 0;

    virtual void SetOwner(IUiAnimSequenceOwner* pOwner) = 0;
    virtual IUiAnimSequenceOwner* GetOwner() const = 0;

    //! Set the currently active director node.
    virtual void SetActiveDirector(IUiAnimNode* pDirectorNode) = 0;
    //! Get the currently active director node, if any.
    virtual IUiAnimNode* GetActiveDirector() const = 0;

    //! Set animation sequence flags
    virtual void SetFlags(int flags) = 0;
    //! Get animation sequence flags
    virtual int GetFlags() const = 0;
    //! Get cutscene related animation sequence flags
    virtual int GetCutSceneFlags(const bool localFlags = false) const = 0;
    //! Set parent animation sequence
    virtual void SetParentSequence(IUiAnimSequence* pParentSequence) = 0;
    //! Get parent animation sequence
    virtual const IUiAnimSequence* GetParentSequence() const = 0;
    //! Check whether this sequence has the given sequence as a descendant through one of its sequence tracks.
    virtual bool IsAncestorOf(const IUiAnimSequence* sequence) const = 0;

    //! Return number of animation nodes in sequence.
    virtual int GetNodeCount() const = 0;
    //! Get animation node at specified index.
    virtual IUiAnimNode* GetNode(int index) const = 0;

    //! Add animation node to sequence.
    //! @return True if node added, same node will not be added 2 times.
    virtual bool AddNode(IUiAnimNode* node) = 0;

    // Reorders the array of nodes, so the specified node is placed after or before the given pivot node depending on the parameter 'next'.
    virtual void ReorderNode(IUiAnimNode* node, IUiAnimNode* pPivotNode, bool next) = 0;

    // Description:
    //      Creates a new animation node with specified type.
    // Arguments:
    //      nodeType - Type of node, one of EUiAnimNodeType enums.
    virtual IUiAnimNode* CreateNode(EUiAnimNodeType nodeType) = 0;

    // Description:
    //      Creates a new animation node from serialized node XML.
    // Arguments:
    //      node - Serialized form of node
    virtual IUiAnimNode* CreateNode(XmlNodeRef node) = 0;

    //! Remove animation node from sequence.
    virtual void RemoveNode(IUiAnimNode* node) = 0;

    // Finds node by name, can be slow.
    // If the node belongs to a director, the director node also should be given
    // since there can be multiple instances of the same node(i.e. the same name)
    // across multiple director nodes.
    virtual IUiAnimNode* FindNodeByName(const char* sNodeName, const IUiAnimNode* pParentDirector) = 0;

    //! Remove all nodes from sequence.
    virtual void RemoveAll() = 0;

    // Activate sequence by binding sequence animations to nodes.
    // must be called prior animating sequence.
    virtual void Activate() = 0;

    /** Check if sequence is activated
    */
    virtual bool IsActivated() const = 0;

    // Deactivates sequence by unbinding sequence animations from nodes.
    virtual void Deactivate() = 0;

    // Pre-caches data associated with this anim sequence.
    virtual void PrecacheData(float startTime = 0.0f) = 0;

    // Update sequence while not playing animation.
    virtual void StillUpdate() = 0;

    // Render function call for some special node.
    virtual void Render() = 0;

    // Evaluate animations of all nodes in sequence.
    // Sequence must be activated before animating.
    virtual void Animate(const SUiAnimContext& ec) = 0;

    //! Set time range of this sequence.
    virtual void SetTimeRange(Range timeRange) = 0;

    //! Get time range of this sequence.
    virtual Range GetTimeRange() = 0;

    //! Resets the sequence
    virtual void Reset(bool bSeekToStart) = 0;

    //! This can have more time-consuming tasks performed additional to tasks of the usual 'Reset()' method.
    virtual void ResetHard() = 0;

    // Called to pause sequence.
    virtual void Pause() = 0;

    // Called to resume sequence.
    virtual void Resume() = 0;

    /** Called to check if sequence is paused.
    */
    virtual bool IsPaused() const = 0;

    /** Called when a sequence is looped.
    */
    virtual void OnLoop() = 0;

    /** Move/Scale all keys in tracks from previous time range to new time range.
    */
    virtual void AdjustKeysToTimeRange(const Range& timeRange) = 0;

    // Serialize this sequence to XML.
    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true, uint32 overrideId = 0, bool bUndo = false) = 0;
    virtual void InitPostLoad(IUiAnimationSystem* pUiAnimationSystem, bool remapIds, LyShine::EntityIdMap* entityIdMap) = 0;

    // Copy some nodes of this sequence to XML.
    virtual void CopyNodes(XmlNodeRef& xmlNode, IUiAnimNode** pSelectedNodes, uint32 count) = 0;

    // Paste nodes given by the XML to this sequence.
    virtual void PasteNodes(const XmlNodeRef& xmlNode, IUiAnimNode* pParent) = 0;

    // Summary:
    //  Adds/removes track events in sequence.
    virtual bool AddTrackEvent(const char* szEvent) = 0;
    virtual bool RemoveTrackEvent(const char* szEvent) = 0;
    virtual bool RenameTrackEvent(const char* szEvent, const char* szNewEvent) = 0;
    virtual bool MoveUpTrackEvent(const char* szEvent) = 0;
    virtual bool MoveDownTrackEvent(const char* szEvent) = 0;
    virtual void ClearTrackEvents() = 0;

    // Summary:
    //  Gets the track events in the sequence.
    virtual int GetTrackEventsCount() const = 0;
    // Summary:
    //  Gets the specified track event in the sequence.
    virtual char const* GetTrackEvent(int iIndex) const = 0;

    virtual IUiAnimStringTable* GetTrackEventStringTable() = 0;

    // Summary:
    //   Called to trigger a track event.
    virtual void TriggerTrackEvent(const char* event, const char* param = NULL) = 0;

    //! Track event listener
    virtual void AddTrackEventListener(IUiTrackEventListener* pListener) = 0;
    virtual void RemoveTrackEventListener(IUiTrackEventListener* pListener) = 0;

    // </interfuscator:shuffle>
};

/** UI animation Listener interface.
    Register at UI animation system to get notified about UI animation events
*/
struct IUiAnimationListener
{
    enum EUiAnimationEvent
    {
        eUiAnimationEvent_Started = 0,   // fired when sequence is started
        eUiAnimationEvent_Stopped,       // fired when sequence ended normally
        eUiAnimationEvent_Aborted,       // fired when sequence was aborted before normal end (STOP and ABORTED event are mutually exclusive!)
        eUiAnimationEvent_Updated,       // fired after sequence time or playback speed was updated
    };

    // <interfuscator:shuffle>
    virtual ~IUiAnimationListener(){}
    //! callback on UI animation events
    virtual void OnUiAnimationEvent(EUiAnimationEvent uiAnimationEvent, IUiAnimSequence* pAnimSequence) = 0;
    virtual void OnUiTrackEvent(AZStd::string eventName, AZStd::string valueName, [[maybe_unused]] IUiAnimSequence* pAnimSequence) {}
    // </interfuscator:shuffle>
};

/** UI Animation System interface.
        Main entrance point to UI Animation capability.
        Enumerate available UI Animation sequences, update all UI Animation sequences, create animation nodes and tracks.
 */
struct IUiAnimationSystem
{
    AZ_RTTI(IUiAnimationSystem, "{26D795DD-6732-4A2F-81A5-B17B53A3ADAA}")

    enum ESequenceStopBehavior
    {
        eSSB_LeaveTime = 0,         // When sequence is stopped it remains in last played time.
        eSSB_GotoEndTime = 1,       // Default behavior in game, sequence is animated at end time before stop.
        eSSB_GotoStartTime = 2, // Default behavior in editor, sequence is animated at start time before stop.
    };

    // <interfuscator:shuffle>
    virtual ~IUiAnimationSystem(){}

    //! Release UI animation system.
    virtual void Release() = 0;
    //! Loads all nodes and sequences from a specific file (should be called when the level is loaded).
    virtual bool Load(const char* pszFile, const char* pszMission) = 0;

    // Description:
    //      Creates a new animation track with specified type.
    // Arguments:
    //      type - Type of track, one of EUiAnimTrackType enums.
    virtual IUiAnimTrack* CreateTrack(EUiAnimCurveType type) = 0;

    virtual IUiAnimSequence* CreateSequence(const char* sequence, bool bLoad = false, uint32 id = 0) = 0;
    virtual IUiAnimSequence* LoadSequence(XmlNodeRef& xmlNode, bool bLoadEmpty = true) = 0;
    virtual void AddSequence(IUiAnimSequence* pSequence) = 0;
    virtual void RemoveSequence(IUiAnimSequence* pSequence) = 0;
    virtual IUiAnimSequence* FindSequence(const char* sequence) const = 0;
    virtual IUiAnimSequence* FindSequenceById(uint32 id) const = 0;
    virtual IUiAnimSequence* GetSequence(int i) const = 0;
    virtual int GetNumSequences() const = 0;
    virtual IUiAnimSequence* GetPlayingSequence(int i) const = 0;
    virtual int GetNumPlayingSequences() const = 0;
    virtual bool IsCutScenePlaying() const = 0;

    virtual uint32 GrabNextSequenceId() = 0;
    //////////////////////////////////////////////////////////////////////////
    //
    // If the name of a sequence changes, the keys that refer it in the
    // sequence track of the director node should be properly updated also.
    //
    // @param   before The old name of the sequence.
    // @param   after The new name of the sequence.
    // @return  Number of modified sequence keys.
    //
    //////////////////////////////////////////////////////////////////////////
    virtual int OnSequenceRenamed(const char* before, const char* after) = 0;
    //////////////////////////////////////////////////////////////////////////
    //
    // If the name of a camera changes, the keys that refer it in the
    // camera track of the director node should be properly updated also.
    // This updates the name of the corresponding camera node also, if any.
    //
    // @param   before The old name of the camera.
    // @param   after The new name of the camera.
    // @return  Number of modified camera keys.
    //
    //////////////////////////////////////////////////////////////////////////
    virtual int OnCameraRenamed(const char* before, const char* after) = 0;

    // Adds a listener to a sequence
    // @param pSequence Pointer to sequence
    // @param pListener Pointer to an IUiAnimationListener
    // @return true on successful add, false otherwise
    virtual bool AddUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener) = 0;

    // Removes a listener from a sequence
    // @param pSequence Pointer to sequence
    // @param pListener Pointer to an IUiAnimationListener
    // @return true on successful removal, false otherwise
    virtual bool RemoveUiAnimationListener(IUiAnimSequence* pSequence, IUiAnimationListener* pListener) = 0;

    virtual ISystem* GetSystem() = 0;

    // Remove all sequences from UI animation system.
    virtual void RemoveAllSequences() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Sequence playback.
    //////////////////////////////////////////////////////////////////////////
    // Start playing sequence.
    // Call ignored if sequence is already playing.
    // @param sequence Name of sequence to play.
    virtual void PlaySequence(const char* pSequenceName, IUiAnimSequence* pParentSeq, bool bResetFX, bool bTrackedSequence, float startTime = -FLT_MAX, float endTime = -FLT_MAX) = 0;

    // Start playing sequence.
    // Call ignored if sequence is already playing.
    // @param sequence Pointer to Valid sequence to play.
    virtual void PlaySequence(IUiAnimSequence* pSequence, IUiAnimSequence* pParentSeq, bool bResetFX, bool bTrackedSequence, float startTime = -FLT_MAX, float endTime = -FLT_MAX) = 0;

    // Stops currently playing sequence.
    // Ignored if sequence is not playing.
    // Returns true if sequence has been stopped. false otherwise
    // @param sequence Name of playing sequence to stop.
    virtual bool StopSequence(const char* pSequenceName) = 0;

    // Stop's currently playing sequence.
    // Ignored if sequence is not playing.
    // Returns true if sequence has been stopped. false otherwise
    // @param sequence Pointer to Valid sequence to stop.
    virtual bool StopSequence(IUiAnimSequence* pSequence) = 0;

    /** Aborts a currently playing sequence.
            Ignored if sequence is not playing.
            Calls IUiAnimationListener with aborted event (done event is NOT called)
            Returns true if sequence has been aborted. false otherwise
            @param sequence Pointer to Valid sequence to stop.
            @param bLeaveTime If false, uses default stop behavior, otherwise leaves the sequence at time
    */
    virtual bool AbortSequence(IUiAnimSequence* pSequence, bool bLeaveTime = false) = 0;

    // Stops all currently playing sequences.
    virtual void StopAllSequences() = 0;

    // Stops all playing cut-scene sequences.
    // This will not stop all sequences, but only those with CUT_SCENE flag set.
    virtual void StopAllCutScenes() = 0;

    // Checks if specified sequence is playing.
    virtual bool IsPlaying(IUiAnimSequence* seq) const = 0;

    /** Resets playback state of UI animation system,
            usually called after loading of level,
    */
    virtual void Reset(bool bPlayOnReset, bool bSeekToStart) = 0;

    // Sequences with PLAY_ONRESET flag will start playing after this call.
    virtual void PlayOnLoadSequences() = 0;

    // Update UI animation system while not playing animation.
    virtual void StillUpdate() = 0;

    // Updates UI animation system every frame before the entity system to animate all playing sequences.
    virtual void PreUpdate(const float dt) = 0;

    // Updates UI animation system every frame after the entity system to animate all playing sequences.
    virtual void PostUpdate(const float dt) = 0;

    // Render function call of some special node.
    virtual void Render() = 0;

    // Set UI animation system into recording mode,
    // While in recording mode any changes made to node will be added as keys to tracks.
    virtual void SetRecording(bool recording) = 0;
    virtual bool IsRecording() const = 0;

    // Pause any playing sequences.
    virtual void Pause() = 0;

    // Resume playing sequences.
    virtual void Resume() = 0;

    // Callback when animation-data changes
    virtual void SetCallback(IUiAnimationCallback* pCallback) = 0;

    virtual IUiAnimationCallback* GetCallback() = 0;

    // Serialize to XML.
    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bRemoveOldNodes = false, bool bLoadEmpty = true) = 0;
    virtual void InitPostLoad(bool remapIds, LyShine::EntityIdMap* entityIdMap) = 0;

    // Gets the float time value for a sequence that is already playing
    virtual float GetPlayingTime(IUiAnimSequence* pSeq) = 0;
    virtual float GetPlayingSpeed(IUiAnimSequence* pSeq) = 0;
    // Sets the time progression of an already playing cutscene.
    // If IUiAnimSequence:NO_SEEK flag is set on pSeq, this call is ignored.
    virtual bool SetPlayingTime(IUiAnimSequence* pSeq, float fTime) = 0;
    virtual bool SetPlayingSpeed(IUiAnimSequence* pSeq, float fSpeed) = 0;
    // Set behavior pattern for stopping sequences.
    virtual void SetSequenceStopBehavior(ESequenceStopBehavior behavior) = 0;

    // Set the start and end time of an already playing cutscene.
    virtual bool GetStartEndTime(IUiAnimSequence* pSeq, float& fStartTime, float& fEndTime) = 0;
    virtual bool SetStartEndTime(IUiAnimSequence* pSeq, const float fStartTime, const float fEndTime) = 0;

    // Make the specified sequence go to a given frame time.
    // @param seqName A sequence name.
    // @param targetFrame A target frame to go to in time.
    virtual void GoToFrame(const char* seqName, float targetFrame) = 0;

    // Get behavior pattern for stopping sequences.
    virtual IUiAnimationSystem::ESequenceStopBehavior GetSequenceStopBehavior() = 0;

    // Should only be called from CUiAnimParamType
    virtual void SerializeParamType(CUiAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version) = 0;

    // Should only be called from UiAnimParamData
    virtual void SerializeParamData(UiAnimParamData& animParamData, XmlNodeRef& xmlNode, bool bLoading) = 0;

    // Called by a sequence whenever a track event is triggered
    virtual void NotifyTrackEventListeners(const char* eventName, const char* valueName, IUiAnimSequence* pSequence) = 0;

#ifdef UI_ANIMATION_SYSTEM_SUPPORT_EDITING
    virtual EUiAnimNodeType GetNodeTypeFromString(const char* pString) const = 0;
    virtual CUiAnimParamType GetParamTypeFromString(const char* pString) const = 0;
#endif

    // </interfuscator:shuffle>
};

inline void SUiAnimContext::Serialize(IUiAnimationSystem* animationSystem, XmlNodeRef& xmlNode, bool bLoading)
{
    if (bLoading)
    {
        XmlString name;
        if (xmlNode->getAttr("sequence", name))
        {
            pSequence = animationSystem->FindSequence(name.c_str());
        }
        xmlNode->getAttr("dt", dt);
        xmlNode->getAttr("fps", fps);
        xmlNode->getAttr("time", time);
        xmlNode->getAttr("bSingleFrame", bSingleFrame);
        xmlNode->getAttr("bResetting", bResetting);
        xmlNode->getAttr("trackMask", trackMask);
        xmlNode->getAttr("startTime", startTime);
    }
    else
    {
        if (pSequence)
        {
            AZStd::string fullname = pSequence->GetName();
            xmlNode->setAttr("sequence", fullname.c_str());
        }
        xmlNode->setAttr("dt", dt);
        xmlNode->setAttr("fps", fps);
        xmlNode->setAttr("time", time);
        xmlNode->setAttr("bSingleFrame", bSingleFrame);
        xmlNode->setAttr("bResetting", bResetting);
        xmlNode->setAttr("trackMask", trackMask);
        xmlNode->setAttr("startTime", startTime);
    }
}

void CUiAnimParamType::Serialize(IUiAnimationSystem* animationSystem, XmlNodeRef& xmlNode, bool bLoading, const uint version)
{
    animationSystem->SerializeParamType(*this, xmlNode, bLoading, version);
}

void UiAnimParamData::Serialize(IUiAnimationSystem* animationSystem, XmlNodeRef& xmlNode, bool bLoading)
{
    animationSystem->SerializeParamData(*this, xmlNode, bLoading);
}
