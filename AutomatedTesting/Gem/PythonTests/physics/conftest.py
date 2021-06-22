import pytest

pytest_plugins = ["pytester"]

# Editor test files collection
def pytest_pycollect_makeitem(collector, name, obj):
    import inspect
    if inspect.isclass(obj):
        for base in obj.__bases__:
            if hasattr(base, "pytest_custom_makeitem"):
                return base.pytest_custom_makeitem(collector, name, obj)


def pytest_addoption(parser):
    parser.addoption("--no-editor-batch", action="store_false", help="Don't batch multiple tests in single editor")
    parser.addoption("--no-editor-parallel", action="store_false", help="Don't run multiple editors in parallel")
    parser.addoption("--parallel-editors", action="store", help="Override the number editors to run at the same time")
