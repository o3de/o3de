/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <CryCommon/platform.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/function/function_template.h>
#include <AzCore/std/string/string_view.h>

struct ConsoleBind;

struct ICVar;
class ITexture;
struct ISystem;

namespace AZ
{
    class Name;
}

#define     CVAR_INT              1
#define     CVAR_FLOAT          2
#define     CVAR_STRING         3

// By default, cvars registered as const, read-only, or cheat cannot be modified in release builds for security.
// The common practice has been to hard-code the default values of all these 'const' cvars before generating a release build.
// However, this is restrictive because the cannot be modified at run-time in release, and so cannot differ between (or during) levels.
// If you wish, you can disable this restriction by defining ALLOW_CONST_CVAR_MODIFICATIONS 1 (which is the default for non-release builds),
// thus allowing const/read-only/cheat cvars to be set at run-time in release, and removing the need to explicitly hard-code your default values for release builds.
#if defined(_RELEASE)
#define ALLOW_AUDIT_CVARS 0
#define ALLOW_CONST_CVAR_MODIFICATIONS AZ_TRAIT_CVARS_ENABLED_FOR_RELEASE_BUILDS
#define LOG_CVAR_INFRACTIONS 0
#define LOG_CVAR_USAGE 0
#else
#define ALLOW_AUDIT_CVARS 1
#define ALLOW_CONST_CVAR_MODIFICATIONS 1
#define LOG_CVAR_INFRACTIONS 1
#define LOG_CVAR_USAGE 1
#endif // defined(_RELEASE)

#define CVAR_GROUPS_ARE_PRIVILEGED 1 // allows cvar groups to manipulate const/cheat cvars (turn off to log group cvar manipulation)
#define LOG_CVAR_INFRACTIONS_CALLSTACK (LOG_CVAR_INFRACTIONS && 0) // logs the callstack when certain cvar issues arise; helpful in tracking down where the problem originated

// If this flag is set during registering a console variable, and the variable exists,
// then the variable will store its value in memory given by src
#define CVF_CHANGE_SOURCE (1u << 16)

// Flags used by ICVar
enum EVarFlags
{
    VF_NULL =                                           0x00000000,         // just to have one recognizable spot where the flags are located in the Register call
    VF_CHEAT =                                      0x00000002,         // stays in the default state when cheats are disabled
    VF_DEV_ONLY =                   0x00000004,         // cvar is only registered with the console in non release builds
    VF_DEDI_ONLY =                  0x00000008,         // cvar is only registered with the console in non release or dedicated server builds
    VF_NET_SYNCED =                             0x00000080,         // synchronised between server and client(s); server authorative
    VF_DUMPTODISK   =                               0x00000100,
    VF_READONLY =                                   0x00000800,         // can not be changed by the user
    VF_REQUIRE_LEVEL_RELOAD =           0x00001000,
    VF_REQUIRE_APP_RESTART =            0x00002000,
    VF_WARNING_NOTUSED  =                   0x00004000,         // shows warning that this var was not used in config file
    VF_COPYNAME =                                   0x00008000,         // otherwise the const char * to the name will be stored without copying the memory
    VF_MODIFIED =                                   0x00010000,         // Set when variable value modified.
    VF_WASINCONFIG  =                           0x00020000,         // Set when variable was present in config file.
    VF_BITFIELD =                                   0x00040000,         // Allow bitfield setting syntax.
    VF_RESTRICTEDMODE =                     0x00080000,         // is visible and usable in restricted (normal user) console mode
    VF_INVISIBLE     =                      0x00100000,         // Invisible to the user in console
    VF_ALWAYSONCHANGE =                     0x00200000,         // Always accept variable value and call on change callback even if variable value didnt change
    VF_BLOCKFRAME =                             0x00400000,         // Blocks the execution of console commands for one frame
    VF_CONST_CVAR =                             0x00800000,         // Set if it is a const cvar not to be set inside cfg-files
    VF_CHEAT_ALWAYS_CHECK =             0x01000000,         // This variable is critical to check in every hash, since it's extremely vulnerable
    VF_CHEAT_NOCHECK =                      0x02000000,       // This variable is set as VF_CHEAT but doesn't have to be checked/hashed since it's harmless to hack
    VF_SYSSPEC_OVERWRITE =              0x04000000,       // This variable is specified by system.cfg with the intention to overwrite all subsequent settings
    VF_CVARGRP_IGNOREINREALVAL =    0x08000000,       // This variable will be ignored when cvar group's real val is checked (Needed for cvars which are in a group but change in various situations)
    VF_RESETTABLE =                          0x10000000,         // This variable will be reset when a commanded reset to defaults occurs
    VF_RENDERER_CVAR =                      0x20000000,         // The update of this variable will be done in render thread
    VF_DEPRECATED =                             0x40000000,         // Deprecated cvars use default values which cannot be modified outside the code
    VF_EXPERIMENTAL =                           0x80000000,         // This variable is used by WIP or experimental feature
};

