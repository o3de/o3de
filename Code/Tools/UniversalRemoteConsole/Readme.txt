Universal Remote Console
------------------------

Important: for the tool to work properly, the configuration files
* filters.xml
* params.xml

need to be in the same directoy where the .exe is found.

The build process will copy these files in the correct directory. In case you accidentally delete them, 
you can find these files in RemoteConsole/config-files.
Feel free to modify this files to your requirements.


================================ Overview  ======================================

In a nutshell, this tool allows you to:
* connect remotely to the game (multiple platforms) and the editor
* process and filter the game's console output
* execute a macro or OS command when certain log is received
* execute CVars and macros, take videos, snapshots, ...
* process (offline) existing log files (drag & drop) [new!]
* modify in-game parameters while the game is running using sliders
* MIDI input devices supported (KORG at the moment) to execute macros and sliders
* MIDI input has the extra advantage of not requiring a change of focus in the application
* fully customisable via XML files that you can change the tool parameters on-the-fly.
* get some statistics

Any comments/questions/requests, just let me know (Dario [dariop])

================ Filter Configuration  Quick Guide =======================================
 File: filters.xml
 
It needs to be located in the SAME DIRECTORY as the application.

 This file allows you to define your own filters and associated accions. 
 Feel free to add/remove/modify the contents of this file to suit your needs.
 The initial content is intended to be an example of usage.
 
 How does this work?
 
 * <Filter Name="Example">
   This attribute is used to specify the filter. In this example, any log that
   contains the word "Example" will be added to this filter's tab.
   
 * <Label>My Example</Label>
   This parameter is optional. If included it will be used to label the filter Tab.
   Otherwise, the "Name" attribute in Filter will be used.
   
 * <Color>FF0000</Color>
   Optional. Specifies the color of the text in the filter. Format R8G8B8.
   
 * <RegExp>\!(\w*)\]</RegExp>
   Optional. Specifies a regular expression to be used as filter. The given example
   would would added to this filter's tab any log that contains something of the 
   kind "...!....]", i.e. has an exclamation mark and at certain point later a "]"
   
 * <Exec Type="DosCmd">dir c:</Exec>
    Optional. Specifies an action to be taken if a particular filter is activated.
     It can be used for instance to trigger a snapshot or a video when certain log 
     message is sent (e.g. a debugging message).
     It can be very useful for debugging and QA.
     There are two types of actions that can be executed:
     + <Exec Type="Macro">ScreenShot</Exec>
        Executes a Macro (in this case ScreenShot, defined in this file)
     + <Exec Type="DosCmd">dir c:</Exec>
        Executes a dos command (in this case "dir c:")

================ Menu Configuration  Quick Guide =======================================
 File: params.xml
 
It needs to be located in the SAME DIRECTORY as the application.

This file defines the menu of the applciation. You can add:
* Targets (IPs of the devices you want to connect to)
* Macros (set of CVars you want to sent at once to the game)
* GamePlay-specific commands (e.g. SetViewMode:FlyOn)
* Buttons (Macros that are places as separate buttons)
* Sliders (Macros whose parameters change with the slider value)

You can assign MIDI codes to each entry so they can be controlled via an external MIDI input.

All this data will become part of your menu so you can customize what you really need.


... and some more stuff to be discovered :-)

Cheers
Dario