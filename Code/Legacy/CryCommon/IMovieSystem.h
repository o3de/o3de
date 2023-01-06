/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <Range.h>
#include <AnimKey.h>
#include <ISplines.h>

#define DEFAULT_NEAR 0.2f
#define DEFAULT_FOV (75.0f * gf_PI / 180.0f)

// forward declaration.
struct IAnimTrack;
struct IAnimNode;
struct IAnimSequence;
struct IMovieSystem;
struct IKey;
class XmlNodeRef;
struct ISplineInterpolator;
struct ILightAnimWrapper;

namespace AZ
{
    namespace Data
    {
        class AssetData;
    }
}

namespace Maestro
{
    template<typename AssetType>
    struct AssetBlends;
}

typedef IMovieSystem* (* PFNCREATEMOVIESYSTEM)(struct ISystem*);

#define LIGHT_ANIMATION_SET_NAME "_LightAnimationSet"
#define MAX_ANIM_NAME_LENGTH 64
//! Very high priority for cut scene sounds.
#define MOVIE_SOUND_PRIORITY 230

#if !defined(CONSOLE)
#define MOVIESYSTEM_SUPPORT_EDITING
#endif

typedef std::vector<IAnimSequence*> AnimSequences;
typedef AZStd::vector<AZStd::string> TrackEvents;

// Forward declare
enum class SequenceType;
enum class AnimNodeType;
enum class AnimValueType;
enum class AnimParamType;

// TODO: Refactor headers that are using these values so they are not need in header files

// AnimValueType::Float is the default value
#define kAnimValueDefault static_cast<AnimValueType>(0)
// AnimValueType::Unknown
#define kAnimValueUnknown static_cast<AnimValueType>(0xFFFFFFFF)
// SequenceType::SequenceComponent is the default value
#define kSequenceTypeDefault static_cast<SequenceType>(1)
// AnimParamType::Invalid
#define kAnimParamTypeInvalid static_cast<AnimParamType>(0xFFFFFFFF)
// AnimParamType::ByString
#define kAnimParamTypeByString static_cast<AnimParamType>(8)

//! Flags that can be set on animation node.
enum EAnimNodeFlags
{
    eAnimNodeFlags_Expanded                 = BIT(0), //!< Deprecated, handled by sandbox now
    eAnimNodeFlags_EntitySelected           = BIT(1), //!< Set if the referenced entity is selected in the editor
    eAnimNodeFlags_CanChangeName            = BIT(2), //!< Set if this node allow changing of its name.
    eAnimNodeFlags_Disabled                 = BIT(3), //!< Disable this node.
    eAnimNodeFlags_DisabledForComponent     = BIT(4), //!< Disable this node for a disabled or pending entity component.
};

enum ENodeExportType
{
    eNodeExportType_Global = 0,
    eNodeExportType_Local = 1,
};

// Common parameters of animation node.
class CAnimParamType
{
    friend class CMovieSystem;
    friend class AnimSerializer;

public:
    AZ_TYPE_INFO(CAnimParamType, "{E2F34955-3B07-4241-8D34-EA3BEF3B33D2}")

    static const uint kParamTypeVersion = 9;

    CAnimParamType()
        : m_type(kAnimParamTypeInvalid) {}

    CAnimParamType(const AZStd::string& name)
    {
        *this = name;
    }

    CAnimParamType(AnimParamType type)
    {
        *this = type;
    }

    // Convert from old enum or int
    void operator =(AnimParamType type)
    {
        m_type = type;
    }

    void operator =(const AZStd::string& name)
    {
        m_type = kAnimParamTypeByString;
        m_name = name;
    }

    // Convert to enum. This needs to be explicit,
    // otherwise operator== will be ambiguous
    AnimParamType GetType() const { return m_type; }

    // Get name
    const char* GetName() const { return m_name.c_str(); }

    bool operator ==(const CAnimParamType& animParamType) const
    {
        if (m_type == kAnimParamTypeByString && animParamType.m_type == kAnimParamTypeByString)
        {
            return m_name == animParamType.m_name;
        }

        return m_type == animParamType.m_type;
    }

    bool operator !=(const CAnimParamType& animParamType) const
    {
        return !(*this == animParamType);
    }

    bool operator <(const CAnimParamType& animParamType) const
    {
        if (m_type == kAnimParamTypeByString && animParamType.m_type == kAnimParamTypeByString)
        {
            return m_name < animParamType.m_name;
        }
        else if (m_type == kAnimParamTypeByString)
        {
            return false; // Always sort named params last
        }
        else if (animParamType.m_type == kAnimParamTypeByString)
        {
            return true; // Always sort named params last
        }
        if (m_type < animParamType.m_type)
        {
            return true;
        }

        return false;
    }


    void SaveToXml(XmlNodeRef& xmlNode) const;
    void LoadFromXml(const XmlNodeRef& xmlNode, const uint version = kParamTypeVersion);

    // Serialization. Defined in Movie.cpp
    inline void Serialize(XmlNodeRef& xmlNode, bool bLoading, const uint version = kParamTypeVersion);

private:
    AnimParamType m_type;
    AZStd::string m_name;
};

namespace AZStd
{
    // define hasher to allow for AZStd maps and sets of CAnimParamType
    template <>
    struct hash < CAnimParamType >
    {
        inline size_t operator()(const CAnimParamType& paramType) const
        {
            AZStd::hash<AnimParamType> paramTypeHasher;
            size_t retVal = paramTypeHasher(paramType.GetType());
            AZStd::hash_combine(retVal, AZ::Crc32(paramType.GetName()));
            return retVal;
        }
    };
}  // namespace AZStd

//! Types of animation track.
// Do not change values! they are serialized
//
// Attention: This should only be expanded if you add a completely new way how tracks store data!
// If you just want to control a new parameter of an entity etc. extend EParamType
//
// Note: TCB splines are only for backward compatibility, Bezier is the default
//
enum EAnimCurveType : unsigned int
{
    eAnimCurveType_TCBFloat         = 1,
    eAnimCurveType_TCBVector        = 2,
    eAnimCurveType_TCBQuat          = 3,
    eAnimCurveType_BezierFloat      = 4,

    eAnimCurveType_Unknown          = 0xFFFFFFFF
};

enum ETrackMask
{
    eTrackMask_MaskSound = 1 << 11, // Old: 1 << ATRACK_SOUND
};