struct ICVarDumpSink
{
    // <interfuscator:shuffle>
    virtual ~ICVarDumpSink()= default;
    virtual void OnElementFound(ICVar* pCVar) = 0;
    // </interfuscator:shuffle>
};

struct IKeyBindDumpSink
{
    // <interfuscator:shuffle>
    virtual ~IKeyBindDumpSink()= default;
    virtual void OnKeyBindFound(const char* sBind, const char* sCommand) = 0;
    // </interfuscator:shuffle>
};

struct IOutputPrintSink
{
    // <interfuscator:shuffle>
    virtual ~IOutputPrintSink()= default;
    virtual void Print(const char* inszText) = 0;
    // </interfuscator:shuffle>
};

// Callback class to derive from when you want to receive callbacks when console var changes.
struct IConsoleVarSink
{
    // <interfuscator:shuffle>
    virtual ~IConsoleVarSink()= default;
    // Called by Console before changing console var value, to validate if var can be changed.
    // Return value: true if ok to change value, false if should not change value.
    virtual bool OnBeforeVarChange(ICVar* pVar, const char* sNewValue) = 0;
    // Called by Console after variable has changed value
    virtual void OnAfterVarChange(ICVar* pVar) = 0;
    // </interfuscator:shuffle>
};

// Interface to the arguments of the console command.
struct IConsoleCmdArgs
{
    // <interfuscator:shuffle>
    virtual ~IConsoleCmdArgs()= default;
    // Gets number of arguments supplied to the command (including the command itself)
    virtual int GetArgCount() const = 0;
    // Gets argument by index, nIndex must be in 0 <= nIndex < GetArgCount()
    virtual const char* GetArg(int nIndex) const = 0;
    // Gets complete command line
    virtual const char* GetCommandLine() const = 0;
    // </interfuscator:shuffle>
};

// Interface to the arguments of the console command.
struct IConsoleArgumentAutoComplete
{
    // <interfuscator:shuffle>
    virtual ~IConsoleArgumentAutoComplete()= default;
    // Gets number of matches for the argument to auto complete.
    virtual int GetCount() const = 0;
    // Gets argument value by index, nIndex must be in 0 <= nIndex < GetCount()
    virtual const char* GetValue(int nIndex) const = 0;
    // </interfuscator:shuffle>
};

// This a definition of the console command function that can be added to console with AddCommand.
using ConsoleCommandFunc = void (*)(IConsoleCmdArgs*);

// This a definition of the callback function that is called when variable change.
using ConsoleVarFunc = void (*)(ICVar*);

/* Summary: Interface to the engine console.

  Description:
    The engine console allow to manipulate the internal engine parameters
    and to invoke commands.
    This interface allow external modules to integrate their functionalities
    into the console as commands or variables.

    IMPLEMENTATIONS NOTES:
    The console takes advantage of the script engine to store the console variables,
    this mean that all variables visible through script and console.
*/
struct IConsole
{
    // <interfuscator:shuffle>
    virtual ~IConsole()= default;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Deletes the console
    virtual void Release() = 0;

