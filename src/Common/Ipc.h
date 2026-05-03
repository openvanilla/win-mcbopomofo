#pragma once

#include <string>
#include <vector>

namespace McBopomofo {
namespace IPC {

const char* const PIPE_NAME = "\\\\.\\pipe\\WinMcBopomofo_IPC_Pipe";

enum class Command : int {
    CMD_RESET = 0,
    CMD_KEY_EVENT = 1,
    CMD_SELECT_CANDIDATE = 2,
    CMD_RELOAD_SETTINGS = 3,
};

struct KeyEventPayload {
    unsigned int vk;
    unsigned int ascii;
    bool shift;
    bool ctrl;
};

struct SelectCandidatePayload {
    int index;
};

struct StateUpdatePayload {
    bool consumed = false;
    std::string commitString;
    std::string composingBuffer;
    int cursorIndex = 0;
    int candidateIndex = -1; // -1 means no candidate window
    int markStart = -1; // -1 means no mark
    int markEnd = -1;
    bool forceVertical = false; // Add flag to force vertical layout
    std::string tooltip;
    std::vector<std::string> candidates;
};

// Serialize a key event to a string
std::string SerializeKeyEvent(const KeyEventPayload& payload);
// Deserialize a key event from a string
bool DeserializeKeyEvent(const std::string& data, KeyEventPayload& payload);

// Serialize a candidate selection to a string
std::string SerializeSelectCandidate(const SelectCandidatePayload& payload);
// Deserialize a candidate selection from a string
bool DeserializeSelectCandidate(const std::string& data, SelectCandidatePayload& payload);

// Serialize a reset command
std::string SerializeReset();
// Check if it is a reset command
bool IsResetCommand(const std::string& data);

// Serialize a reload settings command
std::string SerializeReloadSettings();
// Check if it is a reload settings command
bool IsReloadSettingsCommand(const std::string& data);

// Serialize a state update to a string
std::string SerializeStateUpdate(const StateUpdatePayload& payload);
// Deserialize a state update from a string
bool DeserializeStateUpdate(const std::string& data, StateUpdatePayload& payload);

} // namespace IPC
} // namespace McBopomofo