//! Structure passed to Animate function.
struct SAnimContext
{
    SAnimContext()
    {
        singleFrame = false;
        forcePlay = false;
        resetting = false;
        time = 0;
        dt = 0;
        fps = 0;
        sequence = nullptr;
        trackMask = 0;
        startTime = 0;
    }

    float time;             //!< Current time in seconds.
    float dt;               //!< Delta of time from previous animation frame in seconds.
    float fps;              //!< Last calculated frames per second value.
    bool singleFrame;       //!< This is not a playing animation, more a single-frame update.
    bool forcePlay;         //!< Set when force playing animation.
    bool resetting;         //!< Set when animation sequence is resetting.

    IAnimSequence* sequence; //!< Sequence in which animation performed.

    // TODO: Replace trackMask with something more type safe
    // TODO: Mask should be stored with dynamic length
    uint32 trackMask;       //!< To update certain types of tracks only
    float startTime;        //!< The start time of this playing sequence
};

/** Parameters for cut-scene cameras
*/
struct SCameraParams
{
    SCameraParams()
    {
        fov = 0.0f;
        nearZ = DEFAULT_NEAR;
        justActivated = false;
    }
    AZ::EntityId cameraEntityId;
    float fov;
    float nearZ;
    bool justActivated;
};

//! Interface for movie-system implemented by user for advanced function-support
struct IMovieUser
{
    // <interfuscator:shuffle>
    virtual ~IMovieUser(){}
    //! Called when movie system requests a camera-change.
    virtual void SetActiveCamera(const SCameraParams& Params) = 0;
    //! Called when movie system enters into cut-scene mode.
    virtual void BeginCutScene(IAnimSequence* pSeq, unsigned long dwFlags, bool bResetFX) = 0;
    //! Called when movie system exits from cut-scene mode.
    virtual void EndCutScene(IAnimSequence* pSeq, unsigned long dwFlags) = 0;
    //! Called when movie system wants to send a global event.
    virtual void SendGlobalEvent(const char* pszEvent) = 0;
    // </interfuscator:shuffle>
};

//! Callback-class
struct IMovieCallback
{
    //! Callback-reasons
    enum ECallbackReason
    {
        CBR_CHANGENODE, // Node is changing.
        CBR_CHANGETRACK // Track of the node is changing.
    };

    // <interfuscator:shuffle>
    virtual ~IMovieCallback(){}
    // Called by movie system.
    virtual void OnMovieCallback(ECallbackReason reason, IAnimNode* pNode) = 0;
    virtual void OnSetCamera(const SCameraParams& Params) = 0;
    virtual bool IsSequenceCamUsed() const = 0;
    // </interfuscator:shuffle>
};

/** Interface of Animation Track.
*/
struct IAnimTrack
{
    AZ_RTTI(IAnimTrack, "{AA0D5170-FB28-426F-BA13-7EFF6BB3AC67}");
    AZ_CLASS_ALLOCATOR(IAnimTrack, AZ::SystemAllocator, 0);

    static void Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<IAnimTrack>();
        }
    }

    //! Flags that can be set on animation track.
    enum EAnimTrackFlags
    {
        eAnimTrackFlags_Linear   = BIT(1), //!< Use only linear interpolation between keys.
        eAnimTrackFlags_Loop     = BIT(2), //!< Play this track in a loop.
        eAnimTrackFlags_Cycle    = BIT(3), //!< Cycle track.
        eAnimTrackFlags_Disabled = BIT(4), //!< Disable this track.

        //////////////////////////////////////////////////////////////////////////
        // Used by editor.
        //////////////////////////////////////////////////////////////////////////
        eAnimTrackFlags_Hidden   = BIT(5), //!< Set when track is hidden in track view.
        eAnimTrackFlags_Muted    = BIT(8), //!< Mute this sound track or music track. This only affects the playback in editor on these types of tracks.
    };

    // <interfuscator:shuffle>
    virtual ~IAnimTrack() {};

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    //////////////////////////////////////////////////////////////////////////
    virtual EAnimCurveType GetCurveType() = 0;
    virtual AnimValueType     GetValueType() = 0;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
    // This color is used for the editor.
    virtual ColorB GetCustomColor() const = 0;
    virtual void SetCustomColor(ColorB color) = 0;
    virtual bool HasCustomColor() const = 0;
    virtual void ClearCustomColor() = 0;
