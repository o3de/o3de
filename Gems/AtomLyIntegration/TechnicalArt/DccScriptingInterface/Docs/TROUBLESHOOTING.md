# DccScriptingInterface Troubleshooting

Are you having trouble with the DccScriptingInterface (DCCsi)?

This troubleshooting guide has some common steps you can take to diagnose (Windows only, other platforms not officially supported or tested yet.)

If you encounter bugs along the way, file them as a Github Issue:

[O3DE Bug Report](https://github.com/o3de/o3de/issues/new?assignees=&labels=needs-triage%2Cneeds-sig%2Ckind%2Fbug&template=bug_template.md&title=Bug+Report)

# DCCsi\Python.cmd

The DCCsi is located in a folder location similar to this (I have cloned the o3de engine source code to `C:\depot\o3de-dev`):

`C:\depot\o3de-dev\Gems\AtomLyIntegration\TechnicalArt\DccScriptingInterface`

In the root of the DCCsi are some utilities, one of them starts an O3DE Python Command Prompt in the local DCCsi environment context.

`DccScriptingInterface/python.cmd`

Open a new shell / terminal / command prompt in the DCCsi root

![](assets/img/troubleshooting/2023-03-17-17-09-09-image.png)

Try running

```shell
>.\python.cmd
```

You should see output similar to this

![](assets/img/troubleshooting/2023-03-17-17-10-26-image.png)

If any of the ENVARs printed out have paths that are missing or look suspect/incorrect, that would be an indication that you may need to do some configuration as a developer.

If you see this error, you need to first setup Python for O3DE itself:

```shell
Python not found in c:\path\to\o3de\python
Try running c:\path\to\o3de\python\get_python.bat first.
```

**Solution**:

1. Close the DCCsi Python Command Prompt

2. set up o3de Python by running c:\path\to\o3de\python\get_python.bat

3. Restart the DCCsi Python CMD (`dccsi\python.cmd`)

Now you should be able to validate the DCCsi Python packages can be imported

```shell
>import azpy
```

You should see result similar to the following...

![](assets/img/troubleshooting/2023-03-17-17-11-55-image.png)

If things look good at this point, we want to stop the Python interpreter. In the CMD type `exit()`

```shell
>>>exit()
```

Now that we have ensured that the core O3DE Python is functional, we can move on.

# DCCsi\Foundation.py

The DCCsi requires some python package dependencies, and comes with a script utility to help manage the install of these dependencies (the script uses pip).  At the root of the DCCsi is a `requirements.txt` file, this is the python standard for tracking dependencies.  

Because most DCC apps have their own managed Python and may actually be on a different version of Python, even when a app is given access to the DCCsi the framework and/or APIs (like azpy) will fail - because of the missing dependencies.

`foundation.py` remedies this, it will install the `requirements.txt` into a managed folder within the DCCsi that is also procedurally bootstrapped, this folder is tied to the version of Python for the target application.

The next step to troubleshooting, is to run `foundation.py`

Open a new shell / terminal in the DCCsi



```shell
>python foundation.py
```


