import sys
import json


def get_python_version(qprocess_query=True):
    """! This script is intended to be ran in a QProcess to retrieve the version information of the
    Python executable launched by the process. By default it is set to dump values to stdout, but can
    and is still used to get the Python version information of the main Python version being used to
    launch the DCCsi


    """
    data = sys.version_info
    version_info = {
        'major': data.major,
        'minor': data.minor,
        'micro': data.micro
    }
    if qprocess_query:
        json.dump(version_info, sys.stdout)
    else:
        return version_info