#endif

    // Return what parameter of the node, this track is attached to.
    virtual const CAnimParamType& GetParameterType() const = 0;

    // Assign node parameter ID for this track.
    virtual void SetParameterType(CAnimParamType type) = 0;

    virtual void SetNode(IAnimNode* node) = 0;
    // Return Animation Sequence that owns this node.
    virtual IAnimNode* GetNode() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Animation track can contain sub-tracks (Position XYZ anim track have sub-tracks for x,y,z)
    // Get count of sub tracks.
    virtual int GetSubTrackCount() const = 0;
    // Retrieve pointer the specfied sub track.
    virtual IAnimTrack* GetSubTrack(int nIndex) const = 0;
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
    virtual int CopyKey(IAnimTrack* pFromTrack, int nFromKey) = 0;

    //! Get info about specified key.
    //! @param Short human readable text description of this key.
    //! @param duration of this key in seconds.
    virtual void GetKeyInfo(int key, const char*& description, float& duration) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Get track value at specified time.
    // Interpolates keys if needed.
    // Applies a scale multiplier set in SetMultiplier(), if requested
    //////////////////////////////////////////////////////////////////////////
    virtual void GetValue(float time, float& value, bool applyMultiplier=false) = 0;
    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9133)
     * use equivalent GetValue that accepts AZ::Vector3
     **/
    virtual void GetValue(float time, Vec3& value, bool applyMultiplier = false) = 0;
    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9133)
     * use equivalent GetValue that accepts AZ::Vector4
     **/
    virtual void GetValue(float time, Vec4& value, bool applyMultiplier = false) = 0;
        /**
     * O3DE_DEPRECATION_NOTICE(GHI-9133)
     * use equivalent GetValue that accepts AZ::Quaternion
     **/
    virtual void GetValue(float time, Quat& value) = 0;
    virtual void GetValue(float time, bool& value) = 0;
    virtual void GetValue(float time, Maestro::AssetBlends<AZ::Data::AssetData>& value) = 0;

    // support for AZ:: vector types - re-route to legacy types
    void GetValue(float time, AZ::Vector3& value, bool applyMultiplier = false)
    {
        Vec3 vec3;
        GetValue(time, vec3, applyMultiplier);
        value.Set(vec3.x, vec3.y, vec3.z);
    }
    void GetValue(float time, AZ::Vector4& value, bool applyMultiplier = false)
    {
        Vec4 vec4;
        GetValue(time, vec4, applyMultiplier);
        value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
    }
    void GetValue(float time, AZ::Quaternion& value)
    {
        Quat quat;
        GetValue(time, quat);
        value.Set(quat.v.x, quat.v.y, quat.v.z, quat.w);
    }

    //////////////////////////////////////////////////////////////////////////
    // Set track value at specified time.
    // Adds new keys if required.
    //////////////////////////////////////////////////////////////////////////
    virtual void SetValue(float time, const float& value, bool bDefault = false, bool applyMultiplier = false) = 0;
    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9133)
     * use equivalent SetValue that accepts AZ::Vector3
     **/
    virtual void SetValue(float time, const Vec3& value, bool bDefault = false, bool applyMultiplier = false) = 0;
    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9133)
     * use equivalent SetValue that accepts AZ::Vector4
     **/
    virtual void SetValue(float time, const Vec4& value, bool bDefault = false, bool applyMultiplier = false) = 0;
    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9133)
     * use equivalent SetValue that accepts AZ::Quaternion
     **/
    virtual void SetValue(float time, const Quat& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const bool& value, bool bDefault = false) = 0;
    virtual void SetValue(float time, const Maestro::AssetBlends<AZ::Data::AssetData>& value, bool bDefault = false) = 0;

    // support for AZ:: vector types - re-route to legacy types
    void SetValue(float time, AZ::Vector4& value, bool bDefault = false, bool applyMultiplier = false)
    {
        Vec4 vec4(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
        SetValue(time, vec4, bDefault, applyMultiplier);
    }
    void SetValue(float time, AZ::Vector3& value, bool bDefault = false, bool applyMultiplier = false)
    {
        Vec3 vec3(value.GetX(), value.GetY(), value.GetZ());
        SetValue(time, vec3, bDefault, applyMultiplier);
    }
    void SetValue(float time, AZ::Quaternion& value, bool bDefault = false)
    {
        Quat quat(value.GetW(), value.GetX(), value.GetY(), value.GetZ());
        SetValue(time, quat, bDefault);
    }

    // Only for position tracks, offset all track keys by this amount.
    virtual void OffsetKeyPosition(const Vec3& value) = 0;

    // Used to update the data in tracks after the parent entity has been changed.
    virtual void UpdateKeyDataAfterParentChanged(const AZ::Transform& oldParentWorldTM, const AZ::Transform& newParentWorldTM) = 0;

    // Assign active time range for this track.
    virtual void SetTimeRange(const Range& timeRange) = 0;

    //! @deprecated - IAnimTracks use AZ::Serialization now. Legacy - Serialize this animation track to XML.
    virtual bool Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks = true) = 0;
    virtual bool SerializeSelection(XmlNodeRef& xmlNode, bool bLoading, bool bCopySelected = false, float fTimeOffset = 0) = 0;

    virtual void InitPostLoad(IAnimSequence* /*sequence*/) {};

    //! For custom track animate parameters.
    virtual void Animate([[maybe_unused]] SAnimContext& ec) {};

    // Get access to the internal spline of the track.
    virtual ISplineInterpolator* GetSpline() const { return 0; };

    virtual bool IsKeySelected([[maybe_unused]] int key) const { return false; }

    virtual void SelectKey([[maybe_unused]] int key, [[maybe_unused]] bool select) {}

    virtual void SetSortMarkerKey([[maybe_unused]] unsigned int keyIndex, [[maybe_unused]] bool enabled) {}
    virtual bool IsSortMarkerKey([[maybe_unused]] unsigned int keyIndex) const { return false; }

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

    //! Returns whether the track responds to muting (false by default), which only affects the Edtior.
    //! Tracks that use mute should override this, such as CSoundTrack
    //! @return Boolean of whether the track respnnds to muting or not
    virtual bool UsesMute() const { return false; }

    //! Set a multiplier which will be multiplied to track values in SetValue and divided out in GetValue if requested
    virtual void SetMultiplier(float trackValueMultiplier) = 0;

    // Expanded state interface
    virtual void SetExpanded(bool expanded) = 0;
    virtual bool GetExpanded() const = 0;

    virtual unsigned int GetId() const = 0;
    virtual void SetId(unsigned int id) = 0;

    // </interfuscator:shuffle>
};


/** Callback called by animation node when its animated.
*/
struct IAnimNodeOwner
{
    // <interfuscator:shuffle>
    virtual ~IAnimNodeOwner(){}
    virtual void OnNodeAnimated([[maybe_unused]] IAnimNode* pNode) {}
    virtual void OnNodeVisibilityChanged(IAnimNode* pNode, const bool bHidden) = 0;
    virtual void OnNodeReset([[maybe_unused]] IAnimNode* pNode) {}

    // mark the node's sequence object layer as modified
    virtual void MarkAsModified() = 0;
    // </interfuscator:shuffle>
};

/** Base class for all Animation nodes,
        can host multiple animation tracks, and execute them other time.
        Animation node is reference counted.
*/
struct IAnimNode
{
public:
    AZ_RTTI(IAnimNode, "{0A096354-7F26-4B18-B8C0-8F10A3E0440A}");
    AZ_CLASS_ALLOCATOR(IAnimNode, AZ::SystemAllocator, 0);

    static void Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<IAnimNode>();
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Supported params.
    //////////////////////////////////////////////////////////////////////////
    enum ESupportedParamFlags
    {
        eSupportedParamFlags_MultipleTracks = 0x01, // Set if parameter can be assigned multiple tracks.
        eSupportedParamFlags_Hidden         = 0x02, // Hidden from the Track View UI.
    };

    struct SParamInfo
    {
        SParamInfo()
            : name("")
            , valueType(kAnimValueDefault)
            , flags(ESupportedParamFlags(0)) {};
        SParamInfo(const char* _name, CAnimParamType _paramType, AnimValueType _valueType, ESupportedParamFlags _flags)
            : name(_name)
            , paramType(_paramType)
            , valueType(_valueType)
            , flags(_flags) {};