    // Initialize this instance of engine console
    // Arguments:
    //   pSystem - an instance of ISystem
    virtual void Init(ISystem* pSystem) = 0;
    // Create a new console variable that store the value in a string
    // Arguments:
    //   sName - console variable name
    //   sValue - default value
    //   nFlags - user defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data)
    //   help - help text that is shown when you use <sName> ? in the console
    // Return:
    //   pointer to the interface ICVar
    virtual ICVar* RegisterString(const char* sName, const char* sValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = nullptr) = 0;
    // Create a new console variable that store the value in a int
    // Arguments:
    //   sName - console variable name
    //   iValue - default value
    //   nFlags - user defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data)
    //   help - help text that is shown when you use <sName> ? in the console
    // Return:
    //   pointer to the interface ICVar
    virtual ICVar* RegisterInt(const char* sName, int iValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = nullptr) = 0;
    // Create a new console variable that store the value in a float
    // Arguments:
    //   sName - console variable name
    //   fValue - default value
    //   nFlags - user defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data)
    //   help - help text that is shown when you use <sName> ? in the console
    // Return:
    //   pointer to the interface ICVar
    virtual ICVar* RegisterFloat(const char* sName, float fValue, int nFlags, const char* help = "", ConsoleVarFunc pChangeFunc = nullptr) = 0;

    // Create a new console variable that will update the user defined float
    // Arguments:
    //   sName - console variable name
    //   src - pointer to the memory that will be updated
    //   nFlags - user defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data)
    //   help - help text that is shown when you use <sName> ? in the console
    //   allowModify - allow modification through config vars, prevents missing modifications in release mode
    // Return:
    //   pointer to the interface ICVar
    virtual ICVar* Register(const char* name, float* src, float defaultvalue, int nFlags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = nullptr, bool allowModify = true) = 0;
    // Create a new console variable that will update the user defined integer
    // Arguments:
    //   sName - console variable name
    //   src - pointer to the memory that will be updated
    //   nFlags - user defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data)
    //   help - help text that is shown when you use <sName> ? in the console
    //   allowModify - allow modification through config vars, prevents missing modifications in release mode
    // Return:
    //   pointer to the interface ICVar
    virtual ICVar* Register(const char* name, int* src, int defaultvalue, int nFlags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = nullptr, bool allowModify = true) = 0;

    // Create a new console variable that will update the user defined pointer to null terminated string
    // Arguments:
    //   sName - console variable name
    //   src - pointer to the memory that will be updated
    //   nFlags - user defined flag, this parameter is used by other subsystems and doesn't affect the console variable (basically of user data)
    //   help - help text that is shown when you use <sName> ? in the console
    //   allowModify - allow modification through config vars, prevents missing modifications in release mode
    // Return:
    //   pointer to the interface ICVar
    virtual ICVar* Register(const char* name, const char** src, const char* defaultvalue, int nFlags = 0, const char* help = "", ConsoleVarFunc pChangeFunc = nullptr, bool allowModify = true) = 0;

    // ! Remove a variable from the console
    // @param sVarName console variable name
    // @param bDelete if true the variable is deleted
    // @see ICVar
    virtual void UnregisterVariable(const char* sVarName, bool bDelete = false) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Set the y coordinate where the console will stop to scroll when is dropped
    // @param value y in screen coordinates
    virtual void SetScrollMax(int value) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // add output sink (clases which are interested in the output) - order is not guaranteed
    // @param inpSink must not be 0 and is not allowed to be added twice
    virtual void AddOutputPrintSink(IOutputPrintSink* inpSink) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // remove output sink (clases which are interested in the output) - order is not guaranteed
    // @param inpSink must not be 0 and has to be added before
    virtual void RemoveOutputPrintSink(IOutputPrintSink* inpSink) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // show/hide the console
    // @param specifies if the window must be (true=show,false=hide)
    // @param specifies iRequestScrollMax <=0 if not used, otherwise it sets SetScrollMax temporary to the given value
    virtual void ShowConsole(bool show, int iRequestScrollMax = -1) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Dump all console-variables to a callback-interface
    // @param Callback callback-interface which needs to be called for each element
    virtual void DumpCVars(ICVarDumpSink* pCallback, unsigned int nFlagsFilter = 0) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Bind a console command to a key
    // @param sCmd console command that must be executed
    // @param sRes name of the key to invoke the command
    virtual void CreateKeyBind(const char* sCmd, const char* sRes) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Iterate through the lines - used for dedicated server (truncated if needed)
    // @param indwLineNo 0.. counted from the last printed line on
    // @param outszBuffer pointer to the destination string buffer (zero terminated afterwards), must not be 0
    // @param indwBufferSize 1.. size of the buffer
    // @return true=line was returned, false=there are no more lines

