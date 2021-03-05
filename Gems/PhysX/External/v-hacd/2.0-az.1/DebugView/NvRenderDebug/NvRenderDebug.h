#ifndef NV_RENDER_DEBUG_H
#define NV_RENDER_DEBUG_H

#ifdef _MSC_VER
#ifndef _INTPTR
#define _INTPTR 0
#endif
#endif

#include <stdint.h>

/*!
\file
\brief debug rendering classes and structures
*/

/**
\brief This defines the version number of the API.  If the API changes in anyway, this version number needs to be bumped.
*/
#define RENDER_DEBUG_VERSION 1010
/**
\brief This defines the version number for the communications layer.  If the format or layout of any packets change in a way that will not be backwards compatible, this needs to be bumped
*/
#define RENDER_DEBUG_COMM_VERSION 1010
/**
\brief The default port number for RenderDebug client/server connections.  You can change this if you wish, but you must make sure both your client and server code uses the new port number.
*/
#define RENDER_DEBUG_PORT 5525

namespace nvidia
{
    class NvAllocatorCallback;
    class NvErrorCallback;
}

namespace RENDER_DEBUG
{

/**
\brief Optional interface which provides typed methods for various routines rather than simply using const float pointers
*/
class RenderDebugTyped;

/**
\brief Enums for debug colors
 */
struct DebugColors
{
    /**
    \brief An enumerated list of default color values
    */
    enum Enum
    {
        Default = 0,
        PoseArrows,
        MeshStatic,
        MeshDynamic,
        Shape,
        Text0,
        Text1,
        ForceArrowsLow,
        ForceArrowsNorm,
        ForceArrowsHigh,
        Color0,
        Color1,
        Color2,
        Color3,
        Color4,
        Color5,
        Red,
        Green,
        Blue,
        DarkRed,
        DarkGreen,
        DarkBlue,
        LightRed,
        LightGreen,
        LightBlue,
        Purple,
        DarkPurple,
        Yellow,
        Orange,
        Gold,
        Emerald,
        White,
        Black,
        Gray,
        LightGray,
        DarkGray,
        NUM_COLORS
    };
};

/**
\brief Enums for pre-defined tiled textures
 */
struct DebugTextures
{
    /**
    \brief An enumerated list of default tiled textures
    */
    enum Enum
    {
        TNULL,
        IDETAIL01,
        IDETAIL02,
        IDETAIL03,
        IDETAIL04,
        IDETAIL05,
        IDETAIL06,
        IDETAIL07,
        IDETAIL08,
        IDETAIL09,
        IDETAIL10,
        IDETAIL11,
        IDETAIL12,
        IDETAIL13,
        IDETAIL14,
        IDETAIL15,
        IDETAIL16,
        IDETAIL17,
        IDETAIL18,
        WHITE,
        BLUE_GRAY,
        BROWN,
        DARK_RED,
        GOLD,
        GRAY,
        GREEN,
        INDIGO,
        LAVENDER,
        LIGHT_TORQUISE,
        LIGHT_YELLOW,
        LIME,
        ORANGE,
        PURPLE,
        RED,
        ROSE,
        TORQUISE,
        YELLOW,
        WOOD1,
        WOOD2,
        SPHERE1,
        SPHERE2,
        SPHERE3,
        SPHERE4,
        NUM_TEXTURES
    };
};

/**
\brief Predefined InputEventIds; custom ids must be greater than NUM_SAMPLE_FRAMWORK_EVENT_IDS
 */
struct InputEventIds
{
    enum Enum
    {
        // InputEvents used by SampleApplication
        CAMERA_SHIFT_SPEED = 0,
        CAMERA_MOVE_LEFT ,
        CAMERA_MOVE_RIGHT ,
        CAMERA_MOVE_UP ,
        CAMERA_MOVE_DOWN ,
        CAMERA_MOVE_FORWARD ,
        CAMERA_MOVE_BACKWARD ,
        CAMERA_SPEED_INCREASE ,
        CAMERA_SPEED_DECREASE ,

        CAMERA_MOUSE_LOOK ,
        CAMERA_MOVE_BUTTON ,

        CAMERA_GAMEPAD_ROTATE_LEFT_RIGHT ,
        CAMERA_GAMEPAD_ROTATE_UP_DOWN ,
        CAMERA_GAMEPAD_MOVE_LEFT_RIGHT ,
        CAMERA_GAMEPAD_MOVE_FORWARD_BACK ,

        CAMERA_JUMP ,
        CAMERA_CROUCH ,
        CAMERA_CONTROLLER_INCREASE ,
        CAMERA_CONTROLLER_DECREASE ,

        CAMERA_HOME,
        NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS
    };
};

/**
\brief Predefined InputIds that you can designate
 */
struct InputIds
{
    enum Enum
    {
        WKEY_UNKNOWN = 0,

        WKEY_DEFINITION_START ,

        WKEY_A ,
        WKEY_B ,
        WKEY_C ,
        WKEY_D ,
        WKEY_E ,
        WKEY_F ,
        WKEY_G ,
        WKEY_H ,
        WKEY_I ,
        WKEY_J ,
        WKEY_K ,
        WKEY_L ,
        WKEY_M ,
        WKEY_N ,
        WKEY_O ,
        WKEY_P ,
        WKEY_Q ,
        WKEY_R ,
        WKEY_S ,
        WKEY_T ,
        WKEY_U ,
        WKEY_V ,
        WKEY_W ,
        WKEY_X ,
        WKEY_Y ,
        WKEY_Z ,

        WKEY_0 ,
        WKEY_1 ,
        WKEY_2 ,
        WKEY_3 ,
        WKEY_4 ,
        WKEY_5 ,
        WKEY_6 ,
        WKEY_7 ,
        WKEY_8 ,
        WKEY_9 ,

        WKEY_SPACE ,
        WKEY_RETURN ,
        WKEY_SHIFT ,
        WKEY_CONTROL ,
        WKEY_ESCAPE ,
        WKEY_COMMA ,
        WKEY_NUMPAD0 ,
        WKEY_NUMPAD1 ,
        WKEY_NUMPAD2 ,
        WKEY_NUMPAD3 ,
        WKEY_NUMPAD4 ,
        WKEY_NUMPAD5 ,
        WKEY_NUMPAD6 ,
        WKEY_NUMPAD7 ,
        WKEY_NUMPAD8 ,
        WKEY_NUMPAD9 ,
        WKEY_MULTIPLY ,
        WKEY_ADD ,
        WKEY_SEPARATOR ,
        WKEY_SUBTRACT ,
        WKEY_DECIMAL ,
        WKEY_DIVIDE ,

        WKEY_F1 ,
        WKEY_F2 ,
        WKEY_F3 ,
        WKEY_F4 ,
        WKEY_F5 ,
        WKEY_F6 ,
        WKEY_F7 ,
        WKEY_F8 ,
        WKEY_F9 ,
        WKEY_F10 ,
        WKEY_F11 ,
        WKEY_F12 ,

        WKEY_TAB ,
        WKEY_BACKSPACE ,
        WKEY_PRIOR ,
        WKEY_NEXT ,
        WKEY_UP ,
        WKEY_DOWN ,
        WKEY_LEFT ,
        WKEY_RIGHT ,
        WKEY_HOME,


        SCAN_CODE_UP ,
        SCAN_CODE_DOWN ,
        SCAN_CODE_LEFT ,
        SCAN_CODE_RIGHT ,
        SCAN_CODE_FORWARD ,
        SCAN_CODE_BACKWARD ,
        SCAN_CODE_LEFT_SHIFT ,
        SCAN_CODE_SPACE ,
        SCAN_CODE_L ,
        SCAN_CODE_9 ,
        SCAN_CODE_0 ,

        WKEY_DEFINITION_END ,

        MOUSE_DEFINITION_START ,

        MOUSE_BUTTON_LEFT ,
        MOUSE_BUTTON_RIGHT ,
        MOUSE_BUTTON_CENTER ,

        MOUSE_MOVE ,

        MOUSE_DEFINITION_END ,

        GAMEPAD_DEFINITION_START ,

        GAMEPAD_DIGI_UP ,
        GAMEPAD_DIGI_DOWN ,
        GAMEPAD_DIGI_LEFT ,
        GAMEPAD_DIGI_RIGHT ,
        GAMEPAD_START ,
        GAMEPAD_SELECT ,
        GAMEPAD_LEFT_STICK ,
        GAMEPAD_RIGHT_STICK ,
        GAMEPAD_NORTH ,
        GAMEPAD_SOUTH ,
        GAMEPAD_WEST ,
        GAMEPAD_EAST ,
        GAMEPAD_LEFT_SHOULDER_TOP ,
        GAMEPAD_RIGHT_SHOULDER_TOP ,
        GAMEPAD_LEFT_SHOULDER_BOT ,
        GAMEPAD_RIGHT_SHOULDER_BOT ,

