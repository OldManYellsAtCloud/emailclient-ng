#include "utils.h"
#include "gtest/gtest.h"
#include "imap/imaprequest.h"

#include "loglibrary.h"

#include <unistd.h>

#include <iostream>

#include "base64.h"
#include "imap/imapfetcher.h"

#include "dbmanager.h"

typedef std::basic_string<unsigned char> ustring;

#define HEADER_SUBJECT "Subject: "
#define HEADER_FROM    "From: "
#define HEADER_DATE    "Date: "
#define HEADER_CONTENT_ENCODING "Content-Transfer-Encoding: "

#define SUBJECT_HEADER_KEY  "Subject"
#define FROM_HEADER_KEY     "From"
#define DATE_HEADER_KEY     "Date"
#define CONTENT_TYPE_HEADER_KEY  "Content-Type"
#define CONTENT_TRANSFER_ENCODING_HEADER_KEY  "Content-Transfer-Encoding"

#define PRINTABLE_QUOTED_ENCODING  'Q'
#define BASE64_ENCODING  'B'

// according to the RFC space is also a valid boundary character, which is just plain dumb.
// Let's see first if I have ever received any mail with such a multipart-boundary, before supporting it...
#define VALID_BOUNDARY_CHARACTERS "abcdefghijkmnopqrstuvwxyzABCDEFGHIJKLMOPQRSTUVWXYZ1234567890'/+_,-.:=?()"


std::string extractCharsetFromEncodedString(const std::string& s){
    size_t start = s.find('?');
    size_t end = s.find('?', start + 1);
    return s.substr(start + 1, end - start - 1);
}
/*
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
*/

bool isMultipart(const std::string& content_type){
    return content_type.find("multipart") != std::string::npos;
}
/*
std::string decodeQuotedPrintableString(const std::string& s){
    std::string ret = "";
    std::string hextmp;

    QString qs;

    unsigned char c;
    for (int i = 0; i < s.length(); ++i){

        switch (s[i]){
        case '_':
            ret.append(1, 0x20);
            break;
        case '=':
            hextmp = s.substr(i + 1, 2);
            if (hextmp[0] != 0xd && hextmp[1] != 0xa) { // skip CRLF
                try {
                    c = std::stoi(hextmp, 0, 16);
                    ret.append(1, c);
                } catch (std::exception e){
                    std::cerr << "Could not convert: " << hextmp << std::endl;
                    exit(1);
                }
            }
            i += 2;
            break;
        default:
            ret.append(1, s[i]);
        }
    }

    // TODO: this really must go...
    // but currently that's the only somewhat "sane" way I found to represent funky characters consistently...
    qs = QString::fromStdString(ret);
    return qs.toStdString();
}
*/

std::string decodeSingleLine(const std::string& line){
    if (!isStringEncoded(line))
        return line;

    std::string charset = extractCharsetFromEncodedString(line);
    std::string encodingType = extractEncodingTypeFromEncodedString(line);
    std::string encodedText = extractEncodedTextFromString(line);
    std::string decodedText;

    if (toupper(encodingType[0]) == PRINTABLE_QUOTED_ENCODING)
        decodedText = decodeQuotedPrintableString(encodedText);
    else
        decodedText = decodeBase64String(encodedText);
    return decodedText;
}

std::string extractMultilineEncodedData(std::vector<std::string>::iterator it){
    std::string ret = "";
    while (isStringEncoded(*it)){
        ret += decodeSingleLine(trim(*it));
        ++it;
    }
    return ret;
}


std::pair<std::string, std::string> parseHeaderItem(const std::string& header_line){
    const char COLON2 = ':';
    size_t key_end = header_line.find(COLON2);
    std::pair<std::string, std::string> ret = std::make_pair("", "");

    if (key_end == std::string::npos){
        DBG("Not a valid header item: {}", header_line);
        return ret;
    }
    ret.first = header_line.substr(0, key_end);
    ret.second = header_line.substr(key_end + 2);
    return ret;
}

void decodeHeaderValues(std::map<std::string, std::string>& headerDict){
    for (auto it = headerDict.begin(); it != headerDict.end(); ++it){
        std::vector<std::string> splitValues = splitString(it->second, " ");

        it->second = "";
        for (int i = 0; i < splitValues.size(); ++i){
            if (isStringEncoded(splitValues[i])){
                splitValues[i] = decodeSingleLine(splitValues[i]);
            }
            it->second.append(splitValues[i]).append(" ");
        }

        it->second.erase(it->second.length() - 1, 1);
    }
}