    virtual bool GetLineNo(int indwLineNo, char* outszBuffer, int indwBufferSize) const = 0;

    // @return current number of lines in the console
    virtual int GetLineCount() const = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Retrieve a console variable by name - not case sensitive
    // @param name variable name
    // @return a pointer to the ICVar interface, NULL if is not found
    // @see ICVar
    virtual ICVar* GetCVar(const char* name) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read a value from a configuration file (.ini) and return the value
    // @param szVarName variable name
    // @param szFileName source configuration file
    // @param def_val default value (if the variable is not found into the file)
    // @return the variable value
    virtual char* GetVariable(const char* szVarName, const char* szFileName, const char* def_val) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Read a value from a configuration file (.ini) and return the value
    // @param szVarName variable name
    // @param szFileName source configuration file
    // @param def_val default value (if the variable is not found into the file)
    // @return the variable value
    virtual float GetVariable(const char* szVarName, const char* szFileName, float def_val) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Print a string in the console and go to the new line
    // @param s the string to print
    virtual void PrintLine(AZStd::string_view s) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Append a string in the last console line
    // @param s the string to print
    virtual void PrintLineAppendWithPrevLine(AZStd::string_view s) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Retrieve the status of the console (active/not active)
    // @return the variable value(true = active/false = not active)
    virtual bool GetStatus() = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Clear the console text
    virtual void    Clear() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Update the console
    virtual void    Update() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Description:
    //    Register a new console command.
    // Arguments:
    //    sCommand - Command name.
    //    func     - Pointer to the console command function to be called when command is invoked.
    //    nFlags   - Bitfield consist of VF_ flags (e.g. VF_CHEAT)
    //    sHelp    - Help string, will be displayed when typing in console "command ?".
    // Return
    //    True if successful, false otherwise.
    virtual bool AddCommand(const char* sCommand, ConsoleCommandFunc func, int nFlags = 0, const char* sHelp = nullptr) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Description:
    //    Register a new console command that execute script function.
    //    EG "Game.Connect(%1)" the symbol "%1" will be replaced with the command parameter 1
    //    writing in the console "connect 127.0.0.1" will invoke Game.Connect("127.0.0.1")
    // Arguments:
    //    sCommand - Command name.
    //    sScriptFunc - Script function to be executed when command is invoked.
    //    nFlags   - Bitfield consist of VF_ flags (e.g. VF_CHEAT)
    //    sHelp    - Help string, will be displayed when typing in console "command ?".
    // Return
    //    True if successful, false otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual bool AddCommand(const char* sName, const char* sScriptFunc, int nFlags = 0, const char* sHelp = nullptr) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Description:
    //    Removes a console command which was previously registered with AddCommand.
    // Arguments:
    //    sCommand - Command name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual void RemoveCommand(const char* sName) = 0;

