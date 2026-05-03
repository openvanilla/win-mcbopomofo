#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "CandidateWindow.h"

// Basic assertion macro for tests
#define ASSERT_EQ(expected, actual) \
    if ((expected) != (actual)) { \
        std::wcerr << L"Test Failed: Expected '" << (expected) << L"', got '" << (actual) << L"'\n"; \
        return 1; \
    }

int test_single_candidate() {
    CandidateWindow window;
    window.Create(GetModuleHandle(NULL));

    std::vector<std::string> candidates = {"A"}; // Use simple ascii for test to avoid encoding issues in console
    
    // UpdateUI with 1 candidate
    window.UpdateUI(candidates, 0, false);
    
    // Check if the formatted string is not empty and matches expected
    std::wstring result = window.GetDisplayString();
    ASSERT_EQ(L"1. A", result);

    std::cout << "test_single_candidate passed." << std::endl;
    return 0;
}

int test_multiple_candidates() {
    CandidateWindow window;
    window.Create(GetModuleHandle(NULL));

    std::vector<std::string> candidates = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    
    // Page 1
    window.UpdateUI(candidates, 0, false);
    std::wstring result1 = window.GetDisplayString();
    // It should have 9 items and page indicator
    std::wstring expected1 = L"1. 1   2. 2   3. 3   4. 4   5. 5   6. 6   7. 7   8. 8   9. 9  (1/2)";
    ASSERT_EQ(expected1, result1);

    // Page 2
    window.UpdateUI(candidates, 9, false);
    std::wstring result2 = window.GetDisplayString();
    std::wstring expected2 = L"1. 0  (2/2)";
    ASSERT_EQ(expected2, result2);

    std::cout << "test_multiple_candidates passed." << std::endl;
    return 0;
}

int test_single_candidate_on_second_page() {
    CandidateWindow window;
    window.Create(GetModuleHandle(NULL));

    std::vector<std::string> candidates = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    
    // Page 2: Cursor index 9. There is only "0" on this page.
    window.UpdateUI(candidates, 9, false);
    
    std::wstring result = window.GetDisplayString();
    std::wstring expected = L"1. 0  (2/2)";
    ASSERT_EQ(expected, result);

    std::cout << "test_single_candidate_on_second_page passed." << std::endl;
    return 0;
}

int main() {
    int failures = 0;
    
    CoInitialize(NULL);
    
    failures += test_single_candidate();
    failures += test_multiple_candidates();
    failures += test_single_candidate_on_second_page();

    if (failures == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << failures << " tests failed." << std::endl;
    }

    CoUninitialize();
    return failures;
}
