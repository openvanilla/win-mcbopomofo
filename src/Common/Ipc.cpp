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

  try {
    value.resize(size);
  } catch (...) {
    return false;
  }

  const auto streamSize = static_cast<std::streamsize>(size);
  if (size != static_cast<size_t>(streamSize)) {
    return false;
  }

  if (size > 0) {
    ss.read(value.data(), streamSize);
    if (ss.gcount() != streamSize) {
      return false;
    }
  }

  char terminator = '\0';
  if (!ss.get(terminator) || terminator != '\n') {
    return false;
  }
  return true;
}

bool HasNoTrailingData(std::istringstream& ss) {
  return ss.peek() == std::char_traits<char>::eof();
}

}  // namespace

// Extremely simple and fast newline-delimited serialization

std::string SerializeKeyEvent(const KeyEventPayload& payload) {
  std::ostringstream ss;
  ss << (int)Command::CMD_KEY_EVENT << "\n"
     << payload.vk << "\n"
     << payload.ascii << "\n"
     << (payload.shift ? 1 : 0) << "\n"
     << (payload.ctrl ? 1 : 0) << "\n"
     << (payload.hasCoords ? 1 : 0) << "\n"
     << payload.ownerHwnd << "\n"
     << payload.anchorLeft << "\n"
     << payload.anchorTop << "\n"
     << payload.anchorRight << "\n"
     << payload.anchorBottom << "\n";
  return ss.str();
}

bool DeserializeKeyEvent(const std::string& data, KeyEventPayload& payload) {
  std::istringstream ss(data);
  std::vector<std::string> lines;
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }

  if (lines.size() != 11) return false;
  try {
    if (std::stoi(lines[0]) != (int)Command::CMD_KEY_EVENT) return false;
    payload.vk = std::stoul(lines[1]);
    payload.ascii = std::stoul(lines[2]);
    payload.shift = (lines[3] == "1");
    payload.ctrl = (lines[4] == "1");
    payload.hasCoords = (lines[5] == "1");
    payload.ownerHwnd = std::stoull(lines[6]);
    payload.anchorLeft = std::stoi(lines[7]);
    payload.anchorTop = std::stoi(lines[8]);
    payload.anchorRight = std::stoi(lines[9]);
    payload.anchorBottom = std::stoi(lines[10]);
  } catch (...) {
    return false;
  }

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

  try {
    if (!std::getline(ss, line)) return false;
    if (std::stoi(line) != (int)Command::CMD_SELECT_CANDIDATE) return false;

    if (!std::getline(ss, line)) return false;
    payload.index = std::stoi(line);
  } catch (...) {
    return false;
  }

  return HasNoTrailingData(ss);
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

std::string SerializeOpenSettings() {
  std::ostringstream ss;
  ss << (int)Command::CMD_OPEN_SETTINGS << "\n";
  return ss.str();
}

bool IsOpenSettingsCommand(const std::string& data) {
  std::istringstream ss(data);
  std::string line;
  if (!std::getline(ss, line)) return false;
  try {
    return std::stoi(line) == (int)Command::CMD_OPEN_SETTINGS;
  } catch (...) {
    return false;
  }
}

std::string SerializeGetSettings() {
  std::ostringstream ss;
  ss << (int)Command::CMD_GET_SETTINGS << "\n";
  return ss.str();
}

bool IsGetSettingsCommand(const std::string& data) {
  std::istringstream ss(data);
  std::string line;
  if (!std::getline(ss, line)) return false;
  try {
    return std::stoi(line) == (int)Command::CMD_GET_SETTINGS;
  } catch (...) {
    return false;
  }
}

std::string SerializeClientSettings(const ClientSettingsPayload& payload) {
  std::ostringstream ss;
  ss << (payload.shiftToggleOpenClose ? 1 : 0) << "\n";
  return ss.str();
}

bool DeserializeClientSettings(const std::string& data,
                               ClientSettingsPayload& payload) {
  std::istringstream ss(data);
  std::string line;
  if (!std::getline(ss, line)) return false;
  payload.shiftToggleOpenClose = (line == "1");
  return HasNoTrailingData(ss);
}

std::string SerializeClientLog(const ClientLogPayload& payload) {
  std::ostringstream ss;
  ss << (int)Command::CMD_CLIENT_LOG << "\n"
     << payload.processId << "\n"
     << payload.elapsedMs << "\n";
  WriteSizedString(ss, payload.message);
  return ss.str();
}

