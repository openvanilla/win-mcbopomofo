#include <gtest/gtest.h>

#include "Ipc.h"

using namespace McBopomofo;

TEST(IpcTest, StateUpdateRoundTripsMultilineStrings) {
  IPC::StateUpdatePayload payload;
  payload.consumed = true;
  payload.commitString = "line1\nline2";
  payload.composingBuffer = "compose\nbuffer";
  payload.cursorIndex = 3;
  payload.candidateIndex = 2;
  payload.markStart = 1;
  payload.markEnd = 4;
  payload.tooltip = "tip\ntext";
  payload.candidateKeys = "asdfghjkl";
  payload.candidateKeysCount = 8;
  payload.candidates = {"一二三", "一〢三〤\n千單位", "plain"};

  std::string serialized = IPC::SerializeStateUpdate(payload);

  IPC::StateUpdatePayload decoded;
  ASSERT_TRUE(IPC::DeserializeStateUpdate(serialized, decoded));
  EXPECT_EQ(decoded.consumed, payload.consumed);
  EXPECT_EQ(decoded.commitString, payload.commitString);
  EXPECT_EQ(decoded.composingBuffer, payload.composingBuffer);
  EXPECT_EQ(decoded.cursorIndex, payload.cursorIndex);
  EXPECT_EQ(decoded.candidateIndex, payload.candidateIndex);
  EXPECT_EQ(decoded.markStart, payload.markStart);
  EXPECT_EQ(decoded.markEnd, payload.markEnd);
  EXPECT_EQ(decoded.tooltip, payload.tooltip);
  EXPECT_EQ(decoded.candidateKeys, payload.candidateKeys);
  EXPECT_EQ(decoded.candidateKeysCount, payload.candidateKeysCount);
  EXPECT_EQ(decoded.candidates, payload.candidates);
}

TEST(IpcTest, RejectsTruncatedSizedStringPayload) {
  std::string malformed =
      "1\n"
      "0\n"
      "0\n"
      "0\n"
      "0\n"
      "-1\n"
      "-1\n"
      "0\n"
      "0\n"
      "0\n"
      "0\n"
      "1\n"
      "5\n"
      "abc";

  IPC::StateUpdatePayload decoded;
  EXPECT_FALSE(IPC::DeserializeStateUpdate(malformed, decoded));
}

TEST(IpcTest, ClientSettingsRoundTrip) {
  IPC::ClientSettingsPayload payload;
  payload.shiftToggleOpenClose = false;

  std::string serialized = IPC::SerializeClientSettings(payload);

  IPC::ClientSettingsPayload decoded;
  ASSERT_TRUE(IPC::DeserializeClientSettings(serialized, decoded));
  EXPECT_EQ(decoded.shiftToggleOpenClose, payload.shiftToggleOpenClose);
  EXPECT_TRUE(IPC::IsGetSettingsCommand(IPC::SerializeGetSettings()));
}

TEST(IpcTest, ProcessDisabledQueryRoundTrip) {
  IPC::ProcessDisabledQueryPayload query;
  query.processName = "SheepShaver.exe";

  std::string serializedQuery = IPC::SerializeProcessDisabledQuery(query);

  IPC::ProcessDisabledQueryPayload decodedQuery;
  ASSERT_TRUE(
      IPC::DeserializeProcessDisabledQuery(serializedQuery, decodedQuery));
  EXPECT_EQ(decodedQuery.processName, query.processName);

  IPC::ProcessDisabledResponsePayload response;
  response.disabled = true;

  std::string serializedResponse =
      IPC::SerializeProcessDisabledResponse(response);

  IPC::ProcessDisabledResponsePayload decodedResponse;
  ASSERT_TRUE(IPC::DeserializeProcessDisabledResponse(serializedResponse,
                                                      decodedResponse));
  EXPECT_EQ(decodedResponse.disabled, response.disabled);
}

TEST(IpcTest, KeyEventRoundTripsLayoutAnchor) {
  IPC::KeyEventPayload payload;
  payload.vk = 'A';
  payload.ascii = 'a';
  payload.shift = false;
  payload.ctrl = true;
  payload.hasCoords = true;
  payload.ownerHwnd = 1234;
  payload.anchorLeft = 10;
  payload.anchorTop = 20;
  payload.anchorRight = 30;
  payload.anchorBottom = 40;

  std::string serialized = IPC::SerializeKeyEvent(payload);

  IPC::KeyEventPayload decoded;
  ASSERT_TRUE(IPC::DeserializeKeyEvent(serialized, decoded));
  EXPECT_EQ(decoded.vk, payload.vk);
  EXPECT_EQ(decoded.ascii, payload.ascii);
  EXPECT_EQ(decoded.shift, payload.shift);
  EXPECT_EQ(decoded.ctrl, payload.ctrl);
  EXPECT_EQ(decoded.hasCoords, payload.hasCoords);
  EXPECT_EQ(decoded.ownerHwnd, payload.ownerHwnd);
  EXPECT_EQ(decoded.anchorLeft, payload.anchorLeft);
  EXPECT_EQ(decoded.anchorTop, payload.anchorTop);
  EXPECT_EQ(decoded.anchorRight, payload.anchorRight);
  EXPECT_EQ(decoded.anchorBottom, payload.anchorBottom);
}

TEST(IpcTest, KeyEventRejectsLegacyPayloadWithoutLayoutAnchor) {
  std::string legacy =
      "1\n"
      "65\n"
      "97\n"
      "0\n"
      "1\n";

  IPC::KeyEventPayload decoded;
  EXPECT_FALSE(IPC::DeserializeKeyEvent(legacy, decoded));
}

TEST(IpcTest, KeyEventRejectsLegacyExplicitNoLayoutAnchor) {
  std::string noLayout =
      "1\n"
      "65\n"
      "97\n"
      "0\n"
      "1\n"
      "0\n";

  IPC::KeyEventPayload decoded;
  EXPECT_FALSE(IPC::DeserializeKeyEvent(noLayout, decoded));
}

TEST(IpcTest, KeyEventRejectsPayloadWithRemovedDpiScale) {
  std::string removedDpiScalePayload =
      "1\n"     // CMD_KEY_EVENT
      "65\n"    // vk
      "97\n"    // ascii
      "0\n"     // shift
      "1\n"     // ctrl
      "1\n"     // removed dpiScale field
      "1\n"     // hasCoords
      "5678\n"  // ownerHwnd
      "100\n"   // anchorLeft
      "200\n"   // anchorTop
      "300\n"   // anchorRight
      "400\n";  // anchorBottom

  IPC::KeyEventPayload decoded;
  EXPECT_FALSE(IPC::DeserializeKeyEvent(removedDpiScalePayload, decoded));
}
