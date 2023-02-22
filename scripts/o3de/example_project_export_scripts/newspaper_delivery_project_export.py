import o3de.package as pkg
from lol import *  #our script can import from both o3de CLI and from the local folder it resides in

#this is a simple linear script, as executed from a json workflow



pkg.package_hi()
print("Now running the packaging code")


#apparently our module script can be pre-injected with known variable values!!!
#this comes from o3de/packaging.py
print(o3de_project_path)



pkg.package_windows_standalone_monolithic(project_path= o3de_project_path,
                                      engine_path = o3de_engine_path,
                                          zipped = False)



#before doing full compare contrast, get the json version of this working
lol_hi()

