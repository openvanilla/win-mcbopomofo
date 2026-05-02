#pragma once
#include <memory>
#include "KeyHandler.h"
#include "InputState.h"
#include "UIInterface.h"

namespace McBopomofo {

class InputController {
public:
    InputController(std::shared_ptr<KeyHandler> keyHandler, UIInterface* ui);
    ~InputController() = default;

    // Handles a key press and returns true if the key was consumed by the IME.
    bool HandleKey(const Key& key);

private:
    void ChangeState(std::unique_ptr<InputState> newState);

    std::shared_ptr<KeyHandler> keyHandler_;
    UIInterface* ui_;
    std::unique_ptr<InputState> currentState_;
};

} // namespace McBopomofo