bool DeserializeClientLog(const std::string& data, ClientLogPayload& payload) {
  std::istringstream ss(data);
  std::string line;

  try {
    if (!std::getline(ss, line)) return false;
    if (std::stoi(line) != (int)Command::CMD_CLIENT_LOG) return false;

    if (!std::getline(ss, line)) return false;
    payload.processId = static_cast<unsigned long>(std::stoul(line));

    if (!std::getline(ss, line)) return false;
    payload.elapsedMs = std::stoull(line);
  } catch (...) {
    return false;
  }

  return ReadSizedString(ss, payload.message) && HasNoTrailingData(ss);
}

std::string SerializeProcessDisabledQuery(
    const ProcessDisabledQueryPayload& payload) {
  std::ostringstream ss;
  ss << (int)Command::CMD_IS_PROCESS_DISABLED << "\n";
  WriteSizedString(ss, payload.processName);
  return ss.str();
}

bool DeserializeProcessDisabledQuery(const std::string& data,
                                     ProcessDisabledQueryPayload& payload) {
  std::istringstream ss(data);
  std::string line;

  try {
    if (!std::getline(ss, line)) return false;
    if (std::stoi(line) != (int)Command::CMD_IS_PROCESS_DISABLED) return false;
  } catch (...) {
    return false;
  }

  return ReadSizedString(ss, payload.processName) && HasNoTrailingData(ss);
}

std::string SerializeProcessDisabledResponse(
    const ProcessDisabledResponsePayload& payload) {
  std::ostringstream ss;
  ss << (payload.disabled ? 1 : 0) << "\n";
  return ss.str();
}

bool DeserializeProcessDisabledResponse(
    const std::string& data, ProcessDisabledResponsePayload& payload) {
  std::istringstream ss(data);
  std::string line;
  if (!std::getline(ss, line)) return false;
  payload.disabled = (line == "1");
  return HasNoTrailingData(ss);
}

std::string SerializeStateUpdate(const StateUpdatePayload& payload) {
  std::ostringstream ss;
  ss << (payload.consumed ? 1 : 0) << "\n"
     << payload.cursorIndex << "\n"
     << payload.candidateIndex << "\n"
     << payload.markStart << "\n"
     << payload.markEnd << "\n";

  WriteSizedString(ss, payload.commitString);
  WriteSizedString(ss, payload.composingBuffer);
  WriteSizedString(ss, payload.tooltip);

  ss << payload.candidates.size() << "\n";

  for (const auto& cand : payload.candidates) {
    WriteSizedString(ss, cand);
  }

  WriteSizedString(ss, payload.candidateKeys);
  ss << payload.candidateKeysCount << "\n";
  return ss.str();
}

bool DeserializeStateUpdate(const std::string& data,
                            StateUpdatePayload& payload) {
  std::istringstream ss(data);
  std::string line;

  try {
    if (!std::getline(ss, line)) return false;
    payload.consumed = (line == "1");

    if (!std::getline(ss, line)) return false;
    payload.cursorIndex = std::stoi(line);

    if (!std::getline(ss, line)) return false;
    payload.candidateIndex = std::stoi(line);

    if (!std::getline(ss, line)) return false;
    payload.markStart = std::stoi(line);

    if (!std::getline(ss, line)) return false;
    payload.markEnd = std::stoi(line);
  } catch (...) {
    return false;
  }

  if (!ReadSizedString(ss, payload.commitString)) return false;
  if (!ReadSizedString(ss, payload.composingBuffer)) return false;
  if (!ReadSizedString(ss, payload.tooltip)) return false;

  size_t count = 0;
  try {
    if (!std::getline(ss, line)) return false;
    count = std::stoul(line);
  } catch (...) {
    return false;
  }

  payload.candidates.clear();
  for (size_t i = 0; i < count; ++i) {
    std::string candidate;
    if (!ReadSizedString(ss, candidate)) return false;
    payload.candidates.push_back(std::move(candidate));
  }

  if (!ReadSizedString(ss, payload.candidateKeys)) return false;
  try {
    if (!std::getline(ss, line)) return false;
    payload.candidateKeysCount = std::stoi(line);
  } catch (...) {
    return false;
  }

  return HasNoTrailingData(ss);
}

}  // namespace IPC
}  // namespace McBopomofo
