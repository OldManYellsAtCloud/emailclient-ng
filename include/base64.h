#ifndef BASE64_H
#define BASE64_H

#include <stdint.h>
#include <string>
#include <vector>


namespace base64 {
    std::vector<uint8_t> decode_base64(const std::string& string, bool strict = true);
};

#endif // BASE64_H
