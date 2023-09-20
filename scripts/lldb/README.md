# LLDB Formatters for O3DE

This directory structure contains lldb formatter for O3DE.

It contains an `lldb/.lldbinit` file which is used to source the o3de lldb python scripts to allow for a user's environment.
The other files are python scripts that uses the [LLDB Python api](https://lldb.llvm.org/use/python-reference.html) to add support for O3DE specific container classes.


## Implemented formatters
The following types have implemented formatters and the files where their implementations can be found in

| Types | Formatter Python Script |
|------|------|
|AZStd::basic_string<.*>| lldb/.lldb/o3destd_lldb.py|


## How to use

In order to use the lldb formatters, they need to be sourced into the lldb debugger.
This can be done multiple ways
1. Manually using the lldb script import command
    *  `command script import <path-to-o3destd-py>`
1. As part of an `~/.lldbinit` file that is placed in the user's home directory
    | OS | Path |
    | --- | --- |
    | Linux | `/home/<user>` |
    | MacOS | `/Users/<user>` |
    | Windows | `C:/Users/<user>` |

For more information on how to source an LLDB settings and commands into the debugger, follow the LLDB man page [Configuration Files](https://lldb.llvm.org/man/lldb.html#configuration-files)

The directory structure containing this readme setup to allow source the lldb commands by copying the contents of the lldb folder into the user's home directory.  
**NOTE**: The contents of the lldb folder should be copied and not the the folder itself.  
i.e copy the ".lldbinit" file inside of the "lldb" folder to your home directory `~/` and not "lldb" folder.

### Structure of Home Directory after copying O3DE lldb python formatters
```
~/
    .lldbinit
    .lldb/
        o3destd_lldb.py
```

### Final Notes

* If you as a user already have other custom python formatters for use with lldb, then the lldb `command script import <path-to-o3destd-py>` command can be used to source the O3DE formatters into your lldb instance.
* On Linux/MacOS the `.lldbinit` file and `.lldb` directory are hidden due to starting with a \<dot>, so in order to view them in a file manager, it's hidden files option must be used. Also if copying from a file manager, the hidden files can't be selected without being visible.