std::map<std::string, std::string> parseHeader(const std::string& header){
    size_t start = 0, end = 0;
    std::string line, key, value;
    const std::string CRLF2 = "\r\n";
    std::map<std::string, std::string> headerDict;


    while (start < header.size() && end != std::string::npos){
        end = header.find(CRLF2, start);
        line = header.substr(start, end - start);
        std::pair<std::string, std::string> headerItem = parseHeaderItem(line);
        if (headerItem.first != "")
            headerDict[headerItem.first] = headerItem.second;

        start = end + CRLF2.size();
    }

    return headerDict;
}

void normalizeHeader(std::string& header){
    size_t start;
    while ((start = header.find('\t')) != std::string::npos)
        header.replace(start, 1, " ");
    while ((start = header.find("\r\n ")) != std::string::npos)
        header.erase(start, 2);
}


void parseBodyFolders(std::string folderResponse){
    std::vector<std::string> folders;
    std::vector<std::string> ret = splitString(folderResponse, "\r\n");
    for (std::string s: ret){
        if (!isStringEmpty(s)){
            std::vector<std::string> v = quoteAwareSplitString(s, " ");
            folders.emplace_back(unquoteString(v[v.size() - 1]));
        }
    }

    for (std::string &s: folders)
        std::cerr << "fuu: x" << s << "x" << std::endl;
}

int extractMessageNumber(const std::string& s){
    int ret = -1;
    std::vector<std::string> splitResponse = splitString(s, "\r\n");
    for (const std::string& s: splitResponse){
        if (s.find("EXISTS") != std::string::npos){
            std::vector<std::string> v = splitString(s, " ");
            try {
                ret = std::stoi(v[1]);
            } catch (std::exception e){
                std::cerr << "Could not parse this: " << v[1] <<", error: " << e.what() << std::endl;
            }
            break;
        }
    }
    return ret;
}

std::string getHeader(const std::string& mailResponse){
    size_t end = mailResponse.find("\r\n\r\n");
    if (end == std::string::npos){
        ERROR("Could not find end of header: {}...", mailResponse.substr(0, 100));
        return "?N/A?";
    }

    return mailResponse.substr(0, end);
}

std::string getBody(const std::string& mailResponse){
    size_t start = mailResponse.find("\r\n\r\n");
    if (start == std::string::npos){
        ERROR("Could not find end of header: {}...", mailResponse.substr(0, 100));
        return "?N/A?";
    }
    return mailResponse.substr(start + 4);
}

std::string getBoundaryFromHeaderValue(const std::string& headerValue){
    const std::string BOUNDARY = "boundary=";
    size_t start = headerValue.find(BOUNDARY);
    std::string ret = "?N/A?";
    if (start != std::string::npos){
        size_t end = headerValue.find('"', start + BOUNDARY.length() + 1);
        ret = headerValue.substr(start + BOUNDARY.length(), end - start - BOUNDARY.length() + 1);
        ret = unquoteString(ret);
        ret.insert(ret.begin(), 2, '-');
    }
    return ret;
}

std::vector<std::string> splitBodyByBoundary(const std::string& body, const std::string boundary){
    std::vector<std::string> splitMessages;

    if (boundary != "") {
        splitMessages = splitString(body, boundary);
        splitMessages.erase(splitMessages.begin());
        splitMessages.erase(splitMessages.end() - 1);
    } else {
        splitMessages.push_back(body);
    }

    return splitMessages;
}

std::string extractTransferEncodingFromMessage(const std::string& message){
    std::string ret = "";
    std::vector<std::string> messageLines = splitString(message, "\r\n");

    while (messageLines[0] == "")
        messageLines.erase(messageLines.begin());

    for (const std::string& line: messageLines){
        if (line == "")
            break;

        std::pair<std::string, std::string> headerItem = parseHeaderItem(line);
        if (headerItem.first == CONTENT_TRANSFER_ENCODING_HEADER_KEY){
            ret = headerItem.second;
            break;
        }
    }
    return ret;
}

