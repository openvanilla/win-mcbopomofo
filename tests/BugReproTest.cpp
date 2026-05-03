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
    void Update(InputState* state) override { 
        lastState = state; 
        updateCount++;
    }

    bool resetCalled = false;
    std::string committedString;
    InputState* lastState = nullptr;
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
    // Manually trigger Ctrl+\ then select Big5 (index 0)
    controller->HandleKey(Key::asciiKey('\\', false, true)); // Ctrl+\ 
    
    // Check if it's in SelectingFeature state
    ASSERT_NE(dynamic_cast<InputStates::SelectingFeature*>(ui->lastState), nullptr);
    
    // Select Big5 (first feature)
    controller->SelectCandidate(0);
    
    // Now it should be in Big5 state
    auto* big5State = dynamic_cast<InputStates::Big5*>(ui->lastState);
    ASSERT_NE(big5State, nullptr);
    EXPECT_EQ(big5State->composingBuffer(), "[Big5碼] ");
}

TEST_F(BugReproTest, JumpToIrohaState) {
    controller->HandleKey(Key::asciiKey('\\', false, true)); // Ctrl+\ 
    
    // Select Iroha (fourth feature, index 3)
    controller->SelectCandidate(3);
    
    auto* irohaState = dynamic_cast<InputStates::Iroha*>(ui->lastState);
    ASSERT_NE(irohaState, nullptr);
    EXPECT_EQ(irohaState->composingBuffer(), "[伊呂波] ");
}

TEST_F(BugReproTest, SelectingDateMacroCrashRepro) {
    controller->HandleKey(Key::asciiKey('\\', false, true)); // Ctrl+\ 
    
    // Select Date/Time (index 1)
    controller->SelectCandidate(1);
    
    // Should be in SelectingDateMacro state
    auto* dateMacroState = dynamic_cast<InputStates::SelectingDateMacro*>(ui->lastState);
    ASSERT_NE(dateMacroState, nullptr);
    
    // Select the first candidate (Today Short)
    // This is where it's reported to crash.
    ASSERT_NO_THROW(controller->SelectCandidate(0));
    
    // It should have committed something
    EXPECT_FALSE(ui->committedString.empty());
    // Since ui->Update is not called for Empty state (ui->Reset is called instead),
    // we check ui->resetCalled.
    EXPECT_TRUE(ui->resetCalled);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
