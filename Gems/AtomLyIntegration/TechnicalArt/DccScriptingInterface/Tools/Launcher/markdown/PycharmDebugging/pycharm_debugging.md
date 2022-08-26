# PyCharm IDE And O3DE

The DCCsi provides support for three of the most popular Python IDEs- PyCharm, Visual Studio Code, and Wing. In order to effectively use the tools and utilities contained within the DCCsi system, the user's chosen IDE needs to be provided with information related to the DCCsi itself in order to initialize logging and environment variables. This set of instructions covers setup and usage of the PyCharm IDE with the DCCsi.

## Launch file setup

The first step in getting set up to use the PyCharm IDE is to locate the IDE launcher .BAT file titled "Launch_PyCharmPro.bat" using the location within your O3DE installation directory below:

![alt text](images/launcher_location.PNG)

If you don't have the PyCharm application installed already, you are going to need to get this installed before running the .BAT file. PyCharm is offered in two different versions- PyCharmPro and PyCharm Community. Although you can use the Community  version for some of the functionality offered in the DCCsi, there are limitations- particularly the lack of support for remote debugging. This set of instructions focuses on the Professional version. 

Once PyCharm Pro is installed and accessible, the first step is to ensure that the pathing in the .BAT file can reach your installed location. Once installed, PyCharm does not update the name of the base installation folder, and this is an issue that you will need to be aware of when using the DCCsi Launcher we provide. The .BAT file by default will look into your installed "Program Files" directory on your C: drive, but it is likely that the "Launch_PyCharmPro.bat" file will not be able to link to the right installed location. This can be remedied by setting the version number in the "Env_IDE_PyCharm.bat" file marked below (the second image shows the passage in the file where the version numbers need to be set in order for the application to be found).

Notice that the final image shows the directory name as it exists on disk, and how it correlates with the information contained second image. Once this information is set (and saved), try to double click the "Launch_PyCharmPro.bat". If the path is set correctly, PyCharm should launch.

![alt text](images/pycharm_env.PNG)

![alt text](images/pycharm_bat_path.PNG)

![alt text](images/installation_location.PNG)

The purpose of using the Env_IDE_PyCharm.bat file to launch PyCharm is that it initializes the DCCsi codebase itself, and facilitates access to the tools and utilities contained within. Once this process is complete, its time to set up remote debugging to your DCC package of choice. The two main DCC applications that the DCCsi currently lends support for are Maya and Blender. For the purposes of these instructions, the setup for remote debugging will be described for Maya 2022 and above.

## Remote Debugging Setup in Maya

The setup for developing and running scripts from PyCharm to an open session of Maya is fairly simple to achieve. Once you have PyCharm successfully launched using the "Launch_PyCharmPro.bat file", the next step is to launch Maya using the "Launch_Maya_2022.bat" file (location pictured below).

![alt text][images/maya_launcher_location![__](images/maya_launcher_location.PNG)

Again, using the .bat file to launch the application injects it with information necessary to communicate with the DCCsi codebase, but this time on the DCC Application side (in conjunction with the already open PyCharm IDE ALSO launched via supplied .bat file). Once you have both applications open by way of .bat file launcher, it's time to create a reusable debugging configuration from which to connect with an open Maya session in PyCharm.

## Creating the Remote Debugging Configuration in PyCharm

Located in the grouping of tools at the top right of the PyCharm window, you will see the debug tools (notice the green bug icon). For setting communication lines, we will need to next use the pulldown menu (circled below, currently listing "main"), and you are going to want to select the first entry in the expanded list- "Edit Configurations".

![[]](images/pycharm_configuration_01.PNG)

This will launch the Run/Debug Configurations window. In the upper left hand corner of the window you will see a "+"... click on this button to add a new configuration by selecting "Python Debug Server". In the window, match the settings to what you see in the image below. 

![[]](images/maya_debug_configuration.PNG)

If not auto populated, the "Path mappings" can be entered by clicking the directory icon on the far right of the text field- the paths should be adjusted to point to these paths (with the exception of replacing the repository path with your own repository location):

###### Local path

<O3DE Repository Path>/Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/Tools/Dev/Windows/Solutions

###### Remote path

<O3DE Repository Path>/ Gems/AtomLyIntegration/TechnicalArt/DccScriptingInterface/<maya_console>

"<maya_console>" should appear as typed, but the <O3DE Repository Path> should be replaced with YOUR O3DE Repository location to form the full path into the DCCsi.

You will also notice instructions directly in the window for opening the connection on the PyCharm side, and connecting on the Maya side. You will need to set the pycharm.egg file as it instructs so that on the Maya side you can run the import and settrace commands to it- establishing the link between applications, which will be guidelined next.

## Connecting Maya to PyCharm

This is the last step of the process! If you have successfully established the path to the pycharm egg file in the previous step, your Maya python should be able to see it, and successfully import the module without error. First, make sure that your configuration is set in the dropdown from the last step to the newly created "Maya Remote Debug" configuration. Once that is set, you are going to want to click the green bug icon to the right of that dropdown menu, telling pycharm that you want to start a new debug session. When you click this you will see the red square icon become active (this is the "stop MayaRemoteDebug button"). Also, if you active the "Debug" tab in the lower information console window, you should see the following message:

```
Starting debug server at port 9,001
Use the following code to connect to the debugger:
import pydevd_pycharm
pydevd_pycharm.settrace('localhost', port=9001, stdoutToServer=True, stderrToServer=True, suspend=False)
Waiting for process connection…
```

In the Maya script editor, you should be able to enter those provided lines and execute them. If you are set up properly, you will see an acknowledgement by PyCharm that the connection has been successfully established- it will look something like this:

```
Connected to pydev debugger (build 213.5744.248)
```

At this point you are all set! The process of debugging itself is outside of the scope of this instructional guide, but it is recommended if you are unfamiliar with setting breakpoints and using the debugging process to step through your code, checkout the JetBrains PyCharm documentation for a fairly comprehensive set of instructions.
