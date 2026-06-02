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

#pragma once

#include <cstdint>
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
  CMD_OPEN_SETTINGS = 4,
  CMD_GET_SETTINGS = 5,
  CMD_CLIENT_LOG = 6,
};

enum class CandidateSelectionStyle : int {
  kStandard = 0,
  kShiftDigits = 1,
  kShiftReturn = 2,
};

struct KeyEventPayload {
  unsigned int vk;
  unsigned int ascii;
  bool shift;
  bool ctrl;
  float dpiScale = 1.0f;
  bool hasCoords = false;
  uint64_t ownerHwnd = 0;
  int anchorLeft = 0;
  int anchorTop = 0;
  int anchorRight = 0;
  int anchorBottom = 0;
};

struct SelectCandidatePayload {
  int index;
};

struct ClientSettingsPayload {
  bool shiftToggleOpenClose = true;
};

struct ClientLogPayload {
  unsigned long processId = 0;
  uint64_t elapsedMs = 0;
  std::string message;
};



struct CandidateWindowColors {
  uint32_t text = 0x101010;
  uint32_t background = 0xFFFFFF;
  uint32_t border = 0xCCCCCC;
  uint32_t highlightBackground = 0x0078D7;
  uint32_t highlightText = 0xFFFFFF;
};

struct StateUpdatePayload {
  bool consumed = false;
  std::string commitString;
  std::string composingBuffer;
  int cursorIndex = 0;
  int candidateIndex = -1;  // -1 means no candidate window
  int candidateFontSize = 16;
  int markStart = -1;  // -1 means no mark
  int markEnd = -1;
  bool forceVertical = false;  // Add flag to force vertical layout
  CandidateSelectionStyle selectionStyle = CandidateSelectionStyle::kStandard;
  std::string tooltip;
  std::string hint;
  bool candidateWindowVertical = false;
  std::string candidateKeys = "123456789";
  int candidateKeysCount = 9;
  CandidateWindowColors candidateWindowColors;
  std::vector<std::string> candidates;
};

// Serialize a key event to a string
std::string SerializeKeyEvent(const KeyEventPayload& payload);
// Deserialize a key event from a string
bool DeserializeKeyEvent(const std::string& data, KeyEventPayload& payload);

// Serialize a candidate selection to a string
std::string SerializeSelectCandidate(const SelectCandidatePayload& payload);
// Deserialize a candidate selection from a string
bool DeserializeSelectCandidate(const std::string& data,
                                SelectCandidatePayload& payload);

// Serialize a reset command
std::string SerializeReset();
// Check if it is a reset command
bool IsResetCommand(const std::string& data);

// Serialize a reload settings command
std::string SerializeReloadSettings();
// Check if it is a reload settings command
bool IsReloadSettingsCommand(const std::string& data);

// Serialize an open settings command
std::string SerializeOpenSettings();
// Check if it is an open settings command
bool IsOpenSettingsCommand(const std::string& data);

// Serialize a get settings command
std::string SerializeGetSettings();
// Check if it is a get settings command
bool IsGetSettingsCommand(const std::string& data);

// Serialize a client settings response
std::string SerializeClientSettings(const ClientSettingsPayload& payload);
// Deserialize a client settings response
bool DeserializeClientSettings(const std::string& data,
                               ClientSettingsPayload& payload);

// Serialize a relayed client log message
std::string SerializeClientLog(const ClientLogPayload& payload);
// Deserialize a relayed client log message
bool DeserializeClientLog(const std::string& data, ClientLogPayload& payload);



// Serialize a state update to a string
std::string SerializeStateUpdate(const StateUpdatePayload& payload);
// Deserialize a state update from a string
bool DeserializeStateUpdate(const std::string& data,
                            StateUpdatePayload& payload);

}  // namespace IPC
}  // namespace McBopomofo