        GAMEPAD_RIGHT_STICK_X ,
        GAMEPAD_RIGHT_STICK_Y ,
        GAMEPAD_LEFT_STICK_X ,
        GAMEPAD_LEFT_STICK_Y ,

        GAMEPAD_DEFINITION_END ,

        NUM_KEY_CODES

    };
};

/**
\brief State flags for debug renderable
 */
struct DebugRenderState
{
    /**
    \brief The enumerated list of debug render state bit flags.
    */
    enum Enum
    {
        ScreenSpace        = (1<<0),  //!< true if rendering in screenspace
        NoZbuffer        = (1<<1),  //!< true if zbuffering is disabled.
        SolidShaded        = (1<<2),  //!< true if rendering solid shaded.
        SolidWireShaded    = (1<<3),  //!< Render both as a solid shaded triangle and as a wireframe overlay.
        CounterClockwise= (1<<4),  //!< true if winding order is counter clockwise.
        CameraFacing    = (1<<5),  //!< True if text should be displayed camera facing
        InfiniteLifeSpan = (1<<6),  //!< True if the lifespan is infinite (overrides current display time value)
        CenterText        = (1<<7),  //!< True if the text should be centered.
        DoubleSided        = (1<<8)    //! If true, then triangles should be rendered double sided, back-side uses secondary color
    };
};

/**
\brief Enums for pre-defined render modes for draw axes
*/
struct DebugAxesRenderMode
{
    /**
    \brief An enumerated list of render modes for draw axes
    */
    enum Enum
    {
        DEBUG_AXES_RENDER_SOLID,  //!< Render debug axes with huge solid conus as arrows and cylinders as axes
        DEBUG_AXES_RENDER_LINES,  //!< Render debug axes with plane triangles as arrows and lines as axes
        NUM_DEBUG_AXES_RENDER_MODES
    };
};
/**
\brief This defines a class which represents a single 3x3 rotation matrix and a float 3 offset position for rendering instanced meshes.

You most follow this exact layout where the first 3 floats represent the position of the instance and the remaining 9 represent
the 3x3 rotation matrix for a total of 12 floats.

The default constructor initializes the instance to identity
*/
class RenderDebugInstance
{
public:
    /**
    \brief The default constructor for RenderDebugInstance will initialize the class to an identity transform
    */
    RenderDebugInstance(void)
    {
        mTransform[0] = 0;    // The X position
        mTransform[1] = 0;    // The Y position
        mTransform[2] = 0;    // The Z position

        mTransform[3] = 1.0f;    // ColumnX of the 3x3
        mTransform[4] = 0;
        mTransform[5] = 0;

        mTransform[6] = 0; // ColumnY of the 3x3
        mTransform[7] = 1.0f;
        mTransform[8] = 0;

        mTransform[9] = 0; // Column Z of the 3x3
        mTransform[10] = 0;
        mTransform[11] = 1.0f;

    }
    /**
    \brief a 3x4 matrix (translation + scale/rotation)
    */
    float    mTransform[12];
};

/**
\brief This class defines an extremely simply mesh vertex.

Since the RenderDebug library is limited in scope, the basic mesh vertex is simply a
position, normal, and a single texture co-ordinate.  You cannot choose a texture at
this time.  Currently each mesh is drawn with a default generic noise texture which
is simply lit.
*/
class RenderDebugMeshVertex
{
public:
    /**
    \brief  The world-space position
    */
    float mPosition[3];
    /**
    \brief The normal vector to use for lighting
    */
    float mNormal[3];
    /**
    \brief  Texture co-ordinates
    */
    float mTexel[2];
};

/**
\brief This class defines a simply solid shaded vertex without any texture source.  This is used for solid shaded debug visualization.
 */
class RenderDebugSolidVertex
{
public:
    /**
    \brief  The world-space position
    */
    float mPos[3];

    /**
    \brief  The diffuse color as 32 bit ARGB
    */
    uint32_t mColor;

    /**
    \brief  The normal vector to use for lighting
    */
    float mNormal[3];
};

/**
\brief A simple unlit vertex with color but no normal.

These vertices are used for line drawing.
 */
class RenderDebugVertex
{
public:
    /**
    \brief  The world-space position
    */
    float mPos[3];
    /**
    \brief The diffuse color as 32 bit ARGB
    */
    uint32_t mColor;
};

/**
\brief This enumeration determines how point rendering should be rendered.
*/
enum PointRenderMode
{
    PRM_WIREFRAME_CROSS,    // Render as a small wireframe cross
    PRM_BILLBOARD,            // Render as a small billboard
    PRM_MESH                // Render points as a small low-poly count mesh
};


/**
The InputEvent class provides a way to retrieve IO status remotely; for keyboard, mouse, and game controller
*/
class InputEvent
{
public:
    enum EvenType
    {
        ET_DIGITAL,
        ET_ANALOG,
        ET_POINTER
    };
    InputEvent(void)
    {
        mReserved = 0;
        mId = 0;
        mEventType = ET_DIGITAL;
        mSensitivity = 0;
        mDigitalValue = 0;
        mAnalogValue = 0;
        mMouseX = 0;
        mMouseY = 0;
        mMouseDX = 0;
        mMouseDY = 0;
        mWindowSizeX = 0;
        mWindowSizeY = 0;
        mEyePosition[0] = 0;
        mEyePosition[1] = 0;
        mEyePosition[2] = 0;
        mEyeDirection[0] = 0;
        mEyeDirection[1] = 0;
        mEyeDirection[2] = 0;
        mCommunicationsFrame = 0;
        mRenderFrame = 0;
    }
    /**
    \brief A reserved word use for packet transmission.
    */
    uint32_t    mReserved;
    /**
    \brief The ID associated with this event
    */
    uint32_t    mId;
    /**
    \brief The communications frame number this event was issued
    */
    uint32_t    mCommunicationsFrame;

    /**
    \brief The renderframe frame number this event was issued
    */
    uint32_t    mRenderFrame;
    /**
    \brief whether or not this is an analog and digital event, 0 if digital 1 if analog
    */
    uint32_t    mEventType;
    /**
    \brief The sensitivity of this of this input event
    */
    float        mSensitivity;
    /**
    \brief The digital value if this input has a specific value.
    */
    int32_t        mDigitalValue;
    /**
    \brief The analog value if this input has a specific value.
    */
    float        mAnalogValue;
    /**
    \brief The x mouse position
    */
    uint32_t        mMouseX;
    /**
    \brief The y mouse position
    */
    uint32_t        mMouseY;
    /**
    \brief The delta x mouse position (normalized screen space 0-1)
    */
    float        mMouseDX;
    /**
    \brief The delta y mouse position (normalized screen space 0-1)
    */
    float        mMouseDY;
    /**
    \brief The window size X
    */
    uint32_t    mWindowSizeX;
    /**
    \brief The window size Y
    */
    uint32_t    mWindowSizeY;
    /**
    \brief The eye position of the camera
    */
    float        mEyePosition[3];
    /**
    \brief The eye direction of the camera
    */
    float        mEyeDirection[3];
};

/**
\brief This class defines user provided callback interface to actually display the debug rendering output.

If your application is running in client mode, then you do not need to implement this interface as the
remote DebugView application will render everything for you.  However, if you wish to use the RenderDebug interface
in either local or server mode, you should implement these routines to get the lines and triangles to render.
 */
class RenderDebugInterface
{
public:
    /**
    \brief Implement this method to display lines output from the RenderDebug library

    \param lcount The number of lines to draw (vertex count=lines*2)
    \param vertices The pairs of vertices for each line segment
    \param useZ Whether or not these should be zbuffered
    \param isScreenSpace Whether or not these are in homogeneous screen-space co-ordinates
    */
    virtual void debugRenderLines(uint32_t lcount,
                                const RenderDebugVertex *vertices,
                                bool useZ,
                                bool isScreenSpace) = 0;

    /**
    \brief Implement this method to display solid shaded triangles without any texture surce.

    \param tcount The number of triangles to render. (vertex count=tcount*2)
    \param vertices The vertices for each triangle
    \param useZ Whether or not these should be zbuffered
    \param isScreenSpace Whether or not these are in homogeneous screen-space co-ordinates
    */
    virtual void debugRenderTriangles(uint32_t tcount,
                                const RenderDebugSolidVertex *vertices,
                                bool useZ,
                                bool isScreenSpace) = 0;
    /**
    \brief Implement this method to display messages which were queued either locally or remotely.

    \param msg A generic informational log message
    */
    virtual void debugMessage(const char *msg) = 0;

