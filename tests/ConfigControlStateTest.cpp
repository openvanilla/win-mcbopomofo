#include <windows.h>

#include <gtest/gtest.h>

#include "ControlState.h"

namespace {

class ConfigControlStateTest : public testing::Test {
 protected:
  void SetUp() override {
    parent_ = CreateWindowExW(0, L"Static", L"", WS_OVERLAPPED, 0, 0, 200, 100,
                              nullptr, nullptr, GetModuleHandleW(nullptr),
                              nullptr);
    ASSERT_NE(parent_, nullptr);
  }

  void TearDown() override {
    if (parent_) {
      DestroyWindow(parent_);
      parent_ = nullptr;
    }
  }

  HWND CreateButton(DWORD style) {
    HWND button = CreateWindowExW(
        0, L"Button", L"Test", WS_CHILD | style, 0, 0, 100, 24, parent_,
        nullptr, GetModuleHandleW(nullptr), nullptr);
    EXPECT_NE(button, nullptr);
    return button;
  }

 private:
  HWND parent_ = nullptr;
};

}  // namespace

TEST_F(ConfigControlStateTest, StandardCheckboxUsesNativeCheckState) {
  HWND checkbox = CreateButton(BS_AUTOCHECKBOX);
  ASSERT_NE(checkbox, nullptr);

  McBopomofo::ConfigApp::SetButtonChecked(checkbox, true, false);
  EXPECT_TRUE(McBopomofo::ConfigApp::IsButtonChecked(checkbox, false));
  EXPECT_EQ(SendMessageW(checkbox, BM_GETCHECK, 0, 0), BST_CHECKED);

  McBopomofo::ConfigApp::SetButtonChecked(checkbox, false, false);
  EXPECT_FALSE(McBopomofo::ConfigApp::IsButtonChecked(checkbox, false));
  EXPECT_EQ(SendMessageW(checkbox, BM_GETCHECK, 0, 0), BST_UNCHECKED);
}

TEST_F(ConfigControlStateTest, OwnerDrawCheckboxUsesExplicitState) {
  HWND checkbox = CreateButton(BS_OWNERDRAW);
  ASSERT_NE(checkbox, nullptr);

  McBopomofo::ConfigApp::SetButtonChecked(checkbox, true, true);
  EXPECT_TRUE(McBopomofo::ConfigApp::IsButtonChecked(checkbox, true));
  EXPECT_NE(GetWindowLongPtrW(checkbox, GWLP_USERDATA), 0);

  McBopomofo::ConfigApp::SetButtonChecked(checkbox, false, true);
  EXPECT_FALSE(McBopomofo::ConfigApp::IsButtonChecked(checkbox, true));
  EXPECT_EQ(GetWindowLongPtrW(checkbox, GWLP_USERDATA), 0);
}

TEST_F(ConfigControlStateTest, OwnerDrawCheckboxTogglesExplicitState) {
  HWND checkbox = CreateButton(BS_OWNERDRAW);
  ASSERT_NE(checkbox, nullptr);

  EXPECT_FALSE(McBopomofo::ConfigApp::IsButtonChecked(checkbox, true));
  McBopomofo::ConfigApp::ToggleButtonChecked(checkbox, true);
  EXPECT_TRUE(McBopomofo::ConfigApp::IsButtonChecked(checkbox, true));
  McBopomofo::ConfigApp::ToggleButtonChecked(checkbox, true);
  EXPECT_FALSE(McBopomofo::ConfigApp::IsButtonChecked(checkbox, true));
}

TEST_F(ConfigControlStateTest, OwnerDrawRadioUsesExplicitStateForSaveReads) {
  HWND radio = CreateButton(BS_OWNERDRAW);
  ASSERT_NE(radio, nullptr);

  McBopomofo::ConfigApp::SetButtonChecked(radio, true, true);
  bool valueReadDuringSave =
      McBopomofo::ConfigApp::IsButtonChecked(radio, true);

  EXPECT_TRUE(valueReadDuringSave);
}
