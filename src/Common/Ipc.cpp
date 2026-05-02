#include "Ipc.h"
#include <sstream>

namespace McBopomofo {
namespace IPC {

// Extremely simple and fast newline-delimited serialization
// Format for Key:
// VK
// SHIFT(1/0)
// CTRL(1/0)

std::string SerializeKeyEvent(const KeyEventPayload& payload) {
    std::ostringstream ss;
    ss << payload.vk << "\n"
       << (payload.shift ? 1 : 0) << "\n"
       << (payload.ctrl ? 1 : 0) << "\n";
    return ss.str();
}

bool DeserializeKeyEvent(const std::string& data, KeyEventPayload& payload) {
    std::istringstream ss(data);
    std::string line;
    
    if (!std::getline(ss, line)) return false;
    payload.vk = std::stoul(line);
    
    if (!std::getline(ss, line)) return false;
    payload.shift = (line == "1");

    if (!std::getline(ss, line)) return false;
    payload.ctrl = (line == "1");

    return true;
}

// Format for StateUpdate:
// CONSUMED(1/0)
// CURSOR_INDEX
// COMMIT_STRING
// COMPOSING_BUFFER
// CANDIDATE_COUNT
// CANDIDATE_1
// CANDIDATE_2
// ...

std::string SerializeStateUpdate(const StateUpdatePayload& payload) {
    std::ostringstream ss;
    ss << (payload.consumed ? 1 : 0) << "\n"
       << payload.cursorIndex << "\n"
       << payload.commitString << "\n"
       << payload.composingBuffer << "\n"
       << payload.candidates.size() << "\n";
    
    for (const auto& cand : payload.candidates) {
        ss << cand << "\n";
    }
    return ss.str();
}

bool DeserializeStateUpdate(const std::string& data, StateUpdatePayload& payload) {
    std::istringstream ss(data);
    std::string line;

    if (!std::getline(ss, line)) return false;
    payload.consumed = (line == "1");

    if (!std::getline(ss, line)) return false;
    payload.cursorIndex = std::stoi(line);

    if (!std::getline(ss, line)) return false;
    payload.commitString = line;

    if (!std::getline(ss, line)) return false;
    payload.composingBuffer = line;

    if (!std::getline(ss, line)) return false;
    size_t count = std::stoul(line);

    payload.candidates.clear();
    for (size_t i = 0; i < count; ++i) {
        if (!std::getline(ss, line)) return false;
        payload.candidates.push_back(line);
    }

    return true;
}

} // namespace IPC
} // namespace McBopomofo
