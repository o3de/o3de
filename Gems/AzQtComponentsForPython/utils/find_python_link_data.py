import os, re, sys
from distutils import sysconfig

python_version = 37
python_long_version = 3.7
  
def is_debug():
    import importlib.machinery
    debug_suffix = '_d.pyd' if sys.platform == 'win32' else '_d.so'
    return any([s.endswith(debug_suffix) for s in importlib.machinery.EXTENSION_SUFFIXES])
    
#def python_link_data():
libdir = sysconfig.get_config_var('LIBDIR')
if libdir is None:
    libdir = os.path.abspath(os.path.join(
        sysconfig.get_config_var('LIBDEST'), "..", "libs"))

#flags = {}
#flags['libdir'] = libdir
if sys.platform == 'win32':
    suffix = '_d' if is_debug() else ''
    lib = 'python{}{}'.format(python_version, suffix)

elif sys.platform == 'darwin':
    lib = 'python{}'.format(version)

# Linux and anything else
else:
    if sys.version_info[0] < 3:
        suffix = '_d' if is_debug() else ''
        lib = 'python{}{}'.format(python_long_version, suffix)
    else:
        lib = 'python{}{}'.format(python_long_version, sys.abiflags)

    

lib = re.sub(r'.dll$', '.lib', lib)
result = '{};{}'.format(libdir, lib)
print(result)