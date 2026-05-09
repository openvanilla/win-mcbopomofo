#include "DictionaryService.h"
#include "UTF8Helper.h"

#include <fstream>

namespace McBopomofo {

// Minimal helper to extract string values from JSON
std::string ExtractJsonString(const std::string& json, const std::string& key, size_t& pos) {
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
            if (val[i+1] == '"') { unescaped += '"'; ++i; }
            else if (val[i+1] == '\\') { unescaped += '\\'; ++i; }
            else if (val[i+1] == '/') { unescaped += '/'; ++i; }
            else if (val[i+1] == 'n') { unescaped += '\n'; ++i; }
            else { unescaped += val[i]; }
        } else {
            unescaped += val[i];
        }
    }
    return unescaped;
}

class SimpleDictionaryService : public DictionaryService {

public:
    SimpleDictionaryService(std::string name, std::string urlTemplate)
        : name_(std::move(name)), urlTemplate_(std::move(urlTemplate)) {}

    std::string name() const override { return name_; }

    void lookup(std::string phrase, InputState* state, size_t serviceIndex,
                const StateCallback& stateCallback) override {
        // Handled by the client/server using textForMenu and opening the URL
    }

    std::string textForMenu(std::string selectedString) const override {
        return name_;
    }

    std::string GetUrl(const std::string& phrase) const {
        std::string url = urlTemplate_;
        std::string encoded;
        // Basic URL encoding for UTF-8
        for (char c : phrase) {
            if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
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
    load("data\\dictionary_service.json");
}

void DictionaryServices::load(const std::string& jsonPath) {
    services_.clear();

    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        return;
    }

    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    size_t pos = 0;

    while (pos < json.length()) {
        std::string name = ExtractJsonString(json, "name", pos);
        if (name.empty()) break;
        
        std::string urlTemplate = ExtractJsonString(json, "url_template", pos);
        if (urlTemplate.empty()) break;
        
        services_.push_back(std::make_unique<SimpleDictionaryService>(name, urlTemplate));
    }
}

void DictionaryServices::lookup(std::string phrase, size_t serviceIndex, InputState* state, const StateCallback& stateCallback) {
    if (serviceIndex < services_.size()) {
        auto* svc = dynamic_cast<SimpleDictionaryService*>(services_[serviceIndex].get());
        if (svc) {
            std::string url = svc->GetUrl(phrase);

            // Generate a state with a specific tag so the InputController knows to open the URL
            // and return to the previous state.
            // Since we can't easily execute ShellExecute here in the Engine, we'll rely on the caller.
        }
    }
}

std::vector<std::string> DictionaryServices::menuForPhrase(const std::string& phrase) {
    std::vector<std::string> menu;
    for (const auto& svc : services_) {
        menu.push_back(svc->name());
    }
    return menu;
}

std::string DictionaryServices::getUrlForPhrase(const std::string& phrase, size_t serviceIndex) const {
    if (serviceIndex < services_.size()) {
        auto* svc = dynamic_cast<SimpleDictionaryService*>(services_[serviceIndex].get());
        if (svc) {
            return svc->GetUrl(phrase);
        }
    }
    return "";
}

bool DictionaryServices::hasServices() {
    return !services_.empty();
}

} // namespace McBopomofo