    // Execute a string in the console
    // Arguments:
    //   command - console command e.g. "map testy" - no leading slash
    //   bSilentMode - true=suppresses log in error case and logging the command to the console
    //   bDeferExecution - true=the command is stored in special fifo that allows delayed execution
    //               by using wait_seconds and wait_frames commands
    //
    virtual void ExecuteString(const char* command, bool bSilentMode = false, bool bDeferExecution = false) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Print a message into the log and abort the execution of the application
    // @param message error string to print in the log
    virtual void Exit(const char* command, ...) PRINTF_PARAMS(2, 3) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Return true if the console is opened
    // @return the variable value(true = opened/false = closed)
    virtual bool IsOpened() = 0;

    //////////////////////////////////////////////////////////////////////////
    // Auto completion.
    //////////////////////////////////////////////////////////////////////////
    virtual int     GetNumVars() = 0;

    virtual int     GetNumVisibleVars() = 0;

    // Arguments:
    //   szPrefix - 0 or prefix e.g. "sys_spec_"
    // Return
    //   used size
    virtual size_t GetSortedVars(AZStd::vector<AZStd::string_view>& pszArray, const char* szPrefix = nullptr) = 0;
    virtual const char* AutoComplete(const char* substr) = 0;
    virtual const char* AutoCompletePrev(const char* substr) = 0;
    virtual const char* ProcessCompletion(const char* szInputBuffer) = 0;
    virtual void RegisterAutoComplete(const char* sVarOrCommand, IConsoleArgumentAutoComplete* pArgAutoComplete) = 0;
    virtual void UnRegisterAutoComplete(const char* sVarOrCommand) = 0;
    //
    virtual void ResetAutoCompletion() = 0;
    //////////////////////////////////////////////////////////////////////////

    // Arguments:
    //   szLine - must not be 0
    virtual void SetInputLine(const char* szLine) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Dump all key bindings to a callback-interface
    // @param Callback callback-interface which needs to be called for each element
    virtual void DumpKeyBinds(IKeyBindDumpSink* pCallback) = 0;
    virtual const char* FindKeyBind(const char* sCmd) const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Console variable sink.
    // Adds a new console variables sink callback.
    virtual void AddConsoleVarSink(IConsoleVarSink* pSink) = 0;
    //! Removes a console variables sink callback.
    virtual void RemoveConsoleVarSink(IConsoleVarSink* pSink) = 0;

    //////////////////////////////////////////////////////////////////////////
    // History
    //////////////////////////////////////////////////////////////////////////

    // \param bUpOrDown true=after pressed "up", false=after pressed "down"
    // \return 0 if there is no history line or pointer to the null terminated history line
    virtual const char* GetHistoryElement(bool bUpOrDown) = 0;
    //! \param szCommand must not be 0
    virtual void AddCommandToHistory(const char* szCommand) = 0;

    //////////////////////////////////////////////////////////////////////////
    //
    virtual void LoadConfigVar(const char* sVariable, const char* sValue) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Enable or disable the activation key (tilde by default).
    // This is useful when user is in a text field and want to be able to enter the default key.
    // bEnable=true console will show
    // bEnable=false console will no show
    virtual void EnableActivationKey(bool bEnable) = 0;


    // </interfuscator:shuffle>

    virtual void SetClientDataProbeString(const char* pName, const char* pValue) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
//  This interface for the remote console
struct IRemoteConsoleListener
{
    virtual ~IRemoteConsoleListener() = default;

    virtual void OnConsoleCommand([[maybe_unused]] const char* cmd) {};
    virtual void OnGameplayCommand([[maybe_unused]] const char* cmd) {};
};

struct IRemoteConsole
{
    virtual ~IRemoteConsole() = default;;

    virtual void RegisterConsoleVariables() = 0;
    virtual void UnregisterConsoleVariables() = 0;

    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsStarted() const = 0;

    virtual void AddLogMessage(AZStd::string_view log) = 0;
    virtual void AddLogWarning(AZStd::string_view log) = 0;
    virtual void AddLogError(AZStd::string_view log) = 0;

    virtual void Update() = 0;

