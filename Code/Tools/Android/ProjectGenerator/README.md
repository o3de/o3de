# About The Android Project Generator
This is a standalone tool that works as an extension of the O3DE ProjectManager.  
  
The purpose of this tool is to expedite and automate the creation of an Android Project
that can be compiled and built with Android Studio, or from the command line terminal
with the help of commands like `gradlew assembleProfile`.  
  
With this tool the user can easily create a Keystore file, and/or create an Android Project. 
  
This tool can be spawned manually from the terminal, but its usefulness becomes
more aparent when started from the ProjectManager per-project menu because the
input arguments (like `engine path`, `project path`, etc) required by `main.py`
are automatically inferred by using internal O3DE APIs.  
  
# How To Use The Android Project Generator

## TL;DR
1. Push the `Create Keystore` button (only required if the keystore file has not been created before).
2. Push the `Generate Project` button.
3. Open the project with Android Studio.
  
## More details. 
As soon as the tool is executed by the ProjectManager, the user will be presented
with a window. The window is made of 4 sections:
1. Options to load/save settings from/to user preferred locations on disk. The default settings will be stored
in a file with path `<Project>/apg_config.json`.  
2. The `Keystore Settings` section. In this section, the user customizes the arguments required to create an android keystore for signing the application.
3. The `Android SDK/NDK Settings` section. In this section, the user customizes the arguments required to generate the android project.
4. The `Operations Report` section. This is just a scrollable widget that shows all the applications that are being invoked by this tool, which can help the user visualize details of each command argument, etc.