    /**
    \brief Render a set of instanced triangle meshes.

    This is an optional callback to implement if your renderer can handle instanced
    triangle meshes.  If you don't implement it, then it will default to doing nothing.

    \param meshId The ID of the previously created triangle mesh
    \param textureId1 The ID of the primary texture
    \param textureTile1 The UV tiling rate of the primary texture
    \param textureId2 The ID of the secondary texture
    \param textureTile2 The UV tiling rate of the secondary texture
    \param instanceCount The number of instances to render
    \param instances The array of poses for each instance
    */
    virtual void renderTriangleMeshInstances(uint32_t meshId,
                                             uint32_t textureId1,
                                             float textureTile1,
                                             uint32_t textureId2,
                                             float textureTile2,
                                             uint32_t instanceCount,
                                             const RenderDebugInstance * instances)    = 0;

    /**
    \brief Create a triangle mesh that we can render.  Assumes an indexed triangle mesh.  User provides a *unique* id.  If it is not unique, this will fail.

    This is an method for you to implement if your renderer can handle drawing instanced triangle meshes.
    If it can, then you take the source mesh provided (which may or may not contain indices) and convert it to whatever you internal mesh format is.
    Once you have converted into your own internal mesh format, you should use the meshId provided for future reference to it.

    \param meshId The unique mesh ID
    \param vcount The number of vertices in the triangle mesh
    \param meshVertices The array of vertices
    \param tcount The number of triangles (indices must contain tcount*3 values) If zero, assumed to just be a triangle list (3 vertices per triangle, no indices)
    \param indices The array of triangle mesh indices
    */
    virtual void createTriangleMesh(uint32_t meshId,
                                    uint32_t vcount,
                                    const RenderDebugMeshVertex * meshVertices,
                                    uint32_t tcount,
                                    const uint32_t * indices) = 0;

    /**
    \brief Refreshes a subset of the vertices in the previously created triangle mesh; this is primarily designed to be used for real-time updates of heightfield or similar data.

    This is an method for you to implement if your renderer can handle refreshing the vertices in a previously created triangle mesh

    \param meshId The mesh id of the triangle mesh we are refreshing
    \param vcount The number of vertices to refresh
    \param refreshVertices The array of vertices to update
    \param refreshIndices The array of indicices corresponding to which vertices are to be revised.
    */
    virtual void refreshTriangleMeshVertices(uint32_t meshId,
                                            uint32_t vcount,
                                            const RenderDebugMeshVertex * refreshVertices,
                                            const uint32_t * refreshIndices) = 0;


    /**
    \brief Release a previously created triangle mesh

    \param meshId The meshId of the previously created triangle mesh.
    */
    virtual void releaseTriangleMesh(uint32_t meshId) = 0;

    /**
    \brief Debug visualize a text string rendered using a simple 2d font.

    \param x The X position of the text in normalized screen space 0 is the left of the screen and 1 is the right of the screen.
    \param y The Y position of the text in normalized screen space 0 is the top of the screen and 1 is the bottom of the screen.
    \param scale The scale of the font; these are not true-type fonts, so this is simple sprite scales.  A scale of 0.5 is a nice default value.
    \param shadowOffset The font is displayed with a drop shadow to make it easier to distinguish against the background; a value between 1 to 6 looks ok.
    \param forceFixWidthNumbers This bool controls whether numeric values are printed as fixed width or not.
    \param textColor the 32 bit ARGB value color to use for this piece of text.
    \param textString The string to print on the screen
    */
    virtual void debugText2D(float    x,
                            float    y,
                            float    scale,
                            float    shadowOffset,
                            bool    forceFixWidthNumbers,
                            uint32_t textColor,
                            const char *textString) = 0;

    /**
    \brief Create a custom texture associated with this name and id number

    \param id The id number to associated with this texture, id must be greater than 28
    \param textureName The name of the texture associated with this creation call
    */
    virtual void createCustomTexture(uint32_t id,const char *textureName) = 0;

    /**
    \brief Render a set of data points, either as wire-frame cross hairs or as a small solid instanced mesh.

    This callback provides the ability to (as rapidly as possible) render a large number of

    \param mode Determines what mode to render the point data
    \param meshId The ID of the previously created triangle mesh if rendering in mesh mode
    \param pointColor The color to render the points with, based on the current arrow color in the render state.
    \param pointScale The scale of the points, based on the arrow size in the current render state.
    \param textureId1 The ID of the primary texture
    \param textureTile1 The UV tiling rate of the primary texture
    \param textureId2 The ID of the secondary texture
    \param textureTile2 The UV tiling rate of the secondary texture
    \param pointCount The number of points to render
    \param points The array of points to render
    */
    virtual void debugPoints(PointRenderMode mode,
                            uint32_t meshId,
                            uint32_t pointColor,
                            float pointScale,
                            uint32_t textureId1,
                            float textureTile1,
                            uint32_t textureId2,
                            float textureTile2,
                            uint32_t    pointCount,
                            const float *points) = 0;

    /**
    \brief register a digital input event (maps to windows keyboard commands)

    \param eventId The eventId for this input event; if it is a custom event it must be greater than NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS
    \param inputId This the pre-defined keyboard code for this input event.
    */
    virtual void registerDigitalInputEvent(InputEventIds::Enum eventId,InputIds::Enum inputId) = 0;

    /**
    \brief register a digital input event (maps to windows keyboard commands)

    \param eventId The eventId for this input event; if it is a custom event it must be greater than NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS
    \param sensitivity The sensitivity value associated with this anaglog devent; default value is 1.0
    \param inputId This the pre-defined analog code for this input event.
    */
    virtual void registerAnalogInputEvent(InputEventIds::Enum eventId,float sensitivity,InputIds::Enum inputId) = 0;

    /**
    \brief unregister a previously registered input event.

    \param eventId The id of the previously registered input event.
    */
    virtual void unregisterInputEvent(InputEventIds::Enum eventId) = 0;

    /**
    \brief Reset all input events to an empty state
    */
    virtual void resetInputEvents(void) = 0;

protected:
    virtual ~RenderDebugInterface(void)
    {

    }
};

/**
\brief optional default value for a namespace representing a file request
*/
#define NV_FILE_REQUEST_NAMESPACE "NvFileRequest"

/**
\brief This is an optional pure virtual interface if you wish to provide the ability to retrieve remote resource requests.  Typically only implemented for servers.
*/
class RenderDebugResource
{
public:
    /**
    \brief Optional callback to allow the application to load a named resource.

    You do not have to implement this if you do not wish.  It is required for the ability to request a resource remotely.

    \param nameSpace The namespace (type) of resource being requested.
    \param resourceName The name of the source begin requested
    \param len The length of the requested resource returned.

    \return Returns the pointer to the resource.
    */
    virtual const void *requestResource(const char * nameSpace,const char * resourceName,uint32_t &len) = 0;

    /**
    \brief This method is called when the resource is no longer needed and the application can free up any memory associated with it.

    \param data The resource data that was returned by a previous call to 'requestResource'
    \param len The length of the resource data
    \param nameSpace The namespace (type) of data
    \param resourceName The name of the source

    \return Returns false if this was an unexpected release call.
    */
    virtual bool releaseResource(const void *data,uint32_t len,const char *nameSpace,const char *resourceName) = 0;
protected:
    /**
    \brief Default destructor declaration
    */
    virtual ~RenderDebugResource(void)
    {

    }
};

/**
\brief This is the main RenderDebug base class which provides all debug visualization services.

This interface uses standard C99 data types (uint32_t, float, etc.) so that it has no other
dependencies on any specific math library type.

There is an optional 'type safe' interface which accepts the NVIDIA foundation math types such as
NvVec3, NvMat44, NvQuat, etc.

If you are using the NVIDIA foundation math types you can avoid having to cast inputs to const float pointers
by retrieving the RenderDebugTyped interface, declared in NvRenderDebugTyped.h, and using the method 'getRenderDebugTyped' on the base class.
 */
class RenderDebug
{
public:
    /**
    \brief This enumeration defines the mode and behavior for how the RenderDebug library is to be created.
    */
    enum RunMode
    {
        RM_SERVER,            // running as a server; there can only be one server active at a time
        RM_CLIENT,            // This is a client which intends to get rendering services from a server; if the server is not found then no connection is created.
        RM_LOCAL,            // Just uses the render-debug library for rendering only; disables all client/server communications; just lets an app use it as a debug renderer.
        RM_CLIENT_OR_FILE,  // Runs as a client which connects to a server (if found) or, if no server found, then writes the output to a file that the server can later load and replay
        RM_FILE                // Runs *only* as a file; never tries to connect to a server.
    };