    virtual void RegisterListener(IRemoteConsoleListener* pListener, const char* name) = 0;
    virtual void UnregisterListener(IRemoteConsoleListener* pListener) = 0;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Description:
//  This interface is the 1:1 "C++ representation"
//  of a console variable.
// Note:
//  A console variable is accessible in C++ trough
//  this interface and in all scripts as global variable
//  (with the same name of the variable in the console)
struct ICVar
{
    enum EConsoleLogMode
    {
        eCLM_Off,                           // off
        eCLM_ConsoleAndFile,    // normal info to console and file
        eCLM_FileOnly,              // normal info to file only
        eCLM_FullInfo                   // full info to file only
    };

    // <interfuscator:shuffle>
    // TODO make protected;
    virtual ~ICVar() = default;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // delete the variable
    // NOTE: the variable will automatically unregister itself from the console
    virtual void Release() = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Return the integer value of the variable
    // @return the value
    virtual int GetIVal() const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Return the 64-bit integer value of the variable
    // @return the value
    virtual int64 GetI64Val() const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Return the float value of the variable
    // @return the value
    virtual float GetFVal() const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Return the string value of the variable, don't store pointer as multiple calls to this function might return same memory ptr
    // @return the value
    virtual const char* GetString() const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Return the data probe string value of the variable, don't store pointer as multiple calls to this function might return same memory ptr
    // @return the value
    virtual const char* GetDataProbeString() const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Reset the CVar to its default value if possible
    virtual void Reset() {}
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // set the string value of the variable
    // @param s string representation the value
    virtual void Set(const char* s) = 0;

    // Force to set the string value of the variable - can be called
    // from inside code only
    // @param s string representation the value
    virtual void ForceSet(const char* s) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // set the float value of the variable
    // @param s float representation the value
    virtual void Set(float f) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // set the float value of the variable
    // @param s integer representation the value
    virtual void Set(int i) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // clear the specified bits in the flag field
    virtual void ClearFlags (int flags) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // return the variable's flags
    // @return the variable's flags
    // See Also: EVarFlags
    virtual int GetFlags() const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Set the variable's flags
    // See Also: EVarFlags
    virtual int SetFlags(int flags) = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // return the primary variable's type
    // @return the primary variable's type  e.g. CVAR_INT, CVAR_FLOAT, CVAR_STRING
    virtual int GetType() = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // return the variable's name
    // @return the variable's name
    virtual const char* GetName() const = 0;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // return the variable's help text
    // @return the variable's help text, can be 0 if no help is available
    virtual const char* GetHelp() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // return if the variable may be modified in config files
    // @return if the variable may be modified in config files
    virtual bool IsConstCVar() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // Set a new on change function callback.
    // Deprecated function. The functor should be preferred.
    virtual void SetOnChangeCallback(ConsoleVarFunc pChangeFunc) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Adds a new on change functor to the list.
    // It will add from index 1 on (0 is reserved).
    // Returns an ID to use when getting or removing the functor
    virtual bool AddOnChangeFunctor(AZ::Name functorName, const AZStd::function<void()>& pChangeFunctor) = 0;

    //////////////////////////////////////////////////////////////////////////
    // Get the current callback function.
    virtual ConsoleVarFunc GetOnChangeCallback() const = 0;

    //////////////////////////////////////////////////////////////////////////
    // only useful for CVarGroups, other types return GetIVal()
    // CVarGroups set multiple other CVars and this function returns
    // the integer value the CVarGroup should have, when looking at the controlled cvars
    // Returns:
    //   value that would represent the state, -1 if the state cannot be found
    virtual int GetRealIVal() const = 0;

    virtual void SetLimits(float min, float max) = 0;
    virtual void GetLimits(float& min, float& max) = 0;
    virtual bool HasCustomLimits() = 0;

    // only useful for CVarGroups
    // log difference between expected state and real state
    virtual void DebugLog([[maybe_unused]] const int iExpectedValue, [[maybe_unused]] const EConsoleLogMode mode) const {}
    // </interfuscator:shuffle>

    // Set the data probe string value of the variable
    virtual void SetDataProbeString(const char* pDataProbeString) = 0;
};
