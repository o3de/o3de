TestRailImporter
======
### Summary
This tool will take an .xml test report file, parse it for results, and upload the results to your TestRail site of choice after you supply some command line arguments to it.

### Requirements
* TestRailImporter currently only supports Windows OS with Python 2.
* There are future plans to add support for Mac & Linux as well as Python 3.

### Setup
* You will need the Python 3 version installed that ships with Lumberyard tools. It should be located at `~/path/to/Lumberyard/dev/python/python.cmd` - if it isn't you may need to reinstall Lumberyard and/or run CMAKE to fetch it.
* You will need a TestRail license and a TestRail site for updating test results: https://www.gurock.com/testrail
* A jUnitXML .xml report that can be parsed by `testrail_report_converter.py`. Officially we support `pytest` jUnitXML .xml file reports, but you can modify it as you see fit for your own reports.

### Usage
*Assuming you finished 'Setup' above:*

* **Recommended:** Navigate to the root directory for TestRailImporter and execute the following commands:
`cd ~/path/to/dev/Tools/TestRailImporter/`
`testrail_importer.cmd --testrail-xml=C:/path/to/xml.xml --testrail-user=user@amazon.com --testrail-project-id=1 --testrail-url=https://testrail.com/`
* **Alternative:** If you want to use your own Python version and not the one packaged with Lumberyard, you can call the script directly:
`~path/to/your/python.cmd ~path/to/dev/Tools/TestRailImporter/lib/testrail_importer/testrail_importer.py --testrail-xml=C:/path/to/xml.xml --testrail-user=user@amazon.com --testrail-project-id=1 --testrail-url=https://testrail.com/`

**NOTE:** *If you call the script directly using the **Alternative** option, you will have to configure your Python version to have the required packages for running this script. Reliable usage is only guaranteed with the **Recommended** method of executing the script, so please use that option if possible.*

   **CLI arguments:**
   * `--testrail-xml` - **REQUIRED**: This is the file path to the .xml report file to upload to TestRail.
   * `--testrail-url` - **REQUIRED**: This is the URL to the TestRail site you wish to upload the report to.
   * `--testrail-user` - **REQUIRED**: The username of the TestRail account to use.
   * `--testrail-password` - **REQUIRED for automation:** Password to use with the current `--testrail-user` arg. If the script is ran manually, tty can be used to input the password instead in the terminal or console. Be careful when doing this because it will expose the password in that terminal/console, so make sure to turn @echo off.`*`
   * `--testrail-project-id` - **REQUIRED**: Project ID of the TestRail project to upload results to.`*`
   * `--testrun-id` - **OPTIONAL** Test Run ID of the TestRail project to upload results to (Defaults to "Automated TestRail Report").`*`
   * `--testrail-suite-id` - **REQUIRED IF NO `--testrun-id` arg SUPPLIED** TestRail suite ID for the location of the current test cases.`*`
   * `--testrun-name` - **OPTIONAL** Name given to the report when viewed on the TestRail project's Test Run list.`*`
   * `--logs-folder` - **OPTIONAL** The name of the sub-folder inside `~/path/to/dev/Tools/TestRailImporter/` to store logging files (Defaults to `reports`).
      * *Items marked with an asterisk (`*`) deal with the TestRail API. For more info on the TestRail API see: http://docs.gurock.com/testrail-api2/introduction

### Unit tests
* Unit tests can be run using the following command:
`cd ~/path/to/dev/Tools/TestRailImporter/`
`testrail_importer.cmd --unit-tests`
