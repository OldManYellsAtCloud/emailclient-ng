#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include "mail.h"

#define WHITESPACE_CHARS " \r\n\t"
#define DOUBLE_QUOTE '"'
#define QUESTION_MARK '?'
// printable-quoted
#define PQ_START "=?"
#define PQ_END "?="

#define TARGET_FOLDER "/mymails"

std::string unquoteString(const std::string& s);
const bool isStringEmpty(const std::string& s);
std::vector<std::string> splitString(const std::string& s, const std::string& delim);
std::vector<std::string> quoteAwareSplitString(const std::string& s, const std::string& delim);
std::string trim(const std::string& s);
bool isStringEncoded(const std::string& s);
std::string decodeBase64String(const std::string& s);
std::string decodeQuotedPrintableString(const std::string& s, const bool& convertUnderscoreToSpace = false);
std::string decodeImapUTF7(const std::string &s);
std::string decodeSender(const std::string &s);
std::string getImapDateStringFromNDaysAgo(const int& n);
ENCODING getEncodingType(const std::string& s);
std::string extractEncodingTypeFromEncodedString(const std::string& s);
std::string extractEncodedTextFromString(const std::string& s);
void writeMailToDisk(const Mail& mail);

#endif // UTILS_H