        AZStd::string name;           // parameter name.
        CAnimParamType paramType;     // parameter id.
        AnimValueType valueType;       // value type, defines type of track to use for animating this parameter.
        ESupportedParamFlags flags; // combination of flags from ESupportedParamFlags.
    };

    using AnimParamInfos = AZStd::vector<SParamInfo>;

    // <interfuscator:shuffle>
    virtual ~IAnimNode() {};

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    //! Set node name.
    virtual void SetName(const char* name) = 0;

    //! Get node name.
    virtual const char* GetName() const = 0;

    // Get Type of this node.
    virtual AnimNodeType GetType() const = 0;

    // Return Animation Sequence that owns this node.
    virtual IAnimSequence* GetSequence() const = 0;

    // Set the Animation Sequence that owns this node.
    virtual void SetSequence(IAnimSequence* sequence) = 0;

    // Called when sequence is activated / deactivated
    virtual void Activate(bool bActivate) = 0;

    // Set AnimNode flags.
    // @param flags One or more flags from EAnimNodeFlags.
    // @see EAnimNodeFlags
    virtual void SetFlags(int flags) = 0;

    // Get AnimNode flags.
    // @return flags set on node.
    // @see EAnimNodeFlags
    virtual int GetFlags() const = 0;

    // return true if flagsToCheck is set on the node or any of the node's parents
    virtual bool AreFlagsSetOnNodeOrAnyParent(EAnimNodeFlags flagsToCheck) const = 0;

    // AZ::Entity is bound/handled via their Id over EBuses, as opposed to directly with pointers.
    virtual void SetAzEntityId(const AZ::EntityId& id) = 0;
    virtual AZ::EntityId GetAzEntityId() const = 0;

    // Return movie system that created this node.
    virtual IMovieSystem*   GetMovieSystem() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Space position/orientation scale.
    //////////////////////////////////////////////////////////////////////////
    //! Translate entity node.
    virtual void SetPos(float time, const Vec3& pos) = 0;
    //! Rotate entity node.
    virtual void SetRotate(float time, const Quat& quat) = 0;
    //! Scale entity node.
    virtual void SetScale(float time, const Vec3& scale) = 0;

    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * use equivalent SetPos that accepts AZ::Vector3
     **/
    void SetPos(float time, const AZ::Vector3& pos)
    {
        Vec3 vec3(pos.GetX(), pos.GetY(), pos.GetZ());
        SetPos(time, vec3);
    }
    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * use equivalent SetRotate that accepts AZ::Quaternion
     **/
    void SetRotate(float time, const AZ::Quaternion& rot)
    {
        Quat quat(rot.GetX(), rot.GetY(), rot.GetZ(), rot.GetW());
        SetRotate(time, quat);
    }
    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * use equivalent SetScale that accepts AZ::Vector3
     **/
    void SetScale(float time, const AZ::Vector3& scale)
    {
        Vec3 vec3(scale.GetX(), scale.GetY(), scale.GetZ());
        SetScale(time, vec3);
    }

    //! Compute and return the offset which brings the current position to the given position
    virtual Vec3 GetOffsetPosition(const Vec3& position) { return position - GetPos(); }

    //! Get current entity position.
    virtual Vec3 GetPos() = 0;
    //! Get current entity rotation.
    virtual Quat GetRotate() = 0;
    //! Get entity rotation at specified time.
    virtual Quat GetRotate(float time) = 0;
    //! Get current entity scale.
    virtual Vec3 GetScale() = 0;

    // General Set param.
    // Set float/vec3/vec4 parameter at given time.
    // @return true if parameter set, false if this parameter not exist in node.
    virtual bool SetParamValue(float time, CAnimParamType param, float value) = 0;
    virtual bool SetParamValue(float time, CAnimParamType param, const Vec3& value) = 0;
    virtual bool SetParamValue(float time, CAnimParamType param, const Vec4& value) = 0;

    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * use equivalent SetParamValue that accepts AZ::Vector3
     **/
    bool SetParamValue(float time, CAnimParamType param, const AZ::Vector3& value)
    {
        Vec3 vec3(value.GetX(), value.GetY(), value.GetZ());
        return SetParamValue(time, param, vec3);
    }

    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * use equivalent SetParamValue that accepts AZ::Vector4
     **/
    bool SetParamValue(float time, CAnimParamType param, const AZ::Vector4& value)
    {
        Vec4 vec4(value.GetX(), value.GetY(), value.GetZ(), value.GetW());
        return SetParamValue(time, param, vec4);
    }


    // Get float/vec3/vec4 parameter at given time.
    // @return true if parameter exist, false if this parameter not exist in node.
    virtual bool GetParamValue(float time, CAnimParamType param, float& value) = 0;
    virtual bool GetParamValue(float time, CAnimParamType param, Vec3& value) = 0;
    virtual bool GetParamValue(float time, CAnimParamType param, Vec4& value) = 0;

    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * use equivalent GetParamValue that accepts AZ::Vector4
     **/
    bool GetParamValue(float time, CAnimParamType param, AZ::Vector3& value)
    {
        Vec3 vec3;
        const bool result = GetParamValue(time, param, vec3);
        value.Set(vec3.x, vec3.y, vec3.z);
        return result;
    }

    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * use equivalent GetParamValue that accepts AZ::Vector4
     **/
    bool GetParamValue(float time, CAnimParamType param, AZ::Vector4& value)
    {
        Vec4 vec4;
        const bool result = GetParamValue(time, param, vec4);
        value.Set(vec4.x, vec4.y, vec4.z, vec4.w);
        return result;
    }

    //! Evaluate animation node while not playing animation.
    virtual void StillUpdate() = 0;

    //! Evaluate animation to the given time.
    virtual void Animate(SAnimContext& ec) = 0;

    // Description:
    //      Returns number of supported parameters by this animation node (position,rotation,scale,etc..).
    // Returns:
    //      Number of supported parameters.
    virtual unsigned int GetParamCount() const = 0;

    // Description:
    //      Returns the type of a param by index
    // Arguments:
    //      nIndex - parameter index in range 0 <= nIndex < GetSupportedParamCount()
    virtual CAnimParamType GetParamType(unsigned int nIndex) const = 0;

    // Description:
    //      Check if parameter is supported by this node.
    virtual bool IsParamValid(const CAnimParamType& paramType) const = 0;