void removeHeadersFromMessage(std::string& message){
    const std::string EMPTY_LINE = "\r\n\r\n";
    size_t end_of_header = message.find(EMPTY_LINE);
    message.erase(0, end_of_header + EMPTY_LINE.size());
}

void decodeMessages(std::vector<std::string>& messages, const std::string& globalEncoding){
    std::string messageEncoding = "";
    if (globalEncoding != "")
        messageEncoding = globalEncoding;

    for (int i = 0; i < messages.size(); ++i){
        if (messageEncoding == "")
            messageEncoding = extractTransferEncodingFromMessage(messages[i]);

        if (messageEncoding == "")
            continue;

        removeHeadersFromMessage(messages[i]);

        if (messageEncoding == "base64") {
            messages[i] = decodeBase64String(messages[i]);
        } else if (messageEncoding == "quoted-printable") {
            messages[i] = decodeQuotedPrintableString(messages[i]);
        }
    }
}

TEST(Generic_Suite, MailFetch2){
    ImapRequest ir{};
    CurlRequestScheduler crs{&ir};
    DbManager* dm = DbManager::getInstance();
    ImapFetcher imapFetcher {&crs, dm};

    imapFetcher.fetchNewEmails("INBOX");

    sleep(5);
    EXPECT_FALSE(false);
    /*ImapMailFetcher imf {};
    Mail m = imf.fetchMail("INBOX", 6650);
    DbManager dbm{};
    dbm.storeEmail(m);*/
}


TEST(Generic_Suite, MailFetch1){
    GTEST_SKIP();
    ImapRequest ir{};

    for (int i = 6632; i > 6631; --i){
        ResponseContent rc = ir.FETCH("INBOX", i, "BODY[]");

        std::string header = getHeader(rc.header.getResponse());
        normalizeHeader(header);

        std::map<std::string, std::string> headerDict = parseHeader(header);
        decodeHeaderValues(headerDict);

        std::cerr << rc.header.getResponse();
        std::string body = getBody(rc.header.getResponse());

        std::vector<std::string> splitResponse = splitString(rc.header.getResponse(), "\r\n");
        std::string subject = headerDict[SUBJECT_HEADER_KEY];
        std::string from = headerDict[FROM_HEADER_KEY];
        std::string date = headerDict[DATE_HEADER_KEY];
        std::string globalContentType;
        if (headerDict.contains(CONTENT_TYPE_HEADER_KEY))
            globalContentType = headerDict[CONTENT_TYPE_HEADER_KEY];

        //std::string contentType = getContentType(splitResponse);
        std::string boundary;
        if (isMultipart(globalContentType))
            boundary = getBoundaryFromHeaderValue(globalContentType);

        //std::string contentEncoding = getHeaderData(splitResponse, HEADER_CONTENT_ENCODING);
        std::string globalContentEncoding = headerDict[CONTENT_TRANSFER_ENCODING_HEADER_KEY];

        std::vector<std::string> mails = splitBodyByBoundary(body, boundary);
        decodeMessages(mails, globalContentEncoding);


        std::cerr << "i: " << i << std::endl;
        std::cerr << "subject (dec): " << subject << std::endl;
        std::cerr << "from: " << from << std::endl;

        std::cerr << "date: " << date << std::endl;
        std::cerr << "boundary: " << boundary << std::endl;
        std::cerr << "global content type: " << globalContentType << std::endl;
        std::cerr << "global content encoding: " << globalContentEncoding << std::endl;
        std::cerr << "mail #: " << mails.size() << std::endl;

        std::cerr << "------------------------" << std::endl;

        for (int i = 0; i < mails.size(); ++i){
            std::cerr << "mail no: " << i << std::endl;
            std::cerr << mails[i] << std::endl;
            std::cerr << "WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW" << std::endl;
        }

        std::cerr << "++++++++++++++++++++++++" << std::endl;

        sleep(1);
    }
}




TEST(Generic_Suite, MailNumberFetch2){
    GTEST_SKIP();
    ImapRequest ir{};
    ResponseContent rc = ir.EXAMINE("yocto");
    int i = extractMessageNumber(rc.body.getResponse());
    std::cerr << "msg num: " << i << std::endl; // 9273
    EXPECT_TRUE(false) << rc.header.getResponse();
}

