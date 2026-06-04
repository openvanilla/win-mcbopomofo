# Development Requirements

## 1. TDD Requirements

This project adopts TDD (Test-Driven Development) as the default development method.

When adding features, fixing behaviors, or adjusting state machines, please follow this sequence:

1. Write tests first.
2. Ensure the test describes the expected behavior and reproduces the failure before modification.
3. Then modify the production code.
4. Passing the test is the minimum standard for completing the change.

In other words:

- A feature without tests is not considered complete.
- A feature that does not pass its tests is not considered complete.

## 2. Test File Locations

All project-specific tests are located in the `tests/` directory.

Currently existing tests include:

- `tests/BugReproTest.cpp` - Reproduces specific bugs and regressions.
- `tests/InputMacroTest.cpp` - Tests input macros and text expansion.
- `tests/IpcTest.cpp` - Tests IPC serialization and deserialization payloads.
- `tests/PipeTest.cpp` - Validates named pipe connection and robust transfer logic.
- `tests/RegisterCategories.cpp` - Tests COM registration paths.
- `tests/test_candidate_window.cpp` - Tests the UI candidate window logic.
- `tests/test_cocreate.cpp` - Validates COM object instantiation.
- `tests/test_ipc_client.cpp` - Tests the IPC client mechanism.
- `tests/test_pipe.cpp` - Low-level pipe wrapper tests.
- `tests/test_server.cpp` - Server bootstrap tests.

When adding a new test:

- Place it in the `tests/` directory.
- Simultaneously update the root `CMakeLists.txt` to include the test target and `add_test(...)`.

## 3. Test Naming and Scope

It is recommended to split tests by function rather than piling a large number of unrelated cases into the same file.

For example:

- Candidate window display logic
- IPC encode/decode
- Specific bug reproductions
- State transitions

Each test should ideally verify only one specific behavior.

## 4. How to Run Tests

### Building a Single Test

In an existing build directory:

```powershell
cmake --build . --config Debug --target BugReproTest
```

Or:

```powershell
cmake --build . --config Debug --target CandidateWindowTest
cmake --build . --config Debug --target IpcTest
```

### Running a Single Test

```powershell
ctest -C Debug -R BugReproTest --output-on-failure
```

### Running Project-Specific Tests

```powershell
ctest -C Debug --output-on-failure
```

The current root CMake has intentionally disabled test registration for third-party dependencies (e.g., the OpenCC subproject), so this command should only run tests maintained by this project.

If `ctest` fails to resolve Windows executable paths correctly in certain environments, you can also run the test programs directly:

```powershell
.\build_verify\bin\Debug\CandidateWindowTest.exe
.\build_verify\bin\Debug\BugReproTest.exe
.\build_verify\bin\Debug\IpcTest.exe
```

## 5. Definition of Done

For a feature or fix to be considered complete, it must at least meet the following:

1. Corresponding tests exist.
2. Tests are located in the `tests/` directory.
3. Tests are added to CMake.
4. Tests pass in the local build.
5. If changes affect existing behavior, relevant old tests must also remain passing.

## 6. Recommended Practices

### When Adding Features

- Write a failing test first.
- Make the minimum necessary modifications.
- Finally, run the affected tests.

### When Fixing Bugs

- Reproduce the bug as a regression test.
- Confirm it fails before the fix.
- Confirm it passes after the fix.

### When Modifying the State Machine

- Prioritize adding:
    - State transition tests
    - Candidate paging / selection tests
    - Commit vs. composing behavior tests

These types of modifications, if verified only manually, are prone to re-introducing regression issues related to Notepad, candidate lists, and direct commits.
