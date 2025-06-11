# Contribution guidelines

This repository stores the source code for TigerVNC.

## Coding style
* C and C++ source files use two spaces for indentation. Do not use tabs.
* Wrap lines at 100 columns.
* Tests use the C++17 standard while other code targets C++11.
## Documentation
Place documentation updates in README.rst or under the doc directory as appropriate.
Wrap paragraphs at 72 columns.

## Building and testing
Run the unit tests after modifying any source files:

```
cmake -S . -B build
cmake --build build
ctest --test-dir tests/unit
```

Make a best effort to ensure the test suite passes. If tests fail due to missing
dependencies you may note this in the PR.
You may skip the tests when updating documentation or other non-source files.

## Commit messages
* Start with a one line summary of 50 characters or less.
* Leave a blank line then describe the change in more detail with lines wrapped
  at 72 columns.

## Pull request message
Include two sections named **Summary** and **Testing** describing what was
changed and how the tests were executed. Reference relevant files and test
output.

## Workflow
Work directly on the main branch and avoid rewriting history once a pull request is opened.
