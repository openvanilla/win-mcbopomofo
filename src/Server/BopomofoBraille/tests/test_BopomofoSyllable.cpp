#include "doctest.h"
#include "BopomofoBraille/BopomofoSyllable.h"
#include <stdexcept>

using namespace McBopomofo;

TEST_SUITE("BopomofoSyllable") {

    TEST_CASE("BopomofoSyllable input validation") {
        SUBCASE("throws when Bopomofo string is empty") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf(""), std::runtime_error);
        }

        SUBCASE("throws when Bopomofo string is whitespace only") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("  "), std::runtime_error);
        }

        SUBCASE("throws when Bopomofo string contains Latin letters") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ABC"), std::runtime_error);
        }

        SUBCASE("throws when Bopomofo string mixes zhuyin and Latin characters") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄓB"), std::runtime_error);
        }

        SUBCASE("throws when Bopomofo string contains digits") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄓㄨ111"), std::runtime_error);
        }

        SUBCASE("throws when Braille string is empty") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille(""), std::runtime_error);
        }

        SUBCASE("throws when Braille string is whitespace only") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("  "), std::runtime_error);
        }

        SUBCASE("throws when Braille string contains Latin letters") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("ABC"), std::runtime_error);
        }
    }

    TEST_CASE("BopomofoSyllable conversions") {
        SUBCASE("should convert single consonant ㄘ to ⠚⠱⠄") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄘ");
            CHECK(syllable.braille == "⠚⠱⠄");
        }

        SUBCASE("should convert single consonant ㄙ to ⠑⠱⠄") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄙ");
            CHECK(syllable.braille == "⠑⠱⠄");
        }

        SUBCASE("should convert ⠁⠱⠄ to ㄓ") {
            auto syllable = BopomofoSyllable::fromBraille("⠁⠱⠄");
            CHECK(syllable.bpmf == "ㄓ");
        }

        SUBCASE("should convert ⠃⠱⠄ to ㄔ") {
            auto syllable = BopomofoSyllable::fromBraille("⠃⠱⠄");
            CHECK(syllable.bpmf == "ㄔ");
        }

        SUBCASE("should convert ⠊⠱⠄ to ㄕ") {
            auto syllable = BopomofoSyllable::fromBraille("⠊⠱⠄");
            CHECK(syllable.bpmf == "ㄕ");
        }

        SUBCASE("should throw error for invalid consonant sequence") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄅㄆ"), std::runtime_error);
        }

        SUBCASE("should throw error for consonant after vowel") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄧㄅ"), std::runtime_error);
        }

        SUBCASE("should convert ㄉㄠˋ to ⠙⠩⠐") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄉㄠˋ");
            CHECK(syllable.braille == "⠙⠩⠐");
        }

        SUBCASE("should convert ⠙⠩⠐ to ㄉㄠˋ") {
            auto syllable = BopomofoSyllable::fromBraille("⠙⠩⠐");
            CHECK(syllable.bpmf == "ㄉㄠˋ");
        }

        SUBCASE("should convert ㄓㄨㄥ to ⠁⠯⠄") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄓㄨㄥ");
            CHECK(syllable.braille == "⠁⠯⠄");
        }

        SUBCASE("should convert ⠁⠯⠄ to ㄓㄨㄥ") {
            auto syllable = BopomofoSyllable::fromBraille("⠁⠯⠄");
            CHECK(syllable.bpmf == "ㄓㄨㄥ");
        }

        SUBCASE("should convert ㄒㄧㄢˊ to ⠑⠞⠂") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄒㄧㄢˊ");
            CHECK(syllable.braille == "⠑⠞⠂");
        }

        SUBCASE("should convert ⠑⠞⠂ to ㄒㄧㄢˊ") {
            auto syllable = BopomofoSyllable::fromBraille("⠑⠞⠂");
            CHECK(syllable.bpmf == "ㄒㄧㄢˊ");
        }

        SUBCASE("should throw error for invalid Bopomofo") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf(""), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("invalid"), std::runtime_error);
        }

        SUBCASE("should throw error for invalid Braille") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille(""), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("invalid"), std::runtime_error);
        }

        SUBCASE("should convert ㄋㄧˇ to ⠝⠡⠈") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄋㄧˇ");
            CHECK(syllable.braille == "⠝⠡⠈");
        }

        SUBCASE("should convert ⠝⠡⠈ to ㄋㄧˇ") {
            auto syllable = BopomofoSyllable::fromBraille("⠝⠡⠈");
            CHECK(syllable.bpmf == "ㄋㄧˇ");
        }

        SUBCASE("should convert ㄨㄢ to ⠻⠄") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄨㄢ");
            CHECK(syllable.braille == "⠻⠄");
        }

        SUBCASE("should convert ㄧㄤ to ⠨⠄") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄧㄤ");
            CHECK(syllable.braille == "⠨⠄");
        }

        SUBCASE("should convert ⠨⠄ to ㄧㄤ") {
            auto syllable = BopomofoSyllable::fromBraille("⠨⠄");
            CHECK(syllable.bpmf == "ㄧㄤ");
        }

        SUBCASE("should convert ㄏㄢˇ to ⠗⠧⠈") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄏㄢˇ");
            CHECK(syllable.braille == "⠗⠧⠈");
        }
    }

    TEST_CASE("Additional BopomofoSyllable scenarios") {
        SUBCASE("should convert vowel-only syllables") {
            CHECK(BopomofoSyllable::fromBpmf("ㄚ").braille == "⠜⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄛ").braille == "⠣⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄜ").braille == "⠮⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄝ").braille == "⠢⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄞ").braille == "⠺⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄟ").braille == "⠴⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄠ").braille == "⠩⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄡ").braille == "⠷⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄢ").braille == "⠧⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄣ").braille == "⠥⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄤ").braille == "⠭⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄥ").braille == "⠵⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄦ").braille == "⠱⠄");
        }

        SUBCASE("should convert middle vowel-only syllables") {
            CHECK(BopomofoSyllable::fromBpmf("ㄧ").braille == "⠡⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨ").braille == "⠌⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄩ").braille == "⠳⠄");
        }

        SUBCASE("should convert all tones correctly") {
            CHECK(BopomofoSyllable::fromBpmf("ㄇㄚ").braille == "⠍⠜⠄"); // tone1
            CHECK(BopomofoSyllable::fromBpmf("ㄇㄚˊ").braille == "⠍⠜⠂"); // tone2
            CHECK(BopomofoSyllable::fromBpmf("ㄇㄚˇ").braille == "⠍⠜⠈"); // tone3
            CHECK(BopomofoSyllable::fromBpmf("ㄇㄚˋ").braille == "⠍⠜⠐"); // tone4
            CHECK(BopomofoSyllable::fromBpmf("ㄇㄚ˙").braille == "⠍⠜⠁"); // tone5
        }

        SUBCASE("should convert ㄧ combinations correctly") {
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄚ").braille == "⠾⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄛ").braille == "⠴⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄝ").braille == "⠬⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄞ").braille == "⠢⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄠ").braille == "⠪⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄡ").braille == "⠎⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄢ").braille == "⠞⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄣ").braille == "⠹⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄤ").braille == "⠨⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄧㄥ").braille == "⠽⠄");
        }

        SUBCASE("should convert ㄨ combinations correctly") {
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄚ").braille == "⠔⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄛ").braille == "⠒⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄞ").braille == "⠶⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄟ").braille == "⠫⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄢ").braille == "⠻⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄣ").braille == "⠿⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄤ").braille == "⠸⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄨㄥ").braille == "⠯⠄");
        }

        SUBCASE("should convert ㄩ combinations correctly") {
            CHECK(BopomofoSyllable::fromBpmf("ㄩㄝ").braille == "⠦⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄩㄢ").braille == "⠘⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄩㄣ").braille == "⠲⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄩㄥ").braille == "⠖⠄");
        }

        SUBCASE("should convert consonant + vowel combinations") {
            CHECK(BopomofoSyllable::fromBpmf("ㄅㄚ").braille == "⠕⠜⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄆㄛ").braille == "⠏⠣⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄇㄜ").braille == "⠍⠮⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄈㄞ").braille == "⠟⠺⠄");
        }

        SUBCASE("should convert consonant + middle vowel + vowel combinations") {
            CHECK(BopomofoSyllable::fromBpmf("ㄅㄧㄚ").braille == "⠕⠾⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄆㄨㄚ").braille == "⠏⠔⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄇㄩㄝ").braille == "⠍⠦⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄈㄧㄠ").braille == "⠟⠪⠄");
        }

        SUBCASE("should distinguish ambiguous consonants in Braille") {
            // ㄍ vs ㄐ
            CHECK(BopomofoSyllable::fromBpmf("ㄍㄚ").braille == "⠅⠜⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄐㄧㄚ").braille == "⠅⠾⠄");

            // ㄑ vs ㄘ
            CHECK(BopomofoSyllable::fromBpmf("ㄘㄚ").braille == "⠚⠜⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄑㄧㄚ").braille == "⠚⠾⠄");

            // ㄒ vs ㄙ
            CHECK(BopomofoSyllable::fromBpmf("ㄙㄚ").braille == "⠑⠜⠄");
            CHECK(BopomofoSyllable::fromBpmf("ㄒㄧㄚ").braille == "⠑⠾⠄");
        }

        SUBCASE("should convert from Braille to Bopomofo correctly") {
            CHECK(BopomofoSyllable::fromBraille("⠕⠜⠄").bpmf == "ㄅㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠏⠣⠂").bpmf == "ㄆㄛˊ");
            CHECK(BopomofoSyllable::fromBraille("⠍⠮⠈").bpmf == "ㄇㄜˇ");
            CHECK(BopomofoSyllable::fromBraille("⠟⠺⠐").bpmf == "ㄈㄞˋ");
            CHECK(BopomofoSyllable::fromBraille("⠙⠩⠁").bpmf == "ㄉㄠ˙");
        }

        SUBCASE("should throw errors for invalid combinations") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄅㄅ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄧㄧ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄚㄚ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄚˊˇ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄧㄇ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄚㄧ"), std::runtime_error);
        }

        SUBCASE("should throw errors for invalid Braille") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠕⠜"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠄⠄⠄"), std::runtime_error);
        }

        SUBCASE("should handle whitespace in input") {
            CHECK(BopomofoSyllable::fromBpmf(" ㄅㄚ ").braille == "⠕⠜⠄");
            CHECK(BopomofoSyllable::fromBraille(" ⠕⠜⠄ ").bpmf == "ㄅㄚ");
        }

        SUBCASE("should throw specific error for invalid Braille patterns") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠕⠱⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠑"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠚"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠅"), std::runtime_error);
        }

        SUBCASE("should throw errors for duplicate components in Braille") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠕⠏⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠡⠌⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠜⠄⠂"), std::runtime_error);
        }

        SUBCASE("should convert ⠱ at position 0 to ㄦ") {
            auto syllable = BopomofoSyllable::fromBraille("⠱⠄");
            CHECK(syllable.bpmf == "ㄦ");
        }

        SUBCASE("should correctly parse ambiguous Braille consonants") {
            CHECK(BopomofoSyllable::fromBraille("⠑⠾⠄").bpmf == "ㄒㄧㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠑⠜⠄").bpmf == "ㄙㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠚⠾⠄").bpmf == "ㄑㄧㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠚⠜⠄").bpmf == "ㄘㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠅⠾⠄").bpmf == "ㄐㄧㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠅⠜⠄").bpmf == "ㄍㄚ");
        }

        SUBCASE("should handle Braille combinations from various namespaces") {
            CHECK(BopomofoSyllable::fromBraille("⠾⠄").bpmf == "ㄧㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠬⠄").bpmf == "ㄧㄝ");
            CHECK(BopomofoSyllable::fromBraille("⠽⠄").bpmf == "ㄧㄥ");
            CHECK(BopomofoSyllable::fromBraille("⠔⠄").bpmf == "ㄨㄚ");
            CHECK(BopomofoSyllable::fromBraille("⠒⠄").bpmf == "ㄨㄛ");
            CHECK(BopomofoSyllable::fromBraille("⠯⠄").bpmf == "ㄨㄥ");
            CHECK(BopomofoSyllable::fromBraille("⠦⠄").bpmf == "ㄩㄝ");
            CHECK(BopomofoSyllable::fromBraille("⠘⠄").bpmf == "ㄩㄢ");
            CHECK(BopomofoSyllable::fromBraille("⠖⠄").bpmf == "ㄩㄥ");
        }

        SUBCASE("should throw error for invalid combination attempts") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄧㄜ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄨㄝ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ㄩㄚ"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBpmf("ˊ"), std::runtime_error);
        }
    }

    TEST_CASE("Braille validation edge cases") {
        SUBCASE("throws for duplicated consonant patterns") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠑⠑"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠅⠅"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠅⠡⠅⠡"), std::runtime_error);
        }

        SUBCASE("throws for incomplete Braille sequences") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠅"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠑"), std::runtime_error);
        }

        SUBCASE("throws when tone marker is missing") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠱⠱"), std::runtime_error);
        }

        SUBCASE("throws for multiple consonant symbols") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠕⠄⠕⠄"), std::runtime_error);
        }

        SUBCASE("throws for multiple middle vowel symbols") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠮⠮"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠣⠣"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠡⠡⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠌⠌⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠳⠳⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠌⠻⠄"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠡⠞⠄"), std::runtime_error);
        }

        SUBCASE("throws for invalid Braille characters") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠕⠐"), std::runtime_error);
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠝⠁"), std::runtime_error);
        }

        SUBCASE("throws for duplicated tone markers") {
            CHECK_THROWS_AS(BopomofoSyllable::fromBraille("⠁⠱⠄⠄"), std::runtime_error);
        }
    }

    TEST_CASE("BopomofoSyllable single consonant ㄭ/ㄦ Braille representation") {
        SUBCASE("should represent ㄭ as ㄦ in Braille") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄓ");
            CHECK(syllable.braille == "⠁⠱⠄");
        }
    }

    TEST_CASE("BopomofoSyllable ASCII Braille") {
        SUBCASE("converts simple syllables from Bopomofo to ASCII Braille") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄉㄠˋ", BrailleType::ASCII);
            CHECK(syllable.bpmf == "ㄉㄠˋ");
            CHECK(syllable.braille == "d%\"");
            CHECK(syllable.type == BrailleType::ASCII);
        }

        SUBCASE("converts combined syllables from Bopomofo to ASCII Braille") {
            auto syllable = BopomofoSyllable::fromBpmf("ㄒㄧㄢˊ", BrailleType::ASCII);
            CHECK(syllable.bpmf == "ㄒㄧㄢˊ");
            CHECK(syllable.braille == "et1");
            CHECK(syllable.type == BrailleType::ASCII);
        }

        SUBCASE("converts ASCII Braille back to Bopomofo") {
            auto syllable = BopomofoSyllable::fromBraille("et1", BrailleType::ASCII);
            CHECK(syllable.bpmf == "ㄒㄧㄢˊ");
            CHECK(syllable.braille == "et1");
            CHECK(syllable.type == BrailleType::ASCII);
        }

        SUBCASE("disambiguates ASCII consonants using yi and yu connections") {
            CHECK(BopomofoSyllable::fromBraille("k)'", BrailleType::ASCII).bpmf == "ㄐㄧㄚ");
            CHECK(BopomofoSyllable::fromBraille("k>'", BrailleType::ASCII).bpmf == "ㄍㄚ");
        }
    }
}
