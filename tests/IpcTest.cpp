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
    payload.forceVertical = true;
    payload.tooltip = "tip\ntext";
    payload.candidates = {
        "一二三",
        "一〢三〤\n千單位",
        "plain"
    };

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
    EXPECT_EQ(decoded.forceVertical, payload.forceVertical);
    EXPECT_EQ(decoded.tooltip, payload.tooltip);
    EXPECT_EQ(decoded.candidates, payload.candidates);
}

TEST(IpcTest, RejectsTruncatedSizedStringPayload) {
    std::string malformed =
        "1\n"
        "0\n"
        "0\n"
        "0\n"
        "-1\n"
        "-1\n"
        "5\n"
        "abc";

    IPC::StateUpdatePayload decoded;
    EXPECT_FALSE(IPC::DeserializeStateUpdate(malformed, decoded));
}
