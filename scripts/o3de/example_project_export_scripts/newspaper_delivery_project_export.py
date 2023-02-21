import sys
# import o3de.packaging as pkg
from o3de.packaging import *
import o3de.manifest as man
import o3de.project_manager_interface as pm

#from other_local_script import *  #our script can import from both o3de CLI and from the local folder it resides in

#this is a simple linear script

#pros for basic python scripting approach
# very simple to setup
# data oriented (one workflow per python script)
# if API is minimal, it is readily accessible, and function arguments can be provided by user
# both the script's local directory and O3DE CLI are accessible for import logic

#drawbacks
# requires user to code in python (we have customers that would rather not)
# it can be daunting as a blank slate. coding patterns are not standardized/enforceable, and is up to user to code out correctly
# can't do pre-validation of workflow
# lack of step annotation
# because the entire workflow is python code, it is non-trivial to parse (and with custom team-code, nigh impossible), and we won't be able to enforce data patterns
    # from CLI perspective this is not a big deal, but for ProjectManager UI, this can impede our ability to provide meaningful UX feedback or enable automated auditing
# too much more than what is actually needed for project export use case?


package_hi()
print("Now running the packaging code")


#apparently our module script can be pre-injected with known variable values!!!
#this comes from o3de/packaging.py
print(o3de_project_path)



package_windows_standalone_monolithic(project_path= o3de_project_path,
                                          zipped = False)



#before doing full compare contrast, get the json version of this working


