Base instructions for building crashpad live at https://chromium.googlesource.com/crashpad/crashpad/+/HEAD/doc/developing.md

This document is intended to cover changes from the base instructions for building Lumberyard's crashpad libraries.

depot_tools runs its own copy of the git client which is not granted a Cylance exclusion so you may run into issues and need to do manual git pulls (I did).

Lumberyard currently builds a 2013 and 2015 version of Crashpad.  Crashpad stopped supporting 2013 in December 2016 so it is necessary to checkout an earlier version for this build.  To get the correct build:

Crashpad clone - git fetch crashpad
Crashpad sync to "good" december version for 2013 - git checkout 556c4e

Crashpad has its own set of 3rdParty libraries which are handled by depot_tools - in the case of mini_chromium it is necessary to sync to the same checkout as above when building for 2013.

Clone mini_chromium to 2013 version:
delete third_party/mini_chromium/mini_chromium/build/common 
git clone https://chromium.googlesource.com/chromium/mini_chromium
Sync to good (pre december) Cl - (from within mini_chromium dir) git checkout ca7f42a

Make sure to copy over third_party/mini_chromium/mini_chromium.gyp
make .gypi changes from mini_chromium/mini_chromium/build and crashpad/build

RuntimeLibraries and ITERATOR_DEBUG settings need to match with Lumberyard Settings:

Added to build\crashpad.gypi

    'configurations': {
        'Debug': {
            'msvs_settings': {
                'VCCLCompilerTool' : {
                    'RuntimeLibrary': '3' /MDd
                }
            }
        },
        'Debug_x64': {
            'msvs_settings': {
                'VCCLCompilerTool' : {
                    'RuntimeLibrary': '3' /MDd
                }
            }
        },
        'Release': {
            'msvs_settings': {
                'VCCLCompilerTool' : {
                    'RuntimeLibrary': '2' /MD
                }
            },
            'defines': [
              '_ITERATOR_DEBUG_LEVEL=0'
            ]
        },
        'Release_x64': {
            'msvs_settings': {
                'VCCLCompilerTool' : {
                    'RuntimeLibrary': '2' /MD
                }
            },
            'defines': [
              '_ITERATOR_DEBUG_LEVEL=0'
          
    }
    
mini_chromium base settings live in third_party/mini_chromium/mini_chromium/build/common.gypi - switch RuntimeLibrary in debug from 1 to 3 there

The ninja build system doesn't seem to have a great way to force a full rebuild (Their stated goal is to build as little as possible as quickly as possible), so when from time to time you find you need to force a full rebuild:
Cleaning (from crashpad/crashpad):
del /s /q *.obj
del /s /q *.lib

I made minor modifications to the crash report uploader so a confirmation dialog could be displayed to the user.  The file is checked in perforce as dev/Tools/Crashpad/handler/src/crash_report_upload_thread.cc which should be copied over the existing copy from git which lives in crashpad\handler or the changes can be merged in if necessary.  The new block is in ProcessPendingReport and currently looks like:

// Amazon - Handle giving the user the option of whether or not to send the report.
#if defined(OS_WIN)
  std::wstring sendDialogMessage{ L"Lumberyard has encountered a fatal error.  We're sorry for the inconvenience.\n\nA Lumberyard Editor crash debugging file has been created at:\n" };
  sendDialogMessage += report.file_path.value();
  sendDialogMessage += L"\n\nIf you are willing to submit this file to Amazon it will help us improve the Lumberyard experience.  We will treat this report as confidential.\n\nWould you like to send the error report?";

  int msgboxID = MessageBox(
      NULL,
      sendDialogMessage.data(),
      L"Send Error Report",
      (MB_ICONEXCLAMATION | MB_YESNO | MB_SYSTEMMODAL)
      );

  if (msgboxID == IDNO)
  {
      database_->SkipReportUpload(report.uuid,
          Metrics::CrashSkippedReason::kUploadsDisabled);
      database_->DeleteReport(report.uuid);
      return;
  }
#endif

Currently we build the Debug_x64 and Release_x64 variant for both 2015 and 2013.
Build output goes to out\<buildtypefromabove>\obj\

The libraries we use are:

crashpad_client.lib from client (Also include crashpad_client.cc.pdb)
base.lib from third_party\mini_chromium\mini_chromium\base (and base.cc.pdb)
crashpad_util.lib from util (and crashpad_util.cc.pdb)

All sit together in dev\Tools\Crashpad\bin\windows\vs2015\Debug_x64 (Or whichever is the corresponding compiler/build target you're making)

Each have some include headers that need to go into dev\Tools\Crashpad\include - there are about 15 or so total and hopefully don't need changing from the time of this writing, but if you build with a newer version of Crashpad for 2015 than I did (which was around April 1 2017) you may need to make some updates.  

The 2013 headers have their own directory at Crashpad\include\vs2013.

Then the handler exe is crashpad_handler.exe which goes in dev\tools\Crashpad\handler.  We only need one of those, I used the Release_x64 variant for 2015.

Use the python in (root)/dev/python to do building.