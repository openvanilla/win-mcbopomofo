// Copyright (c) 2026 and onwards The McBopomofo Authors.
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

#include "Ipc.h"

#include <sstream>

namespace McBopomofo {
namespace IPC {
namespace {

void WriteSizedString(std::ostringstream& ss, const std::string& value) {
  ss << value.size() << "\n" << value << "\n";
}

bool ReadSizedString(std::istringstream& ss, std::string& value) {
  std::string line;
  if (!std::getline(ss, line)) return false;

  size_t size = 0;
  try {
    size = std::stoull(line);
  } catch (...) {
    return false;
  }

  value.resize(size);
  if (size > 0) {
    ss.read(value.data(), static_cast<std::streamsize>(size));
    if (ss.gcount() != static_cast<std::streamsize>(size)) {
      return false;
    }
  }

  char terminator = '\0';
  if (!ss.get(terminator) || terminator != '\n') {
    return false;
  }
  return true;
}

}  // namespace

// Extremely simple and fast newline-delimited serialization
// Format for Key:
// VK
// SHIFT(1/0)
// CTRL(1/0)

std::string SerializeKeyEvent(const KeyEventPayload& payload) {
  std::ostringstream ss;
  ss << (int)Command::CMD_KEY_EVENT << "\n"
     << payload.vk << "\n"
     << payload.ascii << "\n"
     << (payload.shift ? 1 : 0) << "\n"
     << (payload.ctrl ? 1 : 0) << "\n";
  return ss.str();
}

bool DeserializeKeyEvent(const std::string& data, KeyEventPayload& payload) {
  std::istringstream ss(data);
  std::string line;

  if (!std::getline(ss, line)) return false;
  if (std::stoi(line) != (int)Command::CMD_KEY_EVENT) return false;

  if (!std::getline(ss, line)) return false;
  payload.vk = std::stoul(line);

  if (!std::getline(ss, line)) return false;
  payload.ascii = std::stoul(line);

  if (!std::getline(ss, line)) return false;
  payload.shift = (line == "1");

  if (!std::getline(ss, line)) return false;
  payload.ctrl = (line == "1");

  return true;
}

std::string SerializeSelectCandidate(const SelectCandidatePayload& payload) {
  std::ostringstream ss;
  ss << (int)Command::CMD_SELECT_CANDIDATE << "\n" << payload.index << "\n";
  return ss.str();
}

bool DeserializeSelectCandidate(const std::string& data,
                                SelectCandidatePayload& payload) {
  std::istringstream ss(data);
  std::string line;

  if (!std::getline(ss, line)) return false;
  if (std::stoi(line) != (int)Command::CMD_SELECT_CANDIDATE) return false;

  if (!std::getline(ss, line)) return false;
  payload.index = std::stoi(line);

  return true;
}

std::string SerializeReset() {
  std::ostringstream ss;
  ss << (int)Command::CMD_RESET << "\n";
  return ss.str();
}

bool IsResetCommand(const std::string& data) {
  std::istringstream ss(data);
  std::string line;
  if (!std::getline(ss, line)) return false;
  try {
    return std::stoi(line) == (int)Command::CMD_RESET;
  } catch (...) {
    return false;
  }
}

std::string SerializeReloadSettings() {
  std::ostringstream ss;
  ss << (int)Command::CMD_RELOAD_SETTINGS << "\n";
  return ss.str();
}

bool IsReloadSettingsCommand(const std::string& data) {
  std::istringstream ss(data);
  std::string line;
  if (!std::getline(ss, line)) return false;
  try {
    return std::stoi(line) == (int)Command::CMD_RELOAD_SETTINGS;
  } catch (...) {
    return false;
  }
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
     << payload.candidateIndex << "\n"
     << (payload.forceVertical ? 1 : 0) << "\n"
     << (payload.useShiftKeySelection ? 1 : 0) << "\n"
     << payload.markStart << "\n"
     << payload.markEnd << "\n";

  WriteSizedString(ss, payload.commitString);
  WriteSizedString(ss, payload.composingBuffer);
  WriteSizedString(ss, payload.tooltip);

  ss << payload.candidates.size() << "\n";

  for (const auto& cand : payload.candidates) {
    WriteSizedString(ss, cand);
  }
  return ss.str();
}

bool DeserializeStateUpdate(const std::string& data,
                            StateUpdatePayload& payload) {
  std::istringstream ss(data);
  std::string line;

  if (!std::getline(ss, line)) return false;
  payload.consumed = (line == "1");

  if (!std::getline(ss, line)) return false;
  payload.cursorIndex = std::stoi(line);

  if (!std::getline(ss, line)) return false;
  payload.candidateIndex = std::stoi(line);

  if (!std::getline(ss, line)) return false;
  payload.forceVertical = (line == "1");

  if (!std::getline(ss, line)) return false;
  payload.useShiftKeySelection = (line == "1");

  if (!std::getline(ss, line)) return false;
  payload.markStart = std::stoi(line);

  if (!std::getline(ss, line)) return false;
  payload.markEnd = std::stoi(line);

  if (!ReadSizedString(ss, payload.commitString)) return false;
  if (!ReadSizedString(ss, payload.composingBuffer)) return false;
  if (!ReadSizedString(ss, payload.tooltip)) return false;

  if (!std::getline(ss, line)) return false;
  size_t count = std::stoul(line);

  payload.candidates.clear();
  for (size_t i = 0; i < count; ++i) {
    std::string candidate;
    if (!ReadSizedString(ss, candidate)) return false;
    payload.candidates.push_back(std::move(candidate));
  }

  return true;
}

}  // namespace IPC
}  // namespace McBopomofo
