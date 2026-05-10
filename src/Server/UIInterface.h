#pragma once
#include <string>
#include "Ipc.h"

namespace McBopomofo {

class UIInterface {
public:
    virtual ~UIInterface() = default;

    // Called when the input state is completely reset (e.g. Esc pressed).
    virtual void reset() = 0;

    // Called when a string should be directly committed to the application.
    virtual void commitString(const std::string& text) = 0;

    // Called when the composition or candidate window state changes.
    virtual void update(const IPC::StateUpdatePayload& state) = 0;
};

} // namespace McBopomofo
