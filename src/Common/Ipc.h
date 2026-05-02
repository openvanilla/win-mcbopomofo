#pragma once

#include <string>
#include <vector>

namespace McBopomofo {
namespace IPC {

const char* const PIPE_NAME = "\\\\.\\pipe\\WinMcBopomofo_IPC_Pipe";

enum class Command : int {
    CMD_RESET = 0,
    CMD_KEY_EVENT = 1,
};

struct KeyEventPayload {
    unsigned int vk;
    unsigned int ascii;
    bool shift;
    bool ctrl;
};

struct StateUpdatePayload {
    bool consumed = false;
    std::string commitString;
    std::string composingBuffer;
    int cursorIndex = 0;
    std::vector<std::string> candidates;
};

// Serialize a key event to a string
std::string SerializeKeyEvent(const KeyEventPayload& payload);
// Deserialize a key event from a string
bool DeserializeKeyEvent(const std::string& data, KeyEventPayload& payload);

// Serialize a state update to a string
std::string SerializeStateUpdate(const StateUpdatePayload& payload);
// Deserialize a state update from a string
bool DeserializeStateUpdate(const std::string& data, StateUpdatePayload& payload);

} // namespace IPC
} // namespace McBopomofo
