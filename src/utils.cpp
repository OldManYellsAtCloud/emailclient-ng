#include <algorithm>
#include <chrono>
#include <utils.h>

#include "base64.h"
#include "loglibrary.h"
#include <filesystem>
#include <fstream>

std::string unquoteString(const std::string& s) {
    if (s[0] != DOUBLE_QUOTE )
        return s;

    std::string ret = s.substr(1);
    if (ret[ret.length() - 1] == DOUBLE_QUOTE)
        ret.erase(ret.length() - 1, 1);

    return ret;
}

const bool isStringEmpty(const std::string& s) {
    return s.find_first_not_of(WHITESPACE_CHARS) == std::string::npos;
}

std::vector<std::string> quoteAwareSplitString(const std::string& s, const std::string& delim){
    std::vector<std::string> ret;

    size_t cur_idx = 0, next_idx, quote_idx;

    while (cur_idx != std::string::npos){
        next_idx = s.find(delim, cur_idx + delim.length());
        quote_idx = s.find(DOUBLE_QUOTE, cur_idx + 1);
        if (quote_idx < next_idx){
            quote_idx = s.find(DOUBLE_QUOTE, quote_idx + 1);
            next_idx = s.find(delim, quote_idx);
        }

        ret.emplace_back(s.substr(cur_idx + delim.length(), next_idx - cur_idx - delim.length()));
        cur_idx = next_idx;
    }

    return ret;
}
std::vector<std::string> splitString(const std::string& s, const std::string& delim){
    size_t start = 0, end;

    std::vector<std::string> ret;
    while ((end = s.find(delim, start)) != std::string::npos){
        ret.push_back(s.substr(start, end - start));
        start = end + delim.size();
    }

    ret.push_back(s.substr(start));

    return ret;
}

std::string trim(const std::string& s){
    std::string ret = s;
    if (isStringEmpty(ret))
        return ret;

    while (ret.find_first_of(WHITESPACE_CHARS) == 0)
        ret.erase(0, 1);

    while (ret.find_last_of(WHITESPACE_CHARS) == ret.length() - 1)
        ret.erase(ret.length() - 1, 1);
    return ret;
}

bool isStringEncoded(const std::string& s){

    if (!s.starts_with(PQ_START))
        return false;
    if (!s.ends_with(PQ_END))
        return false;

    int num_of_qmark = std::count(s.begin(), s.end(), QUESTION_MARK);
    return num_of_qmark == 4;
}

std::string decodeBase64String(const std::string& s){
    std::vector<uint8_t> decoded = decodeBase64Data(s);
    std::string ret = reinterpret_cast<char*>(decoded.data());
    return ret;
}

std::vector<uint8_t> decodeBase64Data(const std::string &s)
{
    std::vector<uint8_t> decoded = base64::decode_base64(s, false);
    return decoded;
}

std::string decodeQuotedPrintableString(const std::string& s, const bool& convertUnderscoreToSpace){
    auto decodedData = decodeQuotedPrintableData(s, convertUnderscoreToSpace);
    std::string ret = std::string(reinterpret_cast<char*>(decodedData.data()));
    return ret;
}

std::vector<uint8_t> decodeQuotedPrintableData(const std::string &s, const bool &convertUnderscoreToSpace)
{
    std::vector<uint8_t> vec;
    std::string ret;
    std::string hextmp;

    uint8_t c;
    for (int i = 0; i < s.length(); ++i){

        switch (s[i]){
        case '=':
            hextmp = s.substr(i + 1, 2);
            if (hextmp[0] != 0xd && hextmp[1] != 0xa) { // skip CRLF
                try {
                    c = std::stoi(hextmp, 0, 16);
                    vec.push_back(c);
                } catch (std::exception e){
                    ERROR("Could not convert to hexadecimal number: {}", hextmp);
                }
            }
            i += 2;
            break;
        case '_':
            if (convertUnderscoreToSpace){
                vec.push_back(0x20);
                break;
            } // else fallthrough
        default:
            vec.push_back(s[i]);
        }
    }

    vec.push_back(0); // null terminator
    return vec;
}

std::string getImapDateStringFromNDaysAgo(const int &n)
{
    auto dateNow = std::chrono::system_clock::now();
    auto dateTarget = dateNow + std::chrono::days(-n);
    return std::format("{:%d-%b-%Y}", dateTarget);
}

