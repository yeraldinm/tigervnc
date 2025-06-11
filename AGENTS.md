# Contribution guidelines

This repository stores the source code for TigerVNC.

## Coding style
* C and C++ source files use two spaces for indentation. Do not use tabs.
* Wrap lines at 100 columns.
* Tests use the C++17 standard while other code targets C++11.

## Building and testing
Run the unit tests after modifying any source files:

```
cmake -S . -B build
cmake --build build
ctest --test-dir tests/unit
```

Make a best effort to ensure the test suite passes. If tests fail due to missing
dependencies you may note this in the PR.

## Commit messages
* Start with a one line summary of 50 characters or less.
* Leave a blank line then describe the change in more detail with lines wrapped
  at 72 columns.

## Pull request message
Include two sections named **Summary** and **Testing** describing what was
changed and how the tests were executed. Reference relevant files and test
output.