    /**
    \brief This is an optional, but highly recommended, callback when connection to the remote DebugView application.

    The client application can (and probably should) provide this callback to deal with long waits from the server.
    For remote connections there can be long stalls waiting for the server to have received the debug visualization data and send back the acknowledge.
    To prevent the application from looking like it us hung, this callback gives it a chance to abort the connection.
    This callback will pass in the number of milliseconds that you have been waiting for the sever to respond.  If you return true, then it will keep waiting.
    If you return false, it will disconnect from the server and stop trying.

    */
    class ServerStallCallback
    {
    public:
        /**
        \brief A callback to the client application to indicate that the client is stalled waiting for an acknowledge from the server.

        \param ms This is the number of milliseconds that the client has been waiting for a response from the server.
        \return If you return true, then the client will contain to wait for a response from the server.  If you return false it will close the connection.
        */
        virtual bool continueWaitingForServer(uint32_t ms) = 0;
    protected:
        /**
        \brief The default protected virtual destructor
        */
        virtual ~ServerStallCallback(void)
        {

        }
    };

    /**
    \brief A class to use when loading the DLL and initializing the RenderDebug library initial state.
    */
    class Desc
    {
    public:
        Desc(void)
        {
            // default name; you should assign this based on your build configuration x86 or x64
            // and use debug if you want to debug at the source level.  Also, if your DLL isn't
            // in the current working directory; you can use a fully qualified path name here.
            dllName = "RenderDebug_x86.dll";
            versionNumber = RENDER_DEBUG_VERSION;
            runMode = RM_LOCAL;
            recordFileName = NULL; //"RenderDebug.rec";
            errorCode = 0;
            echoFileLocally = false;
            hostName = "localhost";
            portNumber = RENDER_DEBUG_PORT;
            maxServerWait = 1000*60;    // If we don't get an ack from the server for 60 seconds, then we disconnect.
            serverStallCallback = 0;    // This is an optional, but highly recommended, callback to the application notifying it that it is stalled waiting for the server.  The user can provide a way to escape cleanly so the application doesn't look like it is in a hung state.
            applicationName = "GenericApplication";
            streamFileName = 0;
            renderDebugResource = 0;
            allocatorCallback = 0;
            errorCallback = 0;
            recordRemoteCommands = 0;
            playbackRemoteCommands = 0;
        }
        /**
        \brief Name of the render-debug DLL to load. (only valid for windows, other platforms simply link in the library)
        */
        const char                *dllName;

        /**
        \brief used to give feedback regarding connection status and registry settings for user-interface values.
        */
        const char                *applicationName;

        /**
        \brief expected version number; if not equal then the DLL won't load.
        */
        uint32_t                versionNumber;
        /**
        \brief startup mode indicating if we are running as a client or a server and with what connection options.
        */
        RunMode                    runMode;

        /**
        \brief If running in 'file' mode, this is the name of the file on disk the data will be rendering data will be recorded to.
        */
        const char                *recordFileName;

        /**
        \brief If it failed to create the render-debug system; this will contain a string explaining why.
        */
        const char                *errorCode;

        /**
        \brief If recording to a file, do we also want to echo the debugging commands locally (for both local render *and* record file at the same time).
        */
        bool                    echoFileLocally;

        /**
        \brief Name of the host to use for TCP/IP connections.
        */
        const char                *hostName;

        /**
        \brief The port number to connect to, the default port is 5525
        */
        uint16_t                portNumber;

        /**
        \brief The maximum number of milliseconds to wait for the server to respond for giving up and closing the connection.
        */
        uint32_t                maxServerWait;
        /**
        \brief  This is an optional, but highly recommended, callback to the application notifying it that it is stalled waiting for the server.  The user can provide a way to escape cleanly so the application doesn't look like it is in a hung state.
        */

        ServerStallCallback        *serverStallCallback;
        /**
        \brief This is an optional callback interface to request named resources from the application.  Should be implemented if you want to request by name resources remotely.
        */
        RenderDebugResource        *renderDebugResource;

        /**
        \brief This is an optional callback interface to vector all memory allocations performed back to the application.
        */
        nvidia::NvAllocatorCallback *allocatorCallback;

        /**
        \brief This is an optional callback interface to vector all warning and error messages back to the application
        */
        nvidia::NvErrorCallback        *errorCallback;

        /**
        \brief This is an optional filename to record all commands received from the remote connection. This is a debugging feature only.
        */
        const char                    *recordRemoteCommands;

        /**
        \brief This is an optional filename to play back previously recorded remote commands; if this file is found then these commands will be streamed back in the same order they were recorded
        */
        const char                    *playbackRemoteCommands;

        /**
        \brief This is an optional filename to record all communications from the client to a stream file on disk which can later be played back. Will be appended with a connection # and postfix of .nvcs
        */
        const char                    *streamFileName;
    };

    //***********************************************************************************
    //** State Management
    //***********************************************************************************

    /**
    \brief Push the current render state on the stack

    For speed and efficiency, the RenderDebug library uses a state-stack which can easily be pushed and popped on and off a stack.
    This prevents the user from having to pass in a huge amount of state with every single primitive call.  For example, rather than having
    to pass the color of a triangle each time you call 'debugTri', instead you set the current render state color and from that point forward
    all 'drawTri' calls will use the currently selected color.  You can push and pop the renderstate to make sure that you leave the state unchanged
    when making recursive calls to routines.  Pushing and popping the render state is very fast and inexpensive to peform.

    The render-state includes a variety of flags which control not only the current color but whether things should be drawn in wireframe, or solid, or solid with wireframe outline

    */
    virtual void  pushRenderState(void) = 0;

    /**
    \brief Pops the last render state off the stack.
    */
    virtual void  popRenderState(void) = 0;

    /**
    \brief Set the current primary and secondary draw colors

    \param color This is the main draw color as a 32 bit ARGB value
    \param arrowColor This is the secondary color, used for primitives which have two colors, like a ray with an arrow head, or a double sided triangle, or a solid shaded triangle with a wireframe outline.
    */
    virtual void  setCurrentColor(uint32_t color=0xFFFFFF,uint32_t arrowColor=0xFF0000) = 0;

    /**
    \brief Gets the current primary draw color

    \return Returns the current primary color
    */
    virtual uint32_t getCurrentColor(void) const = 0;

    /**
    \brief Set the current debug texture

    \param textureEnum1 Which predefined texture to use as the primary texture
    \param tileRate1 The tiling rate to use for the primary texture
    \param textureEnum2 Which (optional) predefined texture to use as the primary texture
    \param tileRate2 The tiling rate to use for the secondary texture
    */
    virtual void setCurrentTexture(DebugTextures::Enum textureEnum1,
                                    float tileRate1,
                                    DebugTextures::Enum textureEnum2,
                                    float tileRate2) = 0;

    /**
    \brief Gets the current debug primary texture

    \return The current texture id of the primary texture
    */
    virtual DebugTextures::Enum getCurrentTexture1(void) const = 0;

    /**
    \brief Gets the current debug secondary texture

    \return The current texture id of the secondary texture
    */
    virtual DebugTextures::Enum getCurrentTexture2(void) const = 0;

    /**
    \brief Gets the current tiling rate of the primary texture

    \return The current tiling rate of the primary texture
    */
    virtual float getCurrentTile1(void) const = 0;

    /**
    \brief Gets the current tiling rate of the secondary texture

    \return The current tiling rate of the secondary texture
    */
    virtual float getCurrentTile2(void) const = 0;
    /**
    \brief Create a debug texture based on a filename

    \param id The id associated with this custom texture, must be greater than 28; the reserved ids for detail textures
    \param fname The name of the DDS file associated with this texture.
    */
    virtual void createCustomTexture(uint32_t id,const char *fname) = 0;

    /**
    \brief Get the current secondary draw color (typically used for the color of arrow head, or the color for wire-frame outlines.

    \return Returns the current secondary color
    */
    virtual uint32_t getCurrentArrowColor(void) const = 0;

    /**
    \brief Sets a general purpose user id which is preserved by the renderstate stack

    \param userId Sets a current arbitrary userId which is preserved on the state stack
    */
    virtual void  setCurrentUserId(int32_t userId) = 0;

    /**
    \brief Gets the current user id

    \return Returns the current user id
    */
    virtual int32_t getCurrentUserId(void) = 0;

    /**
    \brief Set the current display time, this is the duration/lifetime of any draw primitives

    One important feature of the RenderDebug library is that you can specify how long you want a debug visualization primitive to display.
    This is particularly useful for transient events like contacts or raycast results which typically only happen on one frame.  If you draw these
    contacts they will show up and disappear so quickly they can be impossible to see.  Using this feature if you specify a current display time of
    say, for example, two seconds, then the contact will stay on the screen long enough for you to see.  Of course you should be careful with this feature
    as it can cause huge quantities of draw calls to get stacked up over long periods of time.  It should be reserved for discrete transient events.

    \param displayTime This is the current duration/lifetime of any draw primitive.
    */
    virtual void  setCurrentDisplayTime(float displayTime=0.0001f) = 0;