    // Description:
    //      Returns name of supported parameter of this animation node or NULL if not available
    // Arguments:
    //          paramType - parameter id
    virtual AZStd::string GetParamName(const CAnimParamType& paramType) const = 0;

    // Description:
    //      Returns the params value type
    virtual AnimValueType GetParamValueType(const CAnimParamType& paramType) const = 0;

    // Description:
    //      Returns the params value type
    virtual ESupportedParamFlags GetParamFlags(const CAnimParamType& paramType) const = 0;

    // Called node data is re-initialized, such as when changing the entity associated with it.
    virtual void OnReset() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Working with Tracks.
    //////////////////////////////////////////////////////////////////////////
    virtual int GetTrackCount() const = 0;

    // Return track assigned to the specified parameter.
    virtual IAnimTrack* GetTrackByIndex(int nIndex) const = 0;

    // Return first track assigned to the specified parameter.
    virtual IAnimTrack* GetTrackForParameter(const CAnimParamType& paramType) const = 0;

    // Return the i-th track assigned to the specified parameter in case of multiple tracks.
    virtual IAnimTrack* GetTrackForParameter(const CAnimParamType& paramType, uint32 index) const = 0;

    // Get the index of a given track among tracks with the same parameter type in this node.
    virtual uint32 GetTrackParamIndex(const IAnimTrack* pTrack) const = 0;

    // Creates a new track for given parameter.
    virtual IAnimTrack* CreateTrack(const CAnimParamType& paramType) = 0;

    // Initializes track default values after de-serialization / user creation. Only called in editor.
    virtual void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType) = 0;

    // Assign animation track to parameter.
    // if track parameter is NULL track with parameter id param will be removed.
    virtual void SetTrack(const CAnimParamType& paramType, IAnimTrack* track) = 0;

    // Set time range for all tracks in this sequence.
    virtual void SetTimeRange(Range timeRange) = 0;

    // Remove track from anim node.
    virtual void AddTrack(IAnimTrack* pTrack) = 0;

    // Remove track from anim node.
    virtual bool RemoveTrack(IAnimTrack* pTrack) = 0;

    // Description:
    //      Creates default set of tracks supported by this node.
    virtual void CreateDefaultTracks() = 0;

    // returns the tangent type to use for created keys. Override this if you have an animNode that you wish
    // to have tangents other than UNIFIED created for new keys.
    virtual int GetDefaultKeyTangentFlags() const { return SPLINE_KEY_TANGENT_UNIFIED; }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Callback for animation node used by editor.
    // Register notification callback with animation node.
    virtual void SetNodeOwner(IAnimNodeOwner* pOwner) = 0;
    virtual IAnimNodeOwner* GetNodeOwner() = 0;

    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
     **/
    virtual void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) = 0;

    /**
     * O3DE_DEPRECATION_NOTICE(GHI-9326)
     * Serialization for Sequence data in Component Entity Sequences now occurs through AZ::SerializeContext and the Sequence Component
     **/
    virtual void SerializeAnims(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) = 0;

    // Sets up internal pointers post load from Sequence Component
    virtual void InitPostLoad(IAnimSequence* sequence) = 0;

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Groups interface
    //////////////////////////////////////////////////////////////////////////
    virtual void SetParent(IAnimNode* pParent) = 0;
    virtual IAnimNode* GetParent() const = 0;
    virtual IAnimNode* HasDirectorAsParent() const = 0;
    //////////////////////////////////////////////////////////////////////////

    virtual void Render() = 0;

    virtual bool NeedToRender() const = 0;

    // Called from editor if dynamic params need updating
    virtual void UpdateDynamicParams() = 0;
    // </interfuscator:shuffle>

    // Used by AnimCameraNode
    virtual bool GetShakeRotation([[maybe_unused]] const float& time, [[maybe_unused]] Quat& rot){return false; }

    virtual void SetCameraShakeSeed([[maybe_unused]] const uint shakeSeed){};

    // override this method to handle explicit setting of time
    virtual void TimeChanged([[maybe_unused]] float newTime) {};

    // Compares all of the node's track values at the given time with the associated property value and
    //     sets a key at that time if they are different to match the latter
    // Returns the number of keys set
    virtual int SetKeysForChangedTrackValues([[maybe_unused]] float time) { return 0; };

    // Callbacks used when Game/Simulation mode is started and stopped in the Editor. Override if you want to handle these events
    virtual void OnStartPlayInEditor() {};
    virtual void OnStopPlayInEditor() {};

    ////////////////////////////////////////////////////////////////////////////////
    // interface for Components - implemented by CAnimComponentNode

    // Override if the derived node has an associated component type (e.g. CAnimComponentNode)
    virtual void SetComponent([[maybe_unused]] AZ::ComponentId comopnentId, [[maybe_unused]] const AZ::Uuid& componentTypeId) {}

    // returns the componentId of the component the node is associate with, if applicable, or a AZ::InvalidComponentId otherwise
    virtual AZ::ComponentId GetComponentId() const { return AZ::InvalidComponentId; }
    // ~interface for Components
    ////////////////////////////////////////////////////////////////////////////////

    // Used to disable any animation that is overridden by a SceneNode during camera interpolation, such as
    // FoV, transform, nearZ
    virtual void SetSkipInterpolatedCameraNode([[maybe_unused]] const bool skipNodeCameraAnimation) {};

    // Expanded state interface
    virtual void SetExpanded(bool expanded) = 0;
    virtual bool GetExpanded() const = 0;

    // Return the node id. This id is unique within a given sequence.
    virtual int GetId() const = 0;
};

//! Track event listener
struct ITrackEventListener
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
    virtual ~ITrackEventListener(){}
    // Description:
    //      Called when track event is updated
    // Arguments:
    //      pSeq - Animation sequence
    //      reason - Reason for update (see EReason)
    //      event - Track event added
    //      pUserData - Data to accompany reason
    virtual void OnTrackEvent(IAnimSequence* sequence, int reason, const char* event, void* pUserData) = 0;
    // </interfuscator:shuffle>
};

struct IAnimLegacySequenceObject
{
    // <interfuscator:shuffle>
    virtual ~IAnimLegacySequenceObject() {}
    virtual void OnNameChanged() = 0;
    virtual void OnModified() = 0;
    // </interfuscator:shuffle>
};

struct IAnimStringTable
{
    AZ_RTTI(IAnimStringTable, "{35690309-9D22-41FF-80B7-8AF7C8419945}")
    virtual ~IAnimStringTable() {}

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    virtual const char* Add(const char* p) = 0;
};