// TODO: look up the RFC, how this supposed to be exactly
std::string decodeImapUTF7(const std::string &s)
{
    std::vector<uint8_t> out;
    for (int i = 0; i < s.length(); ++i){
        if (s[i] != '&'){
            out.push_back(s[i]);
            continue;
        }

        if (s[i] == '&' && i + 5 > s.length()){
            out.push_back(s[i]);
            continue;
        }

        std::string base64Encoded = s.substr(i + 1, 3);
        std::vector<uint8_t> fragment = base64::decode_base64(base64Encoded, false);
        out.insert(out.end(), fragment.begin() + 1, fragment.begin() + 2);
        i += 4;
    }
    return reinterpret_cast<char*>(out.data());
}


std::string decodeSender(const std::string &s)
{
    std::vector<std::string> splitSender = splitString(s, " ");
    std::string ret = "";
    bool convertUnderscoreToSpace = true;

    for (int i = 0; i < splitSender.size(); ++i){
        if (isStringEncoded(splitSender[i])){
            std::string encodedText = extractEncodedTextFromString(splitSender[i]);
            if (getEncodingType(s) == ENCODING::BASE64)
                splitSender[i] = decodeBase64String(encodedText);
            else
                splitSender[i] = decodeQuotedPrintableString(encodedText, convertUnderscoreToSpace);
        }
    }

    for (const std::string& st: splitSender){
        ret.append(st).append(" ");
    }

    ret.resize(ret.size() - 1);
    return ret;
}

std::string extractEncodingTypeFromEncodedString(const std::string& s){
    size_t start = s.find('?');
    start = s.find('?', start + 1);
    return s.substr(start + 1, 1);
}

std::string extractEncodedTextFromString(const std::string& s){
    size_t start = s.find('?');
    start = s.find('?', start + 1);
    start = s.find('?', start + 1);
    size_t end = s.find('?', start + 1);
    return s.substr(start + 1, end - start - 1);
}

ENCODING getEncodingType(const std::string &s)
{
    std::string encodingType = extractEncodingTypeFromEncodedString(s);
    if (toupper(encodingType[0]) == 'Q')
        return ENCODING::QUOTED_PRINTABLE;
    return ENCODING::BASE64;
}

std::vector<uint8_t> stringToUintVector(const std::string& s){
    std::vector<uint8_t> ret (s.begin(), s.end());
    return ret;
}

bool mailHasHTMLPart(const Mail& mail){
    for (const MailPart &mp: mail.parts)
        if (mp.ct == CONTENT_TYPE::HTML) return true;
    return false;
}

void writeMailToDisk(const Mail &mail, const std::string& folder)
{
    std::filesystem::remove_all(folder);
    std::filesystem::create_directory(folder);

    for (const MailPart& mailPart: mail.parts){
        std::vector<uint8_t> content;
        std::string fileName;
        switch (mailPart.enc){
        case ENCODING::BASE64:
            content = decodeBase64Data(mailPart.content);
            break;
        case ENCODING::QUOTED_PRINTABLE:
            content = decodeQuotedPrintableData(mailPart.content);
            break;
        case ENCODING::NONE:
            content = stringToUintVector(mailPart.content);
        }

        switch (mailPart.ct){
        case CONTENT_TYPE::HTML:
            fileName = "index.html";
            break;
        case CONTENT_TYPE::TEXT:
            if (mailHasHTMLPart(mail)) continue;
            fileName = "index.txt";
            break;
        default:
            fileName = mailPart.name;
        }

        bool addNewLine = std::filesystem::exists(folder + "/" + fileName);

        std::ofstream os (folder + "/" + fileName, std::ios_base::app);
        if (addNewLine)
            os << std::endl;

        os.write(reinterpret_cast<char*>(content.data()), content.size());
        os.close();
    }

}



std::string extractNameFromSender(const std::string &decodedSender)
{
    size_t startOfEmail = decodedSender.rfind("<");
    if (startOfEmail == std::string::npos || startOfEmail < 1)
        return decodedSender;

    std::string name = decodedSender.substr(0, startOfEmail - 1);
    return name;
}