    /**
    \brief Get the current global debug rendering scale

    \return Returns the global rendering scale
    */
    virtual float getRenderScale(void) = 0;

    /**
    \brief Set the current global debug rendering scale.

    /param scale The global rendering scale to apply to all outputs from the render debug library before it shows up on screen.
    */
    virtual void  setRenderScale(float scale) = 0;

    /**
    \brief Set the complete current set of RENDER_DEBUG::DebugRenderState bits explicitly.

    /param states Rather than setting and clearing individual DebugRenderState flags, this method can just set them all to a specific set of values ored together.
    */
    virtual void  setCurrentState(uint32_t states=0) = 0;

    /**
    \brief Add a bit to the current render state.

    /param state Enables a particular DebugRenderState flag
    */
    virtual void  addToCurrentState(RENDER_DEBUG::DebugRenderState::Enum state) = 0; // OR this state flag into the current state.

    /**
    \brief Clear a bit from the current render state.

    /param state Disables a particular DebugRenderState flag
    */
    virtual void  removeFromCurrentState(RENDER_DEBUG::DebugRenderState::Enum state) = 0; // Remove this bit flat from the current state

    /**
    \brief Set the current scale for 3d text

    /param testScale Sets the current scale to apply to 3d printed text
    */
    virtual void  setCurrentTextScale(float textScale) = 0;

    /**
    \brief Set the current arrow head size for rays and other pointer style primitives

    /param arrowSize Set's the current default size for arrow heads.
    */
    virtual void  setCurrentArrowSize(float arrowSize) = 0;

    /**
    \brief Get the current RENDER_DEBUG::DebugRenderState bit fields.

    \return Returns the compute set of render state bites.
    */
    virtual uint32_t getCurrentState(void) const = 0;

    /**
    \brief Set the entire render state in one call rather than doing discrete calls one a time.

    \param states combination of render state flags
    \param color base color duration of display items.
    \param displayTime secondary color, usually used for arrow head
    \param arrowColor The size of arrow heads
    \param arrowSize The global render scale
    \param renderScale The global scale for debug visualization output
    \param textScale The current scale for 3d text
    */
    virtual void  setRenderState(uint32_t states=0,
                                uint32_t color=0xFFFFFF,
                                float displayTime=0.0001f,
                                uint32_t arrowColor=0xFF0000,
                                float arrowSize=0.1f,
                                float renderScale=1.0f,
                                float textScale=1.0f) = 0;


    /**
    \brief Get the entire current render state. Return the RENDER_DEBUG::DebugRenderState

    \param color Primary color
    \param displayTime display time
    \param arrowColor Secondary color
    \param arrowSize Arrow size
    \param renderScale Global render scale
    \param textScale Global text scale
    */
    virtual uint32_t getRenderState(uint32_t &color,
                                    float &displayTime,
                                    uint32_t &arrowColor,
                                    float &arrowSize,
                                    float &renderScale,
                                    float &textScale) const = 0;


    /**
    \brief Sets the global pose for the current debug-rendering context. This is preserved on the state stack.

    \param pose Sets the current global pose for the RenderDebug context, all draw commands will be transformed by this root pose. Default is identity.
    */
    virtual void  setPose(const float pose[16]) = 0;

    /**
    \brief Sets the global pose for the current debug-rendering context. This is preserved on the state stack.

    \param pos Sets the translation component
    \param quat Sets the rotation component as a quat
    */
    virtual void setPose(const float pos[3],const float quat[4]) = 0;

    /**
    \brief Sets the global pose position only, does not change the rotation.

    \param pos Sets the translation component
    */
    virtual void setPosition(const float pos[3]) = 0;

    /**
    \brief Sets the global pose orientation only, does not change the position

    \param quat Sets the orientation of the global pose
    */
    virtual void setOrientation(const float quat[3]) = 0;

    /**
    \brief Sets the global pose back to identity
    */
    virtual void setIdentityPose(void) = 0;

    /**
    \brief Gets the global pose for the current debug rendering context as a 4x4 transform

    \return Returns the current global pose for the RenderDebug
    */
    virtual const float * getPose(void) const = 0;


    //***********************************************************************************
    //** Lines and Triangles
    //***********************************************************************************

    /**
    \brief Draw a grid visualization.
    \param zup Whether the grid is drawn relative to a z-up axis or y-up axis, y-up axis is the default.
    \param gridSize This is the size of the grid.
    */
    virtual void  drawGrid(bool zup=false,
                            uint32_t gridSize=40) = 0; // draw a grid.


    /**
    \brief Draw a 2d rectangle in homogeneous screen-space coordinates.

    /param x1 Upper left of 2d rectangle
    /param y1 Top of 2d rectangle
    /param x2 Lower right of 2d rectangle
    /param y2 Bottom of 2d rectangle
    */
    virtual void debugRect2d(float x1,
                            float y1,
                            float x2,
                            float y2) = 0;

    /**
    \brief Draw a polygon; 'points' is an array of 3d vectors.

    \param pcount The number of data points in the polygon.
    \param points The array of Vec3 points comprising the polygon boundary
    */
    virtual void  debugPolygon(uint32_t pcount,
                                const float *points) = 0;

    /**
    \brief Draw a single line using the current color state

    \param p1 Starting position
    \param p2 Line end position
    */
    virtual void  debugLine(const float p1[3],
                            const float p2[3]) = 0;

    /**
    \brief Draw a gradient line (different start color from end color)

    \param p1 The starting location of the line in 3d
    \param p2 The ending location of the line in 3d
    \param c1 The starting color as a 32 bit ARGB format
    \param c2 The ending color as a 32 bit ARGB format
    */
    virtual void  debugGradientLine(const float p1[3],
                                    const float p2[3],
                                    const uint32_t &c1,
                                    const uint32_t &c2) = 0;

    /**
    \brief Draws a wireframe line with a small arrow head pointing along the direction vector ending at P2

    \param p1 The start of the ray
    \param p2 The end of the ray, where the arrow head will appear
    */
    virtual void  debugRay(const float p1[3],
                            const float p2[3]) = 0;


    /**
    \brief Creates a debug visualization of a 'thick' ray.  Extrudes a cylinder visualization with a nice arrow head.

    \param p1 Starting point of the ray
    \param p2 Ending point of the ray
    \param raySize The thickness of the ray, the arrow head size is used for the arrow head.
    \param arrowTip Whether or not the arrow tip should appear, by default this is true.
    */
    virtual void  debugThickRay(const float p1[3],
                                const float p2[3],
                                float raySize=0.02f,
                                bool arrowTip=true) = 0;


    /**
    \brief Debug visualize a 3d triangle using the current render state flags

    \param p1 First point in the triangle
    \param p2 Second point in the triangle
    \param p3 The third point in the triangle
    */
    virtual void  debugTri(const float p1[3],
                            const float p2[3],
                            const float p3[3]) = 0;

    /**
    \brief Debug visualize a 3d triangle with provided vertex lighting normals.

    \param p1 First point in the triangle
    \param p2 Second point in the triangle
    \param p3 The third point in the triangle
    \param n1 First normal in the triangle
    \param n2 Second normal in the triangle
    \param n3 The third normal in the triangle
    */
    virtual void  debugTriNormals(const float p1[3],
                                const float p2[3],
                                const float p3[3],
                                const float n1[3],
                                const float n2[3],
                                const float n3[3]) = 0;

    /**
    \brief Debug visualize a 3d triangle with a unique color at each vertex.

    \param p1 First point in the triangle
    \param p2 Second point in the triangle
    \param p3 The third point in the triangle
    \param c1 The color of the first vertex
    \param c2 The color of the second vertex
    \param c3 The color of the third vertex
    */
    virtual void  debugGradientTri(const float p1[3],
                                    const float p2[3],
                                    const float p3[3],
                                    const uint32_t &c1,
                                    const uint32_t &c2,
                                    const uint32_t &c3) = 0;

    /**
    \brief Debug visualize a 3d triangle with provided vertex normals and colors

    \param p1 First point in the triangle
    \param p2 Second point in the triangle
    \param p3 The third point in the triangle
    \param n1 First normal in the triangle
    \param n2 Second normal in the triangle
    \param n3 The third normal in the triangle
    \param c1 The color of the first vertex
    \param c2 The color of the second vertex
    \param c3 The color of the third vertex
    */
    virtual void  debugGradientTriNormals(const float p1[3],
                                            const float p2[3],
                                            const float p3[3],
                                            const float n1[3],
                                            const float n2[3],
                                            const float n3[3],
                                            const uint32_t &c1,
                                            const uint32_t &c2,
                                            const uint32_t &c3) = 0;

