#include "DictionaryService.h"
#include "UTF8Helper.h"

namespace McBopomofo {

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
    services_.clear();
    services_.push_back(std::make_unique<SimpleDictionaryService>("萌典", "https://www.moedict.tw/(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("萌典 (台語)", "https://www.moedict.tw/'(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("萌典 (客語)", "https://www.moedict.tw/:(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("Google", "https://www.google.com/search?q=(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("教育部重編國語辭典修訂本", "https://dict.revised.moe.edu.tw/search.jsp?md=1&word=(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("教育部國語辭典簡編本", "https://dict.concised.moe.edu.tw/search.jsp?md=1&word=(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("教育部成語典", "https://dict.idioms.moe.edu.tw/idiomList.jsp?idiom=(encoded)&qMd=0&qTp=1&qTp=2"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("教育部異體字字典", "https://dict.variants.moe.edu.tw/variants/rbt/query_result.do?from=standard&search_text=(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("教育部國字標準字體筆順學習網", "https://stroke-order.learningweb.moe.edu.tw/charactersQueryResult.do?words=(encoded)&lang=zh_TW&csrfPreventionSalt=null"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("教育部臺灣閩南語常用詞辭典", "https://sutian.moe.edu.tw/zh-hant/tshiau/?lui=tai_su&tsha=(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("Wiktionary", "https://zh.wiktionary.org/wiki/Special:Search?search=(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("康熙字典網上版", "https://www.kangxizidian.com/search/index.php?stype=Word&sword=(encoded)&detail=n"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("Unihan Database", "https://www.unicode.org/cgi-bin/GetUnihanData.pl?codepoint=(encoded)"));
    services_.push_back(std::make_unique<SimpleDictionaryService>("國際電腦漢字及異體字知識庫", "https://chardb.iis.sinica.edu.tw/search.jsp?q=(encoded)&stype=1"));
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