TEST(Generic_Suite, MailNumberFetch1){
    GTEST_SKIP();
    ImapRequest ir{};
    ResponseContent rc = ir.EXAMINE("INBOX");
    int i = extractMessageNumber(rc.body.getResponse());
    std::cerr << "msg num: " << i << std::endl; // 6702
    EXPECT_TRUE(false) << rc.header.getResponse();
}

TEST(Generic_Suite, FolderFetch){
    GTEST_SKIP();
    ImapRequest ir{};
    ResponseContent rc = ir.LIST("\"\"", "*");
    parseBodyFolders(rc.body.getResponse());

    EXPECT_TRUE(false);
}

TEST(Base64, test1){
    GTEST_SKIP();
    std::string text = "TWFpbGluZyBsaXN0IHN1YnNjcmlwdGlvbiBjb25maXJtYXRpb24gbm90aWNlIGZvciBtYWlsaW5n"
                       "IGxpc3QKaW5mby1nbnUKCldlIGhhdmUgcmVjZWl2ZWQgYSByZXF1ZXN0IGZyb20gNTEuMTU0LjE0"
                       "NS4yMDUgZm9yIHN1YnNjcmlwdGlvbiBvZgp5b3VyIGVtYWlsIGFkZHJlc3MsICJza2FuZGlncmF1"
                       "bitnbnVAZ21haWwuY29tIiwgdG8gdGhlCmluZm8tZ251QGdudS5vcmcgbWFpbGluZyBsaXN0LiAg"
                       "VG8gY29uZmlybSB0aGF0IHlvdSB3YW50IHRvIGJlIGFkZGVkCnRvIHRoaXMgbWFpbGluZyBsaXN0"
                       "LCBzaW1wbHkgcmVwbHkgdG8gdGhpcyBtZXNzYWdlLCBrZWVwaW5nIHRoZQpTdWJqZWN0OiBoZWFk"
                       "ZXIgaW50YWN0LiAgT3IgdmlzaXQgdGhpcyB3ZWIgcGFnZToKCiAgICBodHRwczovL2xpc3RzLmdu"
                       "dS5vcmcvbWFpbG1hbi9jb25maXJtL2luZm8tZ251LzQ0YTU4NjcxNGY2ZWRmNGQ0ODVlYzdiOWNm"
                       "OTBhOWNmNTg4MTRiZjgKCgpPciBpbmNsdWRlIHRoZSBmb2xsb3dpbmcgbGluZSAtLSBhbmQgb25s"
                       "eSB0aGUgZm9sbG93aW5nIGxpbmUgLS0gaW4gYQptZXNzYWdlIHRvIGluZm8tZ251LXJlcXVlc3RA"
                       "Z251Lm9yZzoKCiAgICBjb25maXJtIDQ0YTU4NjcxNGY2ZWRmNGQ0ODVlYzdiOWNmOTBhOWNmNTg4"
                       "MTRiZjgKCk5vdGUgdGhhdCBzaW1wbHkgc2VuZGluZyBhIGByZXBseScgdG8gdGhpcyBtZXNzYWdl"
                       "IHNob3VsZCB3b3JrIGZyb20KbW9zdCBtYWlsIHJlYWRlcnMsIHNpbmNlIHRoYXQgdXN1YWxseSBs"
                       "ZWF2ZXMgdGhlIFN1YmplY3Q6IGxpbmUgaW4gdGhlCnJpZ2h0IGZvcm0gKGFkZGl0aW9uYWwgIlJl"
                       "OiIgdGV4dCBpbiB0aGUgU3ViamVjdDogaXMgb2theSkuCgpJZiB5b3UgZG8gbm90IHdpc2ggdG8g"
                       "YmUgc3Vic2NyaWJlZCB0byB0aGlzIGxpc3QsIHBsZWFzZSBzaW1wbHkKZGlzcmVnYXJkIHRoaXMg"
                       "bWVzc2FnZS4gIElmIHlvdSB0aGluayB5b3UgYXJlIGJlaW5nIG1hbGljaW91c2x5CnN1YnNjcmli"
                       "ZWQgdG8gdGhlIGxpc3QsIG9yIGhhdmUgYW55IG90aGVyIHF1ZXN0aW9ucywgc2VuZCB0aGVtIHRv"
                       "CmluZm8tZ251LW93bmVyQGdudS5vcmcuCg==";

    std::vector<uint8_t> vec = base64::decode_base64(text, false);
    std::string s = reinterpret_cast<char*>(vec.data());

}