    /**
    \brief Debug visualize a simple point as a small cross

    \param pos The position of the point
    \param radius The size of the point visualization.
    */
    virtual void  debugPoint(const float pos[3],
                            float radius) = 0;

    /**
    \brief Debug visualize a simple point with provided independent scale on the X, Y, and Z axis

    \param pos The position of the point
    \param scale The X/Y/Z scale of the crosshairs.
    */
    virtual void  debugPoint(const float pos[3],
                            const float scale[3]) = 0;

    /**
    \brief Debug visualize an arc as a line with an arrow head at the end.

    \param center The center of the arc
    \param p1 The starting position of the arc
    \param p2 The ending position of the arc
    \param arrowSize The size of the arrow head for the arc.
    \param showRoot Whether or not to debug visualize the center of the arc
    */
    virtual void debugArc(const float center[3],
                            const float p1[3],
                            const float p2[3],
                            float arrowSize=0.1f,
                            bool showRoot=false) = 0;

    /**
    \brief Debug visualize a thick arc

    \param center The center of the arc
    \param p1 The starting position of the arc
    \param p2 The ending position of the arc
    \param thickness The thickness of the cylinder drawn along the arc
    \param showRoot Whether or not to debug visualize the center of the arc
    */
    virtual void debugThickArc(const float center[3],
                                const float p1[3],
                                const float p2[3],
                                float thickness=0.02f,
                                bool showRoot=false) = 0;


    //***********************************************************************************
    //** Shapes
    //***********************************************************************************

    /**
    \brief Create a debug visualization of a cylinder from P1 to P2 with radius provided.

    \param p1 The starting point of the cylinder
    \param p2 The ending point of the cylinder
    \param radius The radius of the cylinder
    */
    virtual void  debugCylinder(const float p1[3],
                                const float p2[3],
                                float radius) = 0;

    /**
    \brief Creates a debug visualization of a plane equation as a couple of concentric circles

    \param normal The normal of the plane equation
    \param dCoff The d plane co-efficient of the plane equation
    \param radius1 The inner radius of the plane visualization
    \param radius2 The outer radius of the plane visualization
    */
    virtual void  debugPlane(const float normal[3],
                            float dCoff,
                            float radius1,
                            float radius2) = 0;

    /**
    \brief Debug visualize a 3d bounding box using the current render state.

    \param bmin The minimum X,Y,Z of the AABB
    \param bmax the maximum extent of the AABB
    */
    virtual void  debugBound(const float bmin[3],const float bmax[3]) = 0;

    /**
    \brief Debug visualize a crude sphere using the current render state settings.

    \param pos The center of the sphere
    \param radius The radius of the sphere.
    \param subdivision The detail level of the sphere, should not be less than 1 or more than 4
    */
    virtual void  debugSphere(const float pos[3],
                                float radius,
                                uint32_t subdivision=2) = 0;

    /**
    \brief Debug visualize a capsule relative to the currently set pose

    \param radius The radius of the capsule
    \param height The height of the capsule
    \param subdivision The number of subdivisions for the approximation, keep this very small. Like 2 or 3.
    */
    virtual void  debugCapsule(float radius,
                                float height,
                                uint32_t subdivision=2) = 0;

    /**
    \brief Debug visualize a tapered capsule relative to the currently set pose

    \param radius1 The start radius
    \param radius2 The end radius
    \param height The height
    \param subdivision The number of subdivisions, keep it small; 2 or 3
    */
    virtual void  debugCapsuleTapered(float radius1,
                                    float radius2,
                                    float height,
                                    uint32_t subdivision=2) = 0;

    /**
    \brief Debug visualize a cylinder relative to the currently set pose

    \param radius The radius of the cylinder
    \param height The height
    \param closeSides Whether or not the ends of the cylinder should be capped.
    \param subdivision The approximation subdivision (keep it small)

    */
    virtual void  debugCylinder(float radius,
                                float height,
                                bool closeSides,
                                uint32_t subdivision=2) = 0;

    /**
    \brief Debug visualize a circle relative to the currently set pose

    \param center The center of the circle
    \param radius The radius of the circle
    \param subdivision The number of subdivisions
    */
    virtual void  debugCircle(const float center[3],
                                float radius,
                                uint32_t subdivision=2) = 0;

    /**
    \brief Debug visualize a cone relative to the currently set pose

    \param length The length of the cone
    \param innerAngle The inner angle of the cone (in radians)
    \param outerAngle The outer angle of the cone (in radians)
    \param stepCount The number of sub-steps when drawing the cone
    \param closeEnd Whether or not to close up the bottom of the cone

    */
    virtual void debugCone(float length,float innerAngle,float outerAngle,uint32_t stepCount,bool closeEnd) = 0;


    //***********************************************************************************
    //** Matrix visualization
    //***********************************************************************************

    /**
    \brief Debug visualize a view*projection matrix frustum.

    \param viewMatrix The view matrix of the frustm
    \param projMatrix The projection matrix of the frustm
    */
    virtual void debugFrustum(const float viewMatrix[16],const float projMatrix[16]) = 0;

    /**
    \brief Debug visualize a 4x4 transform.

    \param transform The matrix we are trying to visualize
    \param distance The size of the visualization
    \param brightness The brightness of the axes
    \param showXYZ Whether or not to print 3d text XYZ labels
    \param showRotation Whether or not to visualize rotation or translation axes
    \param axisSwitch Which axis is currently selected/highlighted.
    \param renderMode How render axes of 4x4 transform
    */
    virtual void  debugAxes(const float transform[16],
                            float distance=0.1f,
                            float brightness=1.0f,
                            bool showXYZ=false,
                            bool showRotation=false,
                            uint32_t axisSwitch=0,
                            DebugAxesRenderMode::Enum renderMode = DebugAxesRenderMode::DEBUG_AXES_RENDER_SOLID
                            ) = 0;

    //***********************************************************************************
    //** 3D Text, 2D Text, Messages, and Commands
    //***********************************************************************************

    /**
    \brief Debug visualize a text string rendered a 3d wireframe lines.  Uses a printf style format.  Not all special symbols available, basic upper/lower case an simple punctuation.

    \param pos The position of the text
    \param fmt The printf style format string
    */
    virtual void debugText(const float pos[3],
                            const char *fmt,...) = 0;

    /**
    \brief Debug visualize a text string rendered using a simple 2d font.

    \param x The X position of the text in normalized screen space 0 is the left of the screen and 1 is the right of the screen.
    \param y The Y position of the text in normalized screen space 0 is the top of the screen and 1 is the bottom of the screen.
    \param scale The scale of the font; these are not true-type fonts, so this is simple sprite scales.  A scale of 0.5 is a nice default value.
    \param shadowOffset The font is displayed with a drop shadow to make it easier to distinguish against the background; a value between 1 to 6 looks ok.
    \param forceFixWidthNumbers This bool controls whether numeric values are printed as fixed width or not.
    \param textColor the 32 bit ARGB value color to use for this piece of text.
    \param fmt The printf style format string
    */
    virtual void debugText2D(float    x,
                            float    y,
                            float    scale,
                            float    shadowOffset,
                            bool    forceFixWidthNumbers,
                            uint32_t textColor,
                            const char *fmt,...) = 0;

    /**
    \brief Sends a debug log message to the remote client/server or recorded to a log file.

    \param fmt The printf style format string for the message
    */
    virtual void debugMessage(const char *fmt,...) = 0;

    /**
    \brief Send a command from the server to the client.  This could be any arbitrary console command, it can also be mouse drag events, debug visualization events, etc.
    * the client receives this command in argc/argv format.

    \param fmt The printf style format string for the message
    */
    virtual bool sendRemoteCommand(const char *fmt,...) = 0;

    /**
    \brief Transmits an arbitrary block of binary data to the remote machine.  The block of data can have a command and id associated with it.

    It is important to note that due to the fact the RenderDebug system is synchronized every single frame, it is strongly recommended
    that you only use this feature for relatively small data items; probably on the order of a few megabytes at most.  If you try to do
    a very large transfer, in theory it would work, but it might take a very long time to complete and look like a hang since it will
    essentially be blocking.

    \param nameSpace The namespace/type associated with this data transfer, for example this could indicate a remote file request.
    \param resourceName The name of the resource associated with this data transfer, for example the id could be the file name of a file transfer request.
    \param data The block of binary data to transmit, you are responsible for maintaining endian correctness of the internal data if necessary.
    \param dlen The length of the lock of data to transmit.

    \return Returns true if the data was queued to be transmitted, false if it failed.
    */
    virtual bool sendRemoteResource(const char *nameSpace,
                                    const char *resourceName,
                                    const void *data,
                                    uint32_t dlen) = 0;

