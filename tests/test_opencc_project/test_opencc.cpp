#include <iostream>
#include "../third_party/OpenCC/src/SimpleConverter.hpp"
int main() { try { opencc::SimpleConverter converter("../build_new_2/bin/data/opencc/tw2s.json"); std::cout << converter.Convert("臺灣") << std::endl; } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << std::endl; } return 0; }
