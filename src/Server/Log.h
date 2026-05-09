// Copyright (c) 2022 and onwards The McBopomofo Authors.
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

#ifndef SRC_LOG_H_
#define SRC_LOG_H_

#include <sstream>
#include <string>

namespace McBopomofo {

class LogMessageContext {
public:
    LogMessageContext(const char* level);
    ~LogMessageContext();

    template <typename T>
    LogMessageContext& operator<<(const T& value) {
        stream_ << value;
        return *this;
    }

    // Overload for stream manipulators like std::endl
    typedef std::ostream& (*OStreamManipulator)(std::ostream&);
    LogMessageContext& operator<<(OStreamManipulator manip) {
        stream_ << manip;
        return *this;
    }

private:
    const char* level_;
    std::ostringstream stream_;
};

} // namespace McBopomofo

#define FCITX_MCBOPOMOFO_ERROR() ::McBopomofo::LogMessageContext("ERROR")
#define FCITX_MCBOPOMOFO_INFO()  ::McBopomofo::LogMessageContext("INFO")
#define FCITX_MCBOPOMOFO_WARN()  ::McBopomofo::LogMessageContext("WARN")
#define FCITX_MCBOPOMOFO_DEBUG() ::McBopomofo::LogMessageContext("DEBUG")

#endif // SRC_LOG_H_
