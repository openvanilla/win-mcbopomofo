#include <gtest/gtest.h>
#include <memory>
#include "InputController.h"
#include "KeyHandler.h"
#include "McBopomofoLM.h"
#include "UIInterface.h"

using namespace McBopomofo;

class MockUI : public UIInterface {
public:
    void Reset() override { resetCalled = true; }
    void CommitString(const std::string& text) override { committedString = text; }
    void Update(const IPC::StateUpdatePayload& state) override {
        lastState = state;
        updateCount++;
    }

    bool resetCalled = false;
    std::string committedString;
    IPC::StateUpdatePayload lastState;
    int updateCount = 0;
};

class DummyLocalizedStrings : public LocalizedStrings {
public:
    std::string cursorIsBetweenSyllables(const std::string&, const std::string&) override { return ""; }
    std::string bopomofoFontAnnotationModeTooltip(bool, bool) override { return ""; }
    std::string syllablesRequired(size_t) override { return ""; }
    std::string syllablesMaximum(size_t) override { return ""; }
    std::string phraseAlreadyExists() override { return ""; }
    std::string pressEnterToAddThePhrase() override { return ""; }
    std::string markedWithSyllablesAndStatus(const std::string&, const std::string&, const std::string& s) override { return s; }
    std::string markingNotAvailableInFontAnnotationMode() override { return ""; }
};

class DummyUserPhraseAdder : public UserPhraseAdder {
public:
    void addUserPhrase(const std::string_view&, const std::string_view&) override {}
    void removeUserPhrase(const std::string_view&, const std::string_view&) override {}
};

class BugReproTest : public ::testing::Test {
protected:
    void SetUp() override {
        lm = std::make_shared<McBopomofoLM>();
        // We don't necessarily need to load a full LM for these state-based bugs,
        // unless it's needed for the converter.
        
        keyHandler = std::make_shared<KeyHandler>(
            lm, nullptr, std::make_shared<DummyUserPhraseAdder>(), 
            std::make_unique<DummyLocalizedStrings>()
        );
        ui = std::make_unique<MockUI>();
        controller = std::make_unique<InputController>(keyHandler, ui.get());
        keyHandler->setChineseConversionEnabled(true);
    }

    std::shared_ptr<McBopomofoLM> lm;
    std::shared_ptr<KeyHandler> keyHandler;
    std::unique_ptr<MockUI> ui;
    std::unique_ptr<InputController> controller;
};

TEST_F(BugReproTest, JumpToBig5State) {
    // Manually trigger Ctrl+backslash then select Big5 (index 0)
    controller->HandleKey(Key::asciiKey('\\', false, true));
    
    // Check if it's in SelectingFeature state
    ASSERT_EQ(ui->lastState.composingBuffer, "");
    ASSERT_GE(ui->lastState.candidates.size(), 4u);
    ASSERT_EQ(ui->lastState.candidates[0], "Big5 輸入");
    
    // Select Big5 (first feature)
    controller->SelectCandidate(0);
    
    // Now it should be in Big5 state
    EXPECT_EQ(ui->lastState.composingBuffer, "[Big5碼] ");
}

TEST_F(BugReproTest, JumpToIrohaState) {
    controller->HandleKey(Key::asciiKey('\\', false, true));
    
    // Select Iroha (fourth feature, index 3)
    controller->SelectCandidate(3);
    
    EXPECT_EQ(ui->lastState.composingBuffer, "[伊呂波] ");
}

TEST_F(BugReproTest, SelectingDateMacroCrashRepro) {
    controller->HandleKey(Key::asciiKey('\\', false, true));
    
    // Select Date/Time (index 1)
    controller->SelectCandidate(1);
    
    // Should be in SelectingDateMacro state
    ASSERT_GT(ui->lastState.candidates.size(), 0u);
    
    // Select the first candidate (Today Short)
    // This is where it's reported to crash.
    ASSERT_NO_THROW(controller->SelectCandidate(0));
    
    // It should have committed something
    EXPECT_FALSE(ui->committedString.empty());
    // Since ui->Update is not called for Empty state (ui->Reset is called instead),
    // we check ui->resetCalled.
    EXPECT_TRUE(ui->resetCalled);
}

TEST_F(BugReproTest, SpacePagesSelectingDateMacroCandidates) {
    ASSERT_TRUE(controller->HandleKey(Key::asciiKey('\\', false, true)));
    ASSERT_EQ(ui->lastState.candidates[1], "日期與時間");
    controller->SelectCandidate(1);  // Date/Time

    ASSERT_GT(static_cast<int>(ui->lastState.candidates.size()), 9);
    ASSERT_EQ(controller->GetCandidateIndex(), 0);

    int previousUpdateCount = ui->updateCount;
    EXPECT_TRUE(controller->HandleKey(Key::asciiKey(Key::SPACE, false, false)));
    EXPECT_EQ(controller->GetCandidateIndex(), 9);
    EXPECT_GT(ui->updateCount, previousUpdateCount);
    EXPECT_GT(static_cast<int>(ui->lastState.candidates.size()), 9);
}

TEST_F(BugReproTest, EscInSelectingFeatureReturnsToEmpty) {
    controller->HandleKey(Key::asciiKey('\\', false, true));
    ASSERT_EQ(ui->lastState.candidates[0], "Big5 輸入");
    
    controller->HandleKey(Key::asciiKey(Key::ESC, false, false));
    
    // It should be back to Empty state.
    // In InputController, Empty state results in ui->Reset() and currentState_ being Empty.
    // In our MockUI, we check resetCalled.
    EXPECT_TRUE(ui->resetCalled);
    EXPECT_NE(dynamic_cast<InputStates::Empty*>(controller->GetCurrentState()), nullptr);
}

TEST_F(BugReproTest, BackspaceInSelectingFeatureReturnsToEmpty) {
    ui->resetCalled = false; // Reset for this test
    controller->HandleKey(Key::asciiKey('\\', false, true));
    ASSERT_EQ(ui->lastState.candidates[0], "Big5 輸入");
    
    controller->HandleKey(Key::asciiKey(Key::BACKSPACE, false, false));
    
    EXPECT_TRUE(ui->resetCalled);
    EXPECT_NE(dynamic_cast<InputStates::Empty*>(controller->GetCurrentState()), nullptr);
}

TEST_F(BugReproTest, ShiftKeyAssociatedPhrasesDismissOnNonSelectionKey) {
    controller->SetStateForTesting(
        std::make_unique<InputStates::AssociatedPhrases>(
            std::make_unique<InputStates::Inputting>("", 0), 0, "ㄇㄧㄥˊ", "名",
            0,
            std::vector<InputStates::ChoosingCandidate::Candidate>{
                {"ㄇㄧㄥˊ-ㄘˊ", "名詞", "名詞"}},
            true),
        0);

    EXPECT_TRUE(controller->HandleKey(Key::asciiKey(Key::BACKSPACE, false, false)));
    EXPECT_NE(dynamic_cast<InputStates::Inputting*>(controller->GetCurrentState()),
              nullptr);
    EXPECT_EQ(
        dynamic_cast<InputStates::AssociatedPhrases*>(controller->GetCurrentState()),
        nullptr);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