    /**
    \brief This function allows you to request a file from the remote machine by name.  If successful it will be returned via 'getRemoteData'

    \param nameSpace The namespace/type associated with this data transfer, for example this could indicate a remote file request.
    \param resourceName The name of the resource associated with this data transfer, for example the id could be the file name of a file transfer request.

    \return Returns true if the request was queued to be transmitted, false if it failed.
    */
    virtual bool requestRemoteResource(const char *nameSpace,
                                    const char *resourceName) = 0;


    /**
    \brief If running in client mode, poll this method to retrieve any pending commands from the server.  If it returns NULL then there are no more commands.
    */
    virtual const char ** getRemoteCommand(uint32_t &argc) = 0;

    /**
    \brief Retrieves a block of remotely transmitted binary data.

    \param nameSpace A a reference to a pointer which will store the namespace (type) associated with this data transfer, for example this could indicate a remote file request.
    \param resourceName A reference to a pointer which will store the resource name associated with this data transfer, for example the resource name could be the file name of a file transfer request.
    \param dlen A reference that will contain length of the lock of data received.
    \param remoteIsBigEndian A reference to a boolean which will be set to true if the remote machine that sent this data is big endian format.

    \return A pointer to the block of data received.
    */
    virtual const void * getRemoteResource(const char *&nameSpace,
                                        const char *&resourceName,
                                        uint32_t &dlen,
                                        bool &remoteIsBigEndian) = 0;


    //***********************************************************************************
    //** Draw Groups
    //***********************************************************************************

    /**
    \brief Resets either a specific block of debug data or all blocks.

    A series of of draw commands can be batched up into blocks, kind of like macros, however nested blocks are not supported.
    For example, if you had a particular collection of draw calls which were repeated every frame, instead of passing them each time, instead
    you could use the beginBlock/endBlock methods to cache them.  Then, on future frames, you could simply render that batch of commands with
    a single draw call.  Each logical block of commands can be issued relative to a pose, so this way you can implement a form of instancing
    as well.

    \param blockIndex -1 reset *everything*, 0 = reset everything except stuff inside blocks, > 0 reset a specific block of data.
    */
    virtual void  reset(int32_t blockIndex=-1) = 0;

    /**
    \brief Begins a draw group relative to this 4x4 matrix.  Returns the draw group id. A draw group is like a macro set of drawing commands.

    \param pose The base pose of this draw group.
    */
    virtual int32_t beginDrawGroup(const float pose[16]) = 0;

    /**
    \brief Mark the end of a draw group.
    */
    virtual void  endDrawGroup(void) = 0;

    /**
    \brief Indicate whether a particular draw group is currently visible or not

    \param groupId The draw group ID
    \param state Whether it should be visible or not.
    */
    virtual void  setDrawGroupVisible(int32_t groupId,
                                        bool state) = 0;


    /**
    \brief Revises the transform for a previously defined draw group.

    \param blockId The id number of the block we are referencing
    \param pose The pose of that block id
    */
    virtual void  setDrawGroupPose(int32_t blockId,const float pose[16]) = 0;


    //***********************************************************************************
    //** Screenspace Support
    //***********************************************************************************

    /**
    \brief Create a 2d screen-space graph
    \param numPoints The number of points in the graph
    \param points The set of data points.
    \param graphMax The maximum Y axis
    \param graphXPos The x screen space position for this graph
    \param graphYPos The y screen space position for this graph.
    \param graphWidth The width of the graph
    \param graphHeight The height of the graph
    \param colorSwitchIndex    Undocumented
    */
    virtual void debugGraph(uint32_t numPoints,
                            float * points,
                            float graphMax,
                            float graphXPos,
                            float graphYPos,
                            float graphWidth,
                            float graphHeight,
                            uint32_t colorSwitchIndex = 0xFFFFFFFF) = 0;


    /**
    \brief Debug visualize a quad in screenspace (always screen facing)

    \param pos The position of the quad
    \param scale The 2d scale
    \param orientation The 2d orientation in radians
    */
    virtual void  debugQuad(const float pos[3],
                            const float scale[2],
                            float orientation) = 0;

    /**
    \brief Sets the view matrix as a full 4x4 matrix.  Required for screen-space aligned debug rendering. If you are not trying to do screen-facing or 2d screenspace rendering this is not required.

    \param view The 4x4 view matrix  This is not transmitted remotely to the server, this is only for local rendering.
    */
    virtual void setViewMatrix(const float view[16]) = 0;

    /**
    \brief Sets the projection matrix as a full 4x4 matrix.  Required for screen-space aligned debug rendering.

    \param projection This current projection matrix, note this is not transmitted to the server, it is only used locally.
    */
    virtual void setProjectionMatrix(const float projection[16]) = 0;


    //***********************************************************************************
    //** Instanced Triangle Methods
    //***********************************************************************************

    /**
    \brief A convenience method to return a unique mesh id number (simply a global counter to avoid clashing with other ids
    */
    virtual uint32_t getMeshId(void) = 0;

    /**
    \brief Render a set of instanced triangle meshes.

    \param meshId This id of the instanced mesh to render
    \param instanceCount The number of instances of this mesh to render
    \param instances The array of 3x4 instance transforms

    */
    virtual void renderTriangleMeshInstances(uint32_t meshId,
                                             uint32_t instanceCount,
                                             const RenderDebugInstance *instances) = 0;

    /**
    \brief Render a set of data points, either as wire-frame cross hairs or as a small solid instanced mesh.

    This callback provides the ability to (as rapidly as possible) render a large number of

    \param mode Determines what mode to render the point data
    \param meshId The ID of the previously created triangle mesh if rendering in mesh mode
    \param textureId1 The ID of the primary texture
    \param textureTile1 The UV tiling rate of the primary texture
    \param textureId2 The ID of the secondary texture
    \param textureTile2 The UV tiling rate of the secondary texture
    \param pointCount The number of points to render
    \param points The array of points to render
    */
    virtual void debugPoints(PointRenderMode mode,
                            uint32_t meshId,
                            uint32_t textureId1,
                            float textureTile1,
                            uint32_t textureId2,
                            float textureTile2,
                            uint32_t    pointCount,
                            const float *points) = 0;

    /**
    \brief This method will produce a debug visualization of a convex hull; either as lines or solid triang;es depending on the current debug render state

    \param planeCount The number of planes in the convex hull
    \param planes The array of plane equations in the convex hull
    */
    virtual void debugConvexHull(uint32_t planeCount,
                                const float *planes) = 0;

    /**
    \brief This is the fast path to render a large batch of lines instead of drawing them one at at time with debugLine

    \param lcount The number of lines to draw (vertex count=lines*2)
    \param vertices The pairs of vertices for each line segment
    \param useZ Whether or not these should be zbuffered
    \param isScreenSpace Whether or not these are in homogeneous screen-space co-ordinates
    */
    virtual void debugRenderLines(uint32_t lcount,
                                const RenderDebugVertex *vertices,
                                bool useZ,
                                bool isScreenSpace) = 0;

    /**
    \brief This is the fast path to render a large batch of solid shaded triangles instead of drawing them one at a time with debugTri

    \param tcount The number of triangles to render. (vertex count=tcount*2)
    \param vertices The vertices for each triangle
    \param useZ Whether or not these should be zbuffered
    \param isScreenSpace Whether or not these are in homogeneous screen-space co-ordinates
    */
    virtual void debugRenderTriangles(uint32_t tcount,
                                const RenderDebugSolidVertex *vertices,
                                bool useZ,
                                bool isScreenSpace) = 0;

    /**
    \brief Create a triangle mesh that we can render.  Assumes an indexed triangle mesh.  User provides a *unique* id.  If it is not unique, this will fail.

    \param meshId The unique mesh id to associate with this triangle mesh
    \param vcount The number of vertices in the triangle mesh
    \param meshVertices  The array of vertices
    \param tcount The number of triangles (indices must contain tcount*3 values if non-null)
    \param indices The optional array of triangle mesh indices.  If this is null then the mesh is assumed to be a triangle list with 3 vertices per triangle.

    */
    virtual void createTriangleMesh(uint32_t meshId,
                                    uint32_t vcount,
                                    const RenderDebugMeshVertex *meshVertices,
                                    uint32_t tcount,
                                    const uint32_t *indices) = 0;

    /**
    \brief Refresh a sub-section of the vertices in a previously created triangle mesh.

    \param meshId The mesh ID of the triangle mesh we are refreshing
    \param vcount This is the number of vertices to refresh.
    \param refreshVertices This is an array of revised vertex data.
    \param refreshIndices This is an array of indices which correspond to the original triangle mesh submitted.  There should be one index for each vertex.
    */
    virtual void refreshTriangleMeshVertices(uint32_t meshId,
                                            uint32_t vcount,
                                            const RenderDebugMeshVertex *refreshVertices,
                                            const uint32_t *refreshIndices) = 0;

