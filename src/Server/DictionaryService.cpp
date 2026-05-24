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

#include "DictionaryService.h"

#define WIN32_LEAN_AND_MEAN
#include <shellapi.h>
#include <windows.h>

#include <fstream>

#include "UTF8Helper.h"
#include "UTFHelper.h"
#include "resource.h"

namespace McBopomofo {

// Minimal helper to extract string values from JSON
std::string ExtractJsonString(const std::string& json, const std::string& key,
                              size_t& pos) {
  std::string searchKey = "\"" + key + "\":";
  size_t keyPos = json.find(searchKey, pos);
  if (keyPos == std::string::npos) return "";

  size_t startQuote = json.find("\"", keyPos + searchKey.length());
  if (startQuote == std::string::npos) return "";

  size_t endQuote = startQuote + 1;
  bool inEscape = false;
  while (endQuote < json.length()) {
    if (json[endQuote] == '\\' && !inEscape) {
      inEscape = true;
    } else if (json[endQuote] == '"' && !inEscape) {
      break;
    } else {
      inEscape = false;
    }
    endQuote++;
  }

  if (endQuote == std::string::npos || endQuote >= json.length()) return "";

  pos = endQuote;
  std::string val = json.substr(startQuote + 1, endQuote - startQuote - 1);

  // basic unescape
  std::string unescaped;
  for (size_t i = 0; i < val.length(); ++i) {
    if (val[i] == '\\' && i + 1 < val.length()) {
      if (val[i + 1] == '"') {
        unescaped += '"';
        ++i;
      } else if (val[i + 1] == '\\') {
        unescaped += '\\';
        ++i;
      } else if (val[i + 1] == '/') {
        unescaped += '/';
        ++i;
      } else if (val[i + 1] == 'n') {
        unescaped += '\n';
        ++i;
      } else {
        unescaped += val[i];
      }
    } else {
      unescaped += val[i];
    }
  }
  return unescaped;
}

class CharacterInfoService : public DictionaryService {
 public:
  std::string name() const override {
    return Utf16ToUtf8(
        LoadLocalizedStringW(GetModuleHandle(NULL), IDS_CHARACTER_INFORMATION));
  }

  void lookup(std::string phrase, InputState* state, size_t /** serviceIndex */,
              const StateCallback& stateCallback) override {
    auto* selecting = dynamic_cast<InputStates::SelectingDictionary*>(state);
    if (selecting != nullptr) {
      auto copy =
          std::make_unique<InputStates::SelectingDictionary>(*selecting);
      auto newState = std::make_unique<InputStates::ShowingCharInfo>(
          std::move(copy), phrase);
      stateCallback(std::move(newState));
    }
  }

  std::string textForMenu(std::string /** selectedString */) const override {
    return name();
  }
};

class SimpleDictionaryService : public DictionaryService {
 public:
  SimpleDictionaryService(std::string name, std::string urlTemplate)
      : name_(std::move(name)), urlTemplate_(std::move(urlTemplate)) {}

  std::string name() const override { return name_; }

  void lookup(std::string phrase, InputState* /** state */,
              size_t /** serviceIndex */,
              const StateCallback& /** stateCallback */) override {
    std::string url = GetUrl(phrase);
    if (url.empty()) {
      return;
    }
    ShellExecuteW(NULL, L"open", Utf8ToUtf16(url).c_str(), NULL, NULL, SW_SHOW);
  }

  std::string textForMenu(std::string selectedString) const override {
    return name_;
  }

  std::string GetUrl(const std::string& phrase) const {
    std::string url = urlTemplate_;
    std::string encoded;
    // Basic URL encoding for UTF-8
    for (char c : phrase) {
      if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' ||
          c == '~') {
        encoded += c;
      } else {
        char buf[4];
        snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)c);
        encoded += buf;
      }
    }
    size_t pos = url.find("(encoded)");
    if (pos != std::string::npos) {
      url.replace(pos, 9, encoded);
    }
    return url;
  }

 private:
  std::string name_;
  std::string urlTemplate_;
};

DictionaryServices::DictionaryServices() {}
DictionaryServices::~DictionaryServices() {}

void DictionaryServices::load() {
  services_.clear();
  services_.push_back(std::make_unique<CharacterInfoService>());
  load("data\\dictionary_service.json");
}

void DictionaryServices::load(const std::string& jsonPath) {
  // services_.clear(); // Removed as we want to keep CharacterInfoService
  std::ifstream file(jsonPath);
  if (!file.is_open()) {
    return;
  }

  std::string json((std::istreambuf_iterator<char>(file)),
                   std::istreambuf_iterator<char>());
  size_t pos = 0;

  while (pos < json.length()) {
    std::string name = ExtractJsonString(json, "name", pos);
    if (name.empty()) break;

    std::string urlTemplate = ExtractJsonString(json, "url_template", pos);
    if (urlTemplate.empty()) break;

    services_.push_back(
        std::make_unique<SimpleDictionaryService>(name, urlTemplate));
  }
}

void DictionaryServices::lookup(std::string phrase, size_t serviceIndex,
                                InputState* state,
                                const StateCallback& stateCallback) {
  if (serviceIndex < services_.size()) {
    services_[serviceIndex]->lookup(phrase, state, serviceIndex, stateCallback);
  }
}

std::vector<std::string> DictionaryServices::menuForPhrase(
    const std::string& phrase) {
  std::vector<std::string> menu;
  for (const auto& svc : services_) {
    menu.push_back(svc->name());
  }
  return menu;
}

bool DictionaryServices::hasServices() { return !services_.empty(); }

}  // namespace McBopomofo
