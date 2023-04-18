pushd %~dp0
../../python/python.cmd -s -B -m pytest -v --tb=short --show-capture=stdout -c ../../pytest.ini --build-directory ../../AutomatedTesting/build/bin/profile ../../AutomatedTesting/Gem/PythonTests/Atom/TestSuite_Periodic_GPU.py --output-path ../../AutomatedTesting/build/Testing/LyTestTools/AutomatedTesting_Atom_TestSuite_Periodic_GPU --junitxml=../../AutomatedTesting/build/Testing/Pytest/AutomatedTesting_Atom_TestSuite_Periodic_GPU.xml
popd