/** Animation sequence, operates on animation nodes contained in it.
 */
struct IAnimSequence
{
    AZ_RTTI(IAnimSequence, "{A60F95F5-5A4A-47DB-B3BB-525BBC0BC8DB}");
    AZ_CLASS_ALLOCATOR(IAnimSequence, AZ::SystemAllocator, 0);

    static const int kSequenceVersion = 5;

    static void Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<IAnimSequence>();
        }
    }

    //! Flags used for SetFlags(),GetFlags(),SetParentFlags(),GetParentFlags() methods.
    enum EAnimSequenceFlags
    {
        eSeqFlags_PlayOnReset         = BIT(0),   //!< Start playing this sequence immediately after reset of movie system(Level load).
        eSeqFlags_OutOfRangeConstant  = BIT(1),   //!< Constant Out-Of-Range,time continues normally past sequence time range.
        eSeqFlags_OutOfRangeLoop      = BIT(2),   //!< Loop Out-Of-Range,time wraps back to the start of range when reaching end of range.
        eSeqFlags_CutScene            = BIT(3),   //!< Cut scene sequence.
        eSeqFlags_NoHUD               = BIT(4),   //!< @deprecated - Don`t display HUD
        eSeqFlags_NoPlayer            = BIT(5),   //!< Disable input and drawing of player
        eSeqFlags_NoGameSounds        = BIT(9),   //!< Suppress all game sounds.
        eSeqFlags_NoSeek              = BIT(10), //!< Cannot seek in sequence.
        eSeqFlags_NoAbort             = BIT(11), //!< Cutscene can not be aborted
        eSeqFlags_NoSpeed             = BIT(13), //!< Cannot modify sequence speed - TODO: add interface control if required
     // eSeqFlags_CanWarpInFixedTime  = BIT(14), //!< @deprecated - Timewarp by scaling a fixed time step - removed July 2017, unused
        eSeqFlags_EarlyMovieUpdate    = BIT(15), //!< Turn the 'sys_earlyMovieUpdate' on during the sequence.
        eSeqFlags_LightAnimationSet   = BIT(16), //!< A special unique sequence for light animations
        eSeqFlags_NoMPSyncingNeeded   = BIT(17), //!< this sequence doesn't require MP net syncing
    };

    virtual ~IAnimSequence() {};

    // for intrusive_ptr support
    virtual void add_ref() = 0;
    virtual void release() = 0;

    //! Set the name of this sequence. (ex. "Intro" in the same case as above)
    virtual void SetName(const char* name) = 0;
    //! Get the name of this sequence. (ex. "Intro" in the same case as above)
    virtual const char* GetName() const = 0;
    //! Get the ID (unique in a level and consistent across renaming) of this sequence.
    virtual uint32 GetId () const = 0;
    //! Resets the ID to the next available ID - used on sequence loads into levels to resolve ID collisions
    virtual void ResetId() = 0;

    // Legacy sequence objects are connected by pointers. SequenceComponents are connected by AZ::EntityId
    virtual void SetLegacySequenceObject(IAnimLegacySequenceObject* legacySequenceObject) = 0;
    virtual IAnimLegacySequenceObject* GetLegacySequenceObject() const = 0;
    virtual void SetSequenceEntityId(const AZ::EntityId& entityOwnerId) = 0;
    virtual const AZ::EntityId& GetSequenceEntityId() const = 0;

    //! Set the currently active director node.
    virtual void SetActiveDirector(IAnimNode* pDirectorNode) = 0;
    //! Get the currently active director node, if any.
    virtual IAnimNode* GetActiveDirector() const = 0;

    //! Set animation sequence flags
    virtual void SetFlags(int flags) = 0;
    //! Get animation sequence flags
    virtual int GetFlags() const = 0;
    //! Get cutscene related animation sequence flags
    virtual int GetCutSceneFlags(const bool localFlags = false) const = 0;
    //! Set parent animation sequence
    virtual void SetParentSequence(IAnimSequence* pParentSequence) = 0;
    //! Get parent animation sequence
    virtual const IAnimSequence* GetParentSequence() const = 0;
    //! Check whether this sequence has the given sequence as a descendant through one of its sequence tracks.
    virtual bool IsAncestorOf(const IAnimSequence* sequence) const = 0;

    //! Return number of animation nodes in sequence.
    virtual int GetNodeCount() const = 0;
    //! Get animation node at specified index.
    virtual IAnimNode* GetNode(int index) const = 0;

    //! Add animation node to sequence.
    //! @return True if node added, same node will not be added 2 times.
    virtual bool AddNode(IAnimNode* node) = 0;

    // Reorders the array of nodes, so the specified node is placed after or before the given pivot node depending on the parameter 'next'.
    virtual void ReorderNode(IAnimNode* node, IAnimNode* pPivotNode, bool next) = 0;

    // Description:
    //      Creates a new animation node with specified type.
    // Arguments:
    //      nodeType - Type of node, one of AnimNodeType enums.
    virtual IAnimNode* CreateNode(AnimNodeType nodeType) = 0;

    // Description:
    //      Creates a new animation node from serialized node XML.
    // Arguments:
    //      node - Serialized form of node
    virtual IAnimNode* CreateNode(XmlNodeRef node) = 0;

    //! Remove animation node from sequence.
    virtual void RemoveNode(IAnimNode* node, bool removeChildRelationships /*=true*/) = 0;

    // Finds node by name, can be slow.
    // If the node belongs to a director, the director node also should be given
    // since there can be multiple instances of the same node(i.e. the same name)
    // across multiple director nodes.
    virtual IAnimNode* FindNodeByName(const char* sNodeName, const IAnimNode* pParentDirector) = 0;

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
    virtual void Animate(const SAnimContext& ec) = 0;

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

    /** Called when time was explicitly jumped to/set.
    */
    virtual void TimeChanged(float newTime) = 0;

    // fix up internal pointers after load from Sequence Component
    virtual void InitPostLoad() = 0;

    // Copy some nodes of this sequence to XML.
    virtual void CopyNodes(XmlNodeRef& xmlNode, IAnimNode** pSelectedNodes, uint32 count) = 0;

    // Paste nodes given by the XML to this sequence.
    virtual void PasteNodes(const XmlNodeRef& xmlNode, IAnimNode* pParent) = 0;

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

    virtual IAnimStringTable* GetTrackEventStringTable() = 0;

    // Summary:
    //   Called to trigger a track event.
    virtual void TriggerTrackEvent(const char* event, const char* param = NULL) = 0;

    //! Track event listener
    virtual void AddTrackEventListener(ITrackEventListener* pListener) = 0;
    virtual void RemoveTrackEventListener(ITrackEventListener* pListener) = 0;

    // return the sequence type - legacy or new component entity
    virtual SequenceType GetSequenceType() const = 0;

    // Expanded state interface
    virtual void SetExpanded(bool expanded) = 0;
    virtual bool GetExpanded() const = 0;

    virtual unsigned int GetUniqueTrackIdAndGenerateNext() = 0;

    // </interfuscator:shuffle>
};

