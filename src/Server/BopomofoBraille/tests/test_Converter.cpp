#include "doctest.h"
#include "BopomofoBraille/Converter.h"
#include "BopomofoBraille/BopomofoSyllable.h"
#include <string>
#include <vector>
#include <variant>

using namespace McBopomofo;

// Helpers to make tests cleaner
static std::string convertBpmfToBraille(const std::string& input, BrailleType type = BrailleType::UNICODE) {
    return BopomofoBrailleConverter::convertBpmfToBraille(input, type);
}

static std::string convertBrailleToBpmf(const std::string& input, BrailleType type = BrailleType::UNICODE) {
    return BopomofoBrailleConverter::convertBrailleToBpmf(input, type);
}

static std::vector<BopomofoBrailleConverter::Token> convertBrailleToTokens(const std::string& input, BrailleType type = BrailleType::UNICODE) {
    return BopomofoBrailleConverter::convertBrailleToTokens(input, type);
}

TEST_SUITE("BopomofoBrailleConverter") {

    TEST_CASE("Test 1 bopomofo syllable") {
        std::string input = "ㄊㄞˊ";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠋⠺⠂");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test 2 bopomofo syllables") {
        std::string input = "ㄊㄞˊㄨㄢ";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠋⠺⠂⠻⠄");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test 3 bopomofo syllables 1") {
        std::string input = "ㄊㄞˊㄨㄢㄖㄣˊ";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠋⠺⠂⠻⠄⠛⠥⠂");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test 3 bopomofo syllables 2") {
        std::string input = "ㄏㄨㄤˊㄈㄟㄏㄨㄥˊ";
        std::string r1 = convertBpmfToBraille(input);
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test bopomofo syllables and punctuation 1") {
        std::string input = "ㄏㄨㄤˊㄈㄟㄏㄨㄥˊ，";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠗⠸⠂⠟⠴⠄⠗⠯⠂⠆");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test bopomofo syllables and punctuation 2") {
        std::string input = "『『ㄊㄞˊ";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠰⠤⠰⠤⠰⠤⠰⠤⠋⠺⠂");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test long phrase") {
        std::string input = "『『ㄊㄞˊㄨㄢㄖㄣˊㄒㄩㄧㄠˋㄏㄣˇㄉㄨㄛㄉㄜ˙ㄒㄧㄠㄆㄛㄎㄨㄞˋ』』";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠰⠤⠰⠤⠰⠤⠰⠤⠋⠺⠂⠻⠄⠛⠥⠂⠑⠳⠄⠪⠐⠗⠥⠈⠙⠒⠄⠙⠮⠁⠑⠪⠄⠏⠣⠄⠇⠶⠐⠤⠆⠤⠆⠤⠆⠤⠆");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test punctuation 1") {
        std::string input = "『";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠰⠤⠰⠤");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test punctuation 2") {
        std::string input = "『『";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠰⠤⠰⠤⠰⠤⠰⠤");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test bopomofo syllables and lower case letter") {
        std::string input = "ㄊㄞˊabc";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠋⠺⠂ ⠁⠃⠉");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "ㄊㄞˊ abc");
    }

    TEST_CASE("Test bopomofo syllables and upper case letter - 1") {
        std::string input = "ㄊㄞˊAbc";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠋⠺⠂ ⠠⠁⠃⠉");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "ㄊㄞˊ Abc");
    }

    TEST_CASE("Test bopomofo syllables and upper case letter - 2") {
        std::string input = "Abcㄊㄞˊ";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠠⠁⠃⠉ ⠋⠺⠂");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "Abc ㄊㄞˊ");
    }

    TEST_CASE("Test bopomofo syllables and digit 1 - 1") {
        std::string input = "ㄊㄞˊ1234";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠋⠺⠂ ⠼⠂⠆⠒⠲");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "ㄊㄞˊ 1234");
    }

    TEST_CASE("Test digit 1 - 1") {
        std::string input = "1234";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠼⠂⠆⠒⠲");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "1234");
    }

    TEST_CASE("Test digit 1 - 2") {
        std::string input = "2234";
        std::string r1 = convertBpmfToBraille(input);
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "2234");
    }

    TEST_CASE("Test digit 1 - 3") {
        std::string input = "22.34";
        std::string r1 = convertBpmfToBraille(input);
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "22.34");
    }

    TEST_CASE("Test digit and letter 1 - 1") {
        std::string input = "ABCD 1234";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠠⠁⠠⠃⠠⠉⠠⠙ ⠼⠂⠆⠒⠲");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "ABCD 1234");
    }

    TEST_CASE("Test digit and letter 2 - 1") {
        std::string input = "1234 ABCD";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠼⠂⠆⠒⠲ ⠠⠁⠠⠃⠠⠉⠠⠙");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "1234 ABCD");
    }

    TEST_CASE("Test Text and Digits 1") {
        std::string input = "ㄉㄧˋ1";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠙⠡⠐ ⠼⠂");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "ㄉㄧˋ 1");
    }

    TEST_CASE("Test letter 1 - 1") {
        std::string input = "name";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠝⠁⠍⠑");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "name");
    }

    TEST_CASE("Test mix 1 - 1") {
        std::string input = "ㄒㄧㄠˇㄇㄞˋ";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠑⠪⠈⠍⠺⠐");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == "ㄒㄧㄠˇㄇㄞˋ");
    }

    TEST_CASE("Test mix 1 - 2") {
        std::string input = "ㄒㄧㄠˇㄇㄞˋㄓㄨˋㄧ音"; // Wait, in the test it was "ㄓㄨˋㄧㄣ" (Zhuyin) - ah, TS says ㄓㄨˋㄧㄣ but wait, the TS code on line 191 is ㄒㄧㄠˇㄇㄞˋㄓㄨˋㄧㄣ
        std::string input_corrected = "ㄒㄧㄠˇㄇㄞˋㄓㄨˋㄧㄣ";
        std::string r1 = convertBpmfToBraille(input_corrected);
        CHECK(r1 == "⠑⠪⠈⠍⠺⠐⠁⠌⠐⠹⠄");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input_corrected);
    }

    TEST_CASE("Test mix 1 - 3") {
        std::string input = "ㄒㄧㄠˇㄇㄞˋㄓㄨˋㄧㄣ 2.5";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠑⠪⠈⠍⠺⠐⠁⠌⠐⠹⠄ ⠼⠆⠨⠢");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test mix 1 - 4") {
        std::string input = "ㄒㄧㄠˇㄇㄞˋㄓㄨˋㄧㄣ 25%";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠑⠪⠈⠍⠺⠐⠁⠌⠐⠹⠄ ⠼⠆⠢⠈⠴");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("Test mix 1 - 5") {
        std::string input = "ㄒㄧㄠˇㄇㄞˋㄓㄨˋㄧㄣ 25°C";
        std::string r1 = convertBpmfToBraille(input);
        CHECK(r1 == "⠑⠪⠈⠍⠺⠐⠁⠌⠐⠹⠄ ⠼⠆⠢⠘⠨⠡ ⠰⠠⠉");
        std::string r2 = convertBrailleToBpmf(r1);
        CHECK(r2 == input);
    }

    TEST_CASE("ASCII type support") {
        SUBCASE("converts Bopomofo syllables to ASCII Braille") {
            std::string input = "ㄊㄞˊ";
            std::string r1 = convertBpmfToBraille(input, BrailleType::ASCII);
            CHECK(r1 == "fw1");
            CHECK(convertBrailleToBpmf(r1, BrailleType::ASCII) == input);
        }

        SUBCASE("converts mixed ASCII content") {
            std::string input = "Ab3";
            std::string r1 = convertBpmfToBraille(input, BrailleType::ASCII);
            CHECK(r1 == ",ab #3");
            CHECK(convertBrailleToBpmf(r1, BrailleType::ASCII) == "Ab 3");
        }

        SUBCASE("tokenizes ASCII Braille into Bopomofo syllable objects") {
            auto tokens = convertBrailleToTokens("fw1", BrailleType::ASCII);
            CHECK(tokens.size() == 1);
            CHECK(std::holds_alternative<BopomofoSyllable>(tokens[0]));
            if (std::holds_alternative<BopomofoSyllable>(tokens[0])) {
                CHECK(std::get<BopomofoSyllable>(tokens[0]).bpmf == "ㄊㄞˊ");
            }
        }
    }

    TEST_CASE("convertBrailleToTokens method") {
        SUBCASE("should return tokens for mixed content") {
            std::string braille = "⠑⠪⠈⠍⠺⠐⠁⠌⠐⠹⠄ ⠼⠆⠢";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() > 0);
            
            bool has_obj = false;
            bool has_str = false;
            for (const auto& token : tokens) {
                if (std::holds_alternative<BopomofoSyllable>(token)) has_obj = true;
                if (std::holds_alternative<std::string>(token)) has_str = true;
            }
            CHECK(has_obj == true);
            CHECK(has_str == true);
        }

        SUBCASE("should handle pure bopomofo content") {
            std::string braille = "⠑⠪⠈⠍⠺⠐";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() > 0);
            for (const auto& token : tokens) {
                CHECK(std::holds_alternative<BopomofoSyllable>(token));
            }
        }

        SUBCASE("should handle pure text content") {
            std::string braille = "⠝⠁⠍⠑";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() == 1);
            CHECK(std::holds_alternative<std::string>(tokens[0]));
            CHECK(std::get<std::string>(tokens[0]) == "name");
        }

        SUBCASE("should handle empty input") {
            auto tokens = convertBrailleToTokens("");
            CHECK(tokens.empty());
        }

        SUBCASE("should handle spaces correctly") {
            std::string braille = "⠑⠪⠈ ⠍⠺⠐";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() > 1);
        }

        SUBCASE("should reset digit state when followed by bopomofo") {
            std::string braille = "⠼⠂⠻⠄";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() == 2);
            CHECK(std::holds_alternative<std::string>(tokens[0]));
            CHECK(std::get<std::string>(tokens[0]) == "1");
            CHECK(std::holds_alternative<BopomofoSyllable>(tokens[1]));
            if (std::holds_alternative<BopomofoSyllable>(tokens[1])) {
                CHECK(std::get<BopomofoSyllable>(tokens[1]).bpmf == "ㄨㄢ");
            }
            std::string roundtrip = convertBrailleToBpmf(braille);
            CHECK(roundtrip == "1ㄨㄢ");
        }

        SUBCASE("should reset letter state when followed by bopomofo") {
            std::string braille = "⠠⠁⠻⠄";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() == 2);
            CHECK(std::holds_alternative<std::string>(tokens[0]));
            CHECK(std::get<std::string>(tokens[0]) == "A");
            CHECK(std::holds_alternative<BopomofoSyllable>(tokens[1]));
            if (std::holds_alternative<BopomofoSyllable>(tokens[1])) {
                CHECK(std::get<BopomofoSyllable>(tokens[1]).bpmf == "ㄨㄢ");
            }
            std::string roundtrip = convertBrailleToBpmf(braille);
            CHECK(roundtrip == "Aㄨㄢ");
        }
    }

    TEST_CASE("Edge cases and error handling") {
        SUBCASE("Test ※ 1234") {
            std::string input = "※ 1234";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1 == "⠈⠼ ⠼⠂⠆⠒⠲");
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == input);
        }

        SUBCASE("should handle empty string input") {
            std::string input = "";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1 == "");
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == "");
        }

        SUBCASE("should handle space-only input") {
            std::string input = "   ";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == "   "); // Wait, TS expectation: expect(r2.trim()).toBe(""); we can check that it's whitespace
            CHECK(r2.find_first_not_of(" ") == std::string::npos);
        }

        SUBCASE("should handle invalid bopomofo characters") {
            std::string input = "ㄊㄞˊ@#$";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1.find("⠋⠺⠂") != std::string::npos);
            CHECK(r1.find("@#$") != std::string::npos);
        }

        SUBCASE("should handle consecutive spaces") {
            std::string input = "ㄊㄞˊ   ㄨㄢ";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
            CHECK(r2.find("ㄨㄢ") != std::string::npos);
        }

        SUBCASE("should handle mixed case letters") {
            std::string input = "aBcD";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1.find("⠠") != std::string::npos);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == "aBcD");
        }

        SUBCASE("should handle special numeric characters") {
            std::string input = "123+456=579";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("123") != std::string::npos);
            CHECK(r2.find("456") != std::string::npos);
            CHECK(r2.find("579") != std::string::npos);
        }
    }

    TEST_CASE("Complex punctuation handling") {
        SUBCASE("should handle specific punctuation marks") {
            std::string input = "ㄊㄞˊ，ㄨㄢ";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
            CHECK(r2.find("ㄨㄢ") != std::string::npos);
        }

        SUBCASE("should handle mixed content with punctuation") {
            std::string input = "ㄊㄞˊ，ㄨㄢ。";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
            CHECK(r2.find("ㄨㄢ") != std::string::npos);
        }

        SUBCASE("should handle quotation marks") {
            std::string input = "「ㄊㄞˊㄨㄢ」";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊㄨㄢ") != std::string::npos);
        }

        SUBCASE("should handle brackets and parentheses") {
            std::string input = "（ㄊㄞˊㄨㄢ）";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊㄨㄢ") != std::string::npos);
        }

        SUBCASE("should keep letters and punctuation compact") {
            std::string input = "abc.";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1 == "⠁⠃⠉⠲");
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == input);
        }
    }

    TEST_CASE("State transition tests") {
        SUBCASE("should handle transitions between different content types") {
            std::string input = "ㄊㄞˊ123abc";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1.find("⠋⠺⠂") != std::string::npos);
            CHECK(r1.find("⠼") != std::string::npos);
            CHECK(r1.find("⠁⠃⠉") != std::string::npos);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
            CHECK(r2.find("123") != std::string::npos);
            CHECK(r2.find("abc") != std::string::npos);
        }

        SUBCASE("should handle alternating content types") {
            std::string input = "ㄊㄞˊ1ㄨㄢa";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
            CHECK(r2.find("1") != std::string::npos);
            CHECK(r2.find("ㄨㄢ") != std::string::npos);
            CHECK(r2.find("a") != std::string::npos);
        }
    }

    TEST_CASE("Performance and boundary tests") {
        SUBCASE("should handle long input strings") {
            std::string longInput = "";
            for (int i = 0; i < 100; ++i) {
                longInput += "ㄊㄞˊㄨㄢ";
            }
            std::string r1 = convertBpmfToBraille(longInput);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == longInput);
        }

        SUBCASE("should handle character conversions correctly") {
            struct Case {
                std::string input;
                std::string expected;
                std::string expectedContains;
            };
            std::vector<Case> validChars = {
                {"ㄞ", "⠺⠄", ""},
                {"a", "", "⠁"},
                {"A", "", "⠠⠁"}
            };

            for (const auto& c : validChars) {
                std::string r1 = convertBpmfToBraille(c.input);
                if (!c.expected.empty()) {
                    CHECK(r1 == c.expected);
                } else if (!c.expectedContains.empty()) {
                    CHECK(r1.find(c.expectedContains) != std::string::npos);
                }

                std::string r2 = convertBrailleToBpmf(r1);
                if (c.input == "1" || c.input == "a" || c.input == "A") {
                    CHECK(r2.find(c.input) != std::string::npos);
                } else {
                    CHECK(r2 == c.input);
                }
            }
        }
    }

    TEST_CASE("Unicode and special character handling") {
        SUBCASE("should handle non-ASCII characters") {
            std::string input = "ㄊㄞˊ中文測試";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
            CHECK(r2.find("中文測試") != std::string::npos);
        }

        SUBCASE("should handle emoji and symbols") {
            std::string input = "ㄊㄞˊ😀★♪";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
            CHECK(r2.find("😀★♪") != std::string::npos);
        }
    }

    TEST_CASE("Consistency tests") {
        SUBCASE("should be consistent across multiple conversions") {
            std::string input = "ㄒㄧㄠˇㄇㄞˋㄓㄨˋㄧㄣ 25°C";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            std::string r3 = convertBpmfToBraille(r2);
            std::string r4 = convertBrailleToBpmf(r3);
            CHECK(r2 == r4);
        }

        SUBCASE("should handle whitespace normalization") {
            std::string input1 = "ㄊㄞˊ ㄨㄢ";
            std::string input2 = "ㄊㄞˊ  ㄨㄢ";
            std::string r1 = convertBpmfToBraille(input1);
            std::string r2 = convertBpmfToBraille(input2);
            CHECK(r1.find("⠋⠺⠂") != std::string::npos);
            CHECK(r2.find("⠋⠺⠂") != std::string::npos);
        }
    }

    TEST_CASE("Additional comprehensive tests") {
        SUBCASE("should handle digit state transitions properly") {
            std::string input = "1a2b3";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("1") != std::string::npos);
            CHECK(r2.find("2") != std::string::npos);
            CHECK(r2.find("3") != std::string::npos);
            CHECK(r2.find("a") != std::string::npos);
            CHECK(r2.find("b") != std::string::npos);
        }

        SUBCASE("should handle rapid state changes") {
            std::string input = "ㄊ1aㄞb2";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("1") != std::string::npos);
            CHECK(r2.find("2") != std::string::npos);
            CHECK(r2.find("a") != std::string::npos);
            CHECK(r2.find("b") != std::string::npos);
        }

        SUBCASE("should reset state after spaces correctly") {
            std::string input = "123 abc ㄊㄞˊ";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("123") != std::string::npos);
            CHECK(r2.find("abc") != std::string::npos);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
        }
    }

    TEST_CASE("Malformed input handling") {
        SUBCASE("should handle null-like inputs gracefully") {
            CHECK_NOTHROW(convertBpmfToBraille(""));
            CHECK_NOTHROW(convertBrailleToBpmf(""));
        }

        SUBCASE("should handle very long digit sequences") {
            std::string longDigits = "";
            for (int i = 0; i < 10; ++i) {
                longDigits += "1234567890";
            }
            std::string r1 = convertBpmfToBraille(longDigits);
            CHECK(r1.find("⠼") != std::string::npos);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("123") != std::string::npos);
            CHECK(r2.find("890") != std::string::npos);
        }

        SUBCASE("should handle very long letter sequences") {
            std::string longLetters = "";
            for (int i = 0; i < 5; ++i) {
                longLetters += "abcdefghijklmnopqrstuvwxyz";
            }
            std::string r1 = convertBpmfToBraille(longLetters);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("abc") != std::string::npos);
            CHECK(r2.find("xyz") != std::string::npos);
        }

        SUBCASE("should handle mixed invalid characters") {
            std::string input = "ㄊㄞˊ@#$%^&*()_+";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("ㄊㄞˊ") != std::string::npos);
        }
    }

    TEST_CASE("Specific braille sequence tests") {
        SUBCASE("should convert digit prefixes correctly") {
            std::string input = "12.34";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1.find("⠼") != std::string::npos);
            CHECK(r1.find("⠨") != std::string::npos);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("12") != std::string::npos);
            CHECK(r2.find("34") != std::string::npos);
        }

        SUBCASE("should handle uppercase letter indicators") {
            std::string input = "AbC";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1.find("⠠") != std::string::npos);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == "AbC");
        }
    }

    TEST_CASE("Token conversion detailed tests") {
        SUBCASE("should handle token conversion with punctuation") {
            std::string braille = "⠋⠺⠂⠨⠴";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() > 0);
        }

        SUBCASE("should handle whitespace in token conversion") {
            std::string braille = "⠋⠺⠂   ⠋⠺⠂";
            auto tokens = convertBrailleToTokens(braille);
            CHECK(tokens.size() > 1);
        }
    }

    TEST_CASE("Real-world usage scenarios") {
        SUBCASE("should handle common phrase patterns") {
            std::string input = "ㄋㄧˇㄏㄠˇ";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == input);
        }

        SUBCASE("should handle date formats") {
            std::string input = "2024/12/31";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("2024") != std::string::npos);
            CHECK(r2.find("12") != std::string::npos);
            CHECK(r2.find("31") != std::string::npos);
        }

        SUBCASE("should handle percentage and temperature") {
            std::string input = "50% 25°C";
            std::string r1 = convertBpmfToBraille(input);
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2.find("50") != std::string::npos);
            CHECK(r2.find("25") != std::string::npos);
        }

        SUBCASE("should handle repeated conversion cycles") {
            std::string input = "ㄊㄞˊㄨㄢ123abc";
            std::string current = input;

            for (int i = 0; i < 5; ++i) {
                std::string braille = convertBpmfToBraille(current);
                current = convertBrailleToBpmf(braille);
            }

            CHECK(current.find("ㄊㄞˊㄨㄢ") != std::string::npos);
            CHECK(current.find("123") != std::string::npos);
            CHECK(current.find("abc") != std::string::npos);
        }

        SUBCASE("should handle empty string edge cases") {
            CHECK(convertBpmfToBraille("") == "");
            CHECK(convertBrailleToBpmf("") == "");
            CHECK(convertBrailleToTokens("").empty());
        }

        SUBCASE("should handle single space input") {
            std::string input = " ";
            std::string r1 = convertBpmfToBraille(input);
            CHECK(r1 == " ");
            std::string r2 = convertBrailleToBpmf(r1);
            CHECK(r2 == " ");
        }
    }
}
