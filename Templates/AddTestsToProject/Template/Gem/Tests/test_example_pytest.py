import unittest.mock as mock
import pytest

# This line is here to prove that you can import things from ly_test_tools or other parts
# of the o3de toolkit to help you make tests.  See the documentation as well as existing tests
# in the gems and in the o3de repository (especially AutomatedTesting project) for examples on how
# to run things inside the editor, etc.
import ly_remote_console.remote_console_commands as remote_console

# The rest is standard pytest functionality, you can write tests here as you would in pytest.
@pytest.mark.unit
class TestExample():
    def test_ExampleTest(self):
        assert True