/** Movie Listener interface.
    Register at movie system to get notified about movie events
*/
struct IMovieListener
{
    enum EMovieEvent
    {
        eMovieEvent_Started = 0,   // fired when sequence is started
        eMovieEvent_Stopped,       // fired when sequence ended normally
        eMovieEvent_Aborted,       // fired when sequence was aborted before normal end (STOP and ABORTED event are mutually exclusive!)
        eMovieEvent_Updated,            // fired after sequence time or playback speed was updated
        eMovieEvent_RecordModeStarted,  // fired when Record Mode is started
        eMovieEvent_RecordModeStopped,  // fired when Record Mode is stopped
    };

    // <interfuscator:shuffle>
    virtual ~IMovieListener(){}
    //! callback on movie events
    virtual void OnMovieEvent(EMovieEvent movieEvent, IAnimSequence* pAnimSequence) = 0;
    // </interfuscator:shuffle>
};

/** Movie System interface.
        Main entrance point to engine movie capability.
        Enumerate available movies, update all movies, create animation nodes and tracks.
 */
struct IMovieSystem
{
    AZ_RTTI(IMovieSystem, "{D8E6D6E9-830D-40DC-87F3-E9A069FBEB69}")

    enum ESequenceStopBehavior
    {
        eSSB_LeaveTime = 0,         // When sequence is stopped it remains in last played time.
        eSSB_GotoEndTime = 1,       // Default behavior in game, sequence is animated at end time before stop.
        eSSB_GotoStartTime = 2, // Default behavior in editor, sequence is animated at start time before stop.
    };

    // <interfuscator:shuffle>
    virtual ~IMovieSystem(){}

    //! Release movie system.
    virtual void Release() = 0;
    //! Set the user.
    virtual void SetUser(IMovieUser* pUser) = 0;
    //! Get the user.
    virtual IMovieUser* GetUser() = 0;

    virtual IAnimSequence* CreateSequence(const char* sequence, bool bLoad = false, uint32 id = 0, SequenceType = kSequenceTypeDefault, AZ::EntityId entityId = AZ::EntityId()) = 0;
    virtual void AddSequence(IAnimSequence* sequence) = 0;
    virtual void RemoveSequence(IAnimSequence* sequence) = 0;
    virtual IAnimSequence* FindLegacySequenceByName(const char* sequence) const = 0;
    virtual IAnimSequence* FindSequence(const AZ::EntityId& componentEntitySequenceId) const = 0;
    virtual IAnimSequence* FindSequenceById(uint32 id) const = 0;
    virtual IAnimSequence* GetSequence(int i) const = 0;
    virtual int GetNumSequences() const = 0;
    virtual IAnimSequence* GetPlayingSequence(int i) const = 0;
    virtual int GetNumPlayingSequences() const = 0;
    virtual bool IsCutScenePlaying() const = 0;

    virtual uint32 GrabNextSequenceId() = 0;
    // called whenever a new sequence Id is set - to update nextSequenceId
    virtual void OnSetSequenceId(uint32 sequenceId) = 0;
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
    // @param sequence Pointer to sequence
    // @param pListener Pointer to an IMovieListener
    // @return true on successful add, false otherwise
    virtual bool AddMovieListener(IAnimSequence* sequence, IMovieListener* pListener) = 0;

    // Removes a listener from a sequence
    // @param sequence Pointer to sequence
    // @param pListener Pointer to an IMovieListener
    // @return true on successful removal, false otherwise
    virtual bool RemoveMovieListener(IAnimSequence* sequence, IMovieListener* pListener) = 0;

    virtual ISystem* GetSystem() = 0;

    // Remove all sequences from movie system.
    virtual void RemoveAllSequences() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Sequence playback.
    //////////////////////////////////////////////////////////////////////////
    // Start playing sequence.
    // Call ignored if sequence is already playing.
    // @param sequence Name of sequence to play.
    virtual void PlaySequence(const char* pSequenceName, IAnimSequence* pParentSeq, bool bResetFX, bool bTrackedSequence, float startTime = -FLT_MAX, float endTime = -FLT_MAX) = 0;

    // Start playing sequence.
    // Call ignored if sequence is already playing.
    // @param sequence Pointer to Valid sequence to play.
    virtual void PlaySequence(IAnimSequence* sequence, IAnimSequence* pParentSeq, bool bResetFX, bool bTrackedSequence, float startTime = -FLT_MAX, float endTime = -FLT_MAX) = 0;

    // Stops currently playing sequence.
    // Ignored if sequence is not playing.
    // Returns true if sequence has been stopped. false otherwise
    // @param sequence Name of playing sequence to stop.
    virtual bool StopSequence(const char* pSequenceName) = 0;

    // Stop's currently playing sequence.
    // Ignored if sequence is not playing.
    // Returns true if sequence has been stopped. false otherwise
    // @param sequence Pointer to Valid sequence to stop.
    virtual bool StopSequence(IAnimSequence* sequence) = 0;

    /** Aborts a currently playing sequence.
            Ignored if sequence is not playing.
            Calls IMovieListener with MOVIE_EVENT_ABORTED event (MOVIE_EVENT_DONE is NOT called)
            Returns true if sequence has been aborted. false otherwise
            @param sequence Pointer to Valid sequence to stop.
            @param bLeaveTime If false, uses default stop behavior, otherwise leaves the sequence at time
    */
    virtual bool AbortSequence(IAnimSequence* sequence, bool bLeaveTime = false) = 0;

    // Stops all currently playing sequences.
    virtual void StopAllSequences() = 0;