    /**
    \brief Release a previously created triangle mesh
    */
    virtual void releaseTriangleMesh(uint32_t meshId) = 0;


    //***********************************************************************************
    //** Utility and Support
    //***********************************************************************************

    /**
    \brief Special case command that affects how the server processes the previous frame of data.

    \return Returns true if it is safe to skip this frame of data, false if there are required commands that must be executed.
    */
    virtual bool    trySkipFrame(void) = 0;

    /**
    \brief Returns the number of times that the 'render' method has been called, this is local.
    */
    virtual uint32_t getUpdateCount(void) const = 0;

    /**
    \brief Called once per frame to flush all debug visualization commands queued.
    \param dtime The amount of time which has passed for this frame
    \param iface The optional callback interface to process the raw lines and triangles, NULL for applications connected to a file or server.
    */
    virtual bool render(float dtime,
                    RenderDebugInterface *iface) = 0;

    /**
    \brief Returns the current view*projection matrix

    \return A convenience method to return the current view*projection matrix (local only)
    */
    virtual const float* getViewProjectionMatrix(void) const = 0;

    /**
    \brief Returns the current view matrix we are using

    \return  A convenience method which returns the currently set view matrix (local only)
    */
    virtual const float *getViewMatrix(void) const = 0;

    /**
    \brief Gets the current projection matrix.

    \return A convenience method to return the current projection matrix (local only)
    */
    virtual const float *getProjectionMatrix(void) const = 0;

    /**
    \brief A convenience helper method to convert euler angles (in degrees) into a standard XYZW quaternion.

    \param angles The X,Y,Z angles in degrees
    \param q The output quaternion
    */
    virtual void  eulerToQuat(const float angles[3],float q[4]) = 0; // angles are in degrees.

    /**
    \brief A convenience method to convert two positions into a 4x4 transform

    \param p0 The origin
    \param p1 The point we want to create a rotation arc towards
    \param xform The 4x4 output matrix transform

    */
    virtual void rotationArc(const float p0[3],const float p1[3],float xform[16]) = 0;

    /**
    \brief Set a debug color value by name.

    \param colorEnum Which of the predefined color choices
    \param value The 32 bit ARGB value to associate with this enumerated type
    */
    virtual void setDebugColor(DebugColors::Enum colorEnum, uint32_t value) = 0;

    /**
    \brief Return a debug color value by type

    \param colorEnum Which of the predefined enumerated color types
    \return Returns the 32 bit ARGB color value assigned to this enumeration
    */
    virtual uint32_t getDebugColor(DebugColors::Enum colorEnum) const = 0;

    /**
    \brief Return a debug color value by RGB inputs

    \param red The red component 0-1
    \param green The green component 0-1
    \param blue the blue component 0-1
    \return Returns floating point RGB values as 32 bit RGB format.
    */
    virtual uint32_t getDebugColor(float red, float green, float blue) const = 0;

    /**
    \brief Set the base file name to record communications stream; or NULL to disable it.

    \param fileName The base name of the file to record the communications channel stream to, or NULL to disable it.
    */
    virtual bool setStreamFilename(const char *fileName) = 0;

    /**
    \brief Begins a file-playback session. Returns the number of recorded frames in the recording file.  Zero if the file was not valid.
    */
    virtual uint32_t setFilePlayback(const char *fileName) = 0;

    /**
    \brief Begin playing back a communications stream recording

    \param fileName The name of the previously captured communications stream file
    */
    virtual bool setStreamPlayback(const char *fileName) = 0;

    /**
    \brief Sets the file playback to a specific frame.  Returns true if successful.
    */
    virtual bool setPlaybackFrame(uint32_t playbackFrame) = 0;

    /**
    \brief Returns the number of recorded frames in the debug render recording file.
    */
    virtual uint32_t getPlaybackFrameCount(void) const = 0;

    /**
    \brief Stops the current recording playback.
    */
    virtual void stopPlayback(void) = 0;

    /**
    \brief Do a 'try' lock on the global render debug mutex.  This is simply provided as an optional convenience if you are accessing RenderDebug from multiple threads and want to prevent contention.
    */
    virtual bool trylock(void) = 0;

    /**
    \brief Lock the global render-debug mutex to avoid thread contention.
    */
    virtual void lock(void) = 0;

    /**
    \brief Unlock the global render-debug mutex
    */
    virtual void unlock(void) = 0;

    /**
    \brief Report what 'Run' mode we are operating in.
    */
    virtual RunMode getRunMode(void) = 0;

    /**
    \brief Returns true if we still have a valid connection to the server.
    */
    virtual bool isConnected(void) const = 0;

    /**
    \brief Returns the current synchronized frame between client/server communications.  Returns zero if no active connection exists.
    */
    virtual uint32_t getCommunicationsFrame(void) const = 0;

    /**
    \brief Returns the name of the currently connected application
    */
    virtual const char *getRemoteApplicationName(void) = 0;

    /**
    \brief Returns the optional typed methods for various render debug routines.
    */
    virtual RenderDebugTyped *getRenderDebugTyped(void) = 0;

    /**
    \brief Release the render debug class
    */
    virtual void release(void) = 0;

    //***********************************************************************************
    //** Digital and Analog input support
    //***********************************************************************************

    /**
    \brief register a digital input event (maps to windows keyboard commands)

    \param eventId The eventId for this input event; if it is a custom event it must be greater than NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS
    \param inputId This the pre-defined keyboard code for this input event.
    */
    virtual void registerDigitalInputEvent(InputEventIds::Enum eventId,InputIds::Enum inputId) = 0;

    /**
    \brief register a digital input event (maps to windows keyboard commands)

    \param eventId The eventId for this input event; if it is a custom event it must be greater than NUM_SAMPLE_FRAMEWORK_INPUT_EVENT_IDS
    \param sensitivity The sensitivity value associated with this anaglog devent; default value is 1.0
    \param inputId This the pre-defined analog code for this input event.
    */
    virtual void registerAnalogInputEvent(InputEventIds::Enum eventId,float sensitivity,InputIds::Enum inputId) = 0;

    /**
    \brief unregister a previously registered input event.

    \param eventId The id of the previously registered input event.
    */
    virtual void unregisterInputEvent(InputEventIds::Enum eventId) = 0;

    /**
    \brief Reset all input events to an empty state
    */
    virtual void resetInputEvents(void) = 0;

    /**
    \brief Transmit an actual input event to the remote client

    \param ev The input event data to transmit
    */
    virtual void sendInputEvent(const InputEvent &ev) = 0;

    /**
    \brief Returns any incoming input event for processing purposes

    \param flush If this is true, the event will be flushed and no longer reported, if false this acts as a 'peek' operation for the next input event
    */
    virtual const InputEvent *getInputEvent(bool flush) = 0;

protected:

    virtual ~RenderDebug(void) { }
};

/**
\brief A convenience helper-class to create a scoped mutex-lock around calls into the debug render library, if you might potentially be calling it from multiple threads.
*/
class ScopedRenderDebug
{
public:
    /**
    \brief This constructor for ScopedRenderDebug initiates a global mutex lock to prevent other threads from accessing this instance of RenderDebug at the same time
    */
    ScopedRenderDebug(RenderDebug *rd)
    {
        mRenderDebug = rd;
        if ( mRenderDebug )
        {
            mRenderDebug->lock();
        }
    }

    /**
    \brief This is the destructor for ScopedRenderDebug which releases the previously acquired lock when this helper class goes out of scope
    */
    ~ScopedRenderDebug(void)
    {
        if ( mRenderDebug )
        {
            mRenderDebug->unlock();
        }
    }

    /**
    \brief The instance of RenderDebug that is to be locked
    */
    RenderDebug *mRenderDebug;
};

/**
\brief This is the single method to create an instance of the RenderDebug class using the properties supplied in the descriptor.

\param desc This contains the information about how to construct the RenderDebug interface; either as local, client, or server, and other options.

\return Returns a pointer to the RenderDebug interface if successfull.  It it returns null, then check the desc.errorCode value
*/
RENDER_DEBUG::RenderDebug *createRenderDebug(RENDER_DEBUG::RenderDebug::Desc &desc);


} // end of RENDER_DEBUG namespace

extern "C"
#if NV_WINDOWS_FAMILY != 0
__declspec(dllexport)
#endif
RENDER_DEBUG::RenderDebug* createRenderDebugExport(RENDER_DEBUG::RenderDebug::Desc &desc);

/**
\brief A helper macro to create an instanced of the ScopedRenderDebug class to apply a global mutex lock on access to the RenderDebug API
*/
#define SCOPED_RENDER_DEBUG_LOCK(x) RENDER_DEBUG::ScopedRenderDebug _lockRenderDebug(x)

#endif // RENDER_DEBUG_H