    // Stops all playing cut-scene sequences.
    // This will not stop all sequences, but only those with CUT_SCENE flag set.
    virtual void StopAllCutScenes() = 0;

    // Checks if specified sequence is playing.
    virtual bool IsPlaying(IAnimSequence* seq) const = 0;

    /** Resets playback state of movie system,
            usually called after loading of level,
    */
    virtual void Reset(bool bPlayOnReset, bool bSeekToStart) = 0;

    // Sequences with PLAY_ONRESET flag will start playing after this call.
    virtual void PlayOnLoadSequences() = 0;

    // Update movie system while not playing animation.
    virtual void StillUpdate() = 0;

    // Updates movie system every frame before the entity system to animate all playing sequences.
    virtual void PreUpdate(const float dt) = 0;

    // Updates movie system every frame after the entity system to animate all playing sequences.
    virtual void PostUpdate(const float dt) = 0;

    // Render function call of some special node.
    virtual void Render() = 0;

    // Set and enable Fixed Step cvars
    virtual void EnableFixedStepForCapture(float step) = 0;

    // Disable Fixed Step cvars and return to previous settings
    virtual void DisableFixedStepForCapture() = 0;

    // Signal the capturing start.
    virtual void StartCapture(const ICaptureKey& key, int frame) = 0;

    // Signal the capturing end.
    virtual void EndCapture() = 0;

    // Actually turn on/off the capturing.
    // This is needed for the timing issue of turning on/off the capturing.
    virtual void ControlCapture() = 0;

    // Returns true if a Render Output capture is currently active.
    virtual bool IsCapturing() const = 0;

    // Set movie system into recording mode,
    // While in recording mode any changes made to node will be added as keys to tracks.
    virtual void SetRecording(bool recording) = 0;
    virtual bool IsRecording() const = 0;

    virtual void EnableCameraShake(bool bEnabled) = 0;

    // Pause any playing sequences.
    virtual void Pause() = 0;

    // Resume playing sequences.
    virtual void Resume() = 0;

    // Pause cut scenes in editor.
    virtual void PauseCutScenes() = 0;

    // Resume cut scenes in editor.
    virtual void ResumeCutScenes() = 0;

    // Callback when animation-data changes
    virtual void SetCallback(IMovieCallback* pCallback) = 0;

    virtual IMovieCallback* GetCallback() = 0;

    virtual const SCameraParams& GetCameraParams() const = 0;
    virtual void SetCameraParams(const SCameraParams& Params) = 0;
    virtual void SendGlobalEvent(const char* pszEvent) = 0;

    // Gets the float time value for a sequence that is already playing
    virtual float GetPlayingTime(IAnimSequence* pSeq) = 0;
    virtual float GetPlayingSpeed(IAnimSequence* pSeq) = 0;
    // Sets the time progression of an already playing cutscene.
    // If IAnimSequence:NO_SEEK flag is set on pSeq, this call is ignored.
    virtual bool SetPlayingTime(IAnimSequence* pSeq, float fTime) = 0;
    virtual bool SetPlayingSpeed(IAnimSequence* pSeq, float fSpeed) = 0;
    // Set behavior pattern for stopping sequences.
    virtual void SetSequenceStopBehavior(ESequenceStopBehavior behavior) = 0;

    // Set the start and end time of an already playing cutscene.
    virtual bool GetStartEndTime(IAnimSequence* pSeq, float& fStartTime, float& fEndTime) = 0;
    virtual bool SetStartEndTime(IAnimSequence* pSeq, const float fStartTime, const float fEndTime) = 0;

    // Make the specified sequence go to a given frame time.
    // @param seqName A sequence name.
    // @param targetFrame A target frame to go to in time.
    virtual void GoToFrame(const char* seqName, float targetFrame) = 0;

    // Get the name of camera used for sequences instead of cameras specified in the director node.
    virtual const char* GetOverrideCamName() const = 0;

    // Get behavior pattern for stopping sequences.
    virtual IMovieSystem::ESequenceStopBehavior GetSequenceStopBehavior() = 0;

    // These are used to disable 'Ragdollize' events in the editor when the 'AI/Physics' is off.
    virtual bool IsPhysicsEventsEnabled() const = 0;
    virtual void EnablePhysicsEvents(bool enable) = 0;

    virtual void EnableBatchRenderMode(bool bOn) = 0;
    virtual bool IsInBatchRenderMode() const = 0;

    virtual void LoadParamTypeFromXml(CAnimParamType& animParamType, const XmlNodeRef& xmlNode, const uint version) = 0;
    virtual void SaveParamTypeToXml(const CAnimParamType& animParamType, XmlNodeRef& xmlNode) = 0;

    // Should only be called from CAnimParamType
    virtual void SerializeParamType(CAnimParamType& animParamType, XmlNodeRef& xmlNode, bool bLoading, const uint version) = 0;

    // For buffering and presenting user notification messages in the Editor. Will also print as an AZ_Warning()
    virtual void LogUserNotificationMsg(const AZStd::string& msg) = 0;
    virtual void ClearUserNotificationMsgs() = 0;
    virtual const AZStd::string& GetUserNotificationMsgs() const = 0;

    // Call this from OnActivate() when a new sequence component entity is activated.
    virtual void OnSequenceActivated(IAnimSequence* sequence) = 0;

#ifdef MOVIESYSTEM_SUPPORT_EDITING
    virtual AnimNodeType GetNodeTypeFromString(const char* pString) const = 0;
    virtual CAnimParamType GetParamTypeFromString(const char* pString) const = 0;
#endif

    // fill in the animNodeType from the xmlNode description (or vice versa)
    virtual void SerializeNodeType(AnimNodeType& animNodeType, XmlNodeRef& xmlNode, bool bLoading, const uint version, int flags) = 0;

    // </interfuscator:shuffle>
};

inline void CAnimParamType::SaveToXml(XmlNodeRef& xmlNode) const
{
    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->SaveParamTypeToXml(*this, xmlNode);
    }
}

inline void CAnimParamType::LoadFromXml(const XmlNodeRef& xmlNode, const uint version)
{
    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->LoadParamTypeFromXml(*this, xmlNode, version);
    }
}

inline void CAnimParamType::Serialize(XmlNodeRef& xmlNode, bool bLoading, const uint version)
{
    if (gEnv->pMovieSystem)
    {
        gEnv->pMovieSystem->SerializeParamType(*this, xmlNode, bLoading, version);
    }
}
