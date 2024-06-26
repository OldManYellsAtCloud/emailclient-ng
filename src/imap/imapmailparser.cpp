#include "imap/imapmailparser.h"
#include "loglibrary.h"
#include "utils.h"

ImapMailParser::ImapMailParser() {
    ir = std::make_unique<ImapRequest>();
    crs = std::make_unique<CurlRequestScheduler>(ir.get());
}

Mail ImapMailParser::parseImapResponseToMail(ResponseContent rc, const std::string& folder)
{
    Mail mail;
    std::string header = getHeader(rc.header.getResponse());
    normalizeHeader(header);
    std::map<std::string, std::string> headerDict = parseHeader(header);

    std::string body = getBody(rc.header.getResponse());

    std::string globalContentType;
    if (headerDict.contains(CONTENT_TYPE_HEADER_KEY))
        globalContentType = headerDict[CONTENT_TYPE_HEADER_KEY];

    std::string boundary = "";

    if (isBodyMultiPart(body)){
        std::string mailHeader = getHeader(body);
        normalizeHeader(mailHeader);
        std::map<std::string, std::string> mailHeaderDict = parseHeader(mailHeader);
        if (mailHeaderDict.contains(CONTENT_TYPE_HEADER_KEY))
            boundary = getBoundaryFromHeaderValue(mailHeaderDict[CONTENT_TYPE_HEADER_KEY]);
        body = stripBodyFromMultipartHeader(body);
    } else if (isMultipart(globalContentType)) {
        boundary = getBoundaryFromHeaderValue(globalContentType);
    }

    std::string globalContentEncoding = "";
    if (headerDict.contains(CONTENT_TRANSFER_ENCODING_HEADER_KEY))
        globalContentEncoding = headerDict[CONTENT_TRANSFER_ENCODING_HEADER_KEY];

    std::vector<std::string> mails = splitBodyByBoundary(body, boundary);
    std::vector<struct MailPart> mailParts;
    for (const std::string& s: mails){
        mailParts.push_back(parseMailPart(s, globalContentEncoding, globalContentType));
    }

    mail.uid = extractUidFromResponse(rc.header.getResponse());
    mail.folder = folder;
    mail.subject = headerDict[SUBJECT_HEADER_KEY];
    mail.from = headerDict[FROM_HEADER_KEY];
    mail.date_string = headerDict[DATE_HEADER_KEY];
    mail.parts = mailParts;

    return mail;
}


std::string ImapMailParser::getHeader(const std::string& mailResponse){
    size_t end = mailResponse.find(DOUBLE_CRLF);
    if (end == std::string::npos){
        ERROR("Could not find end of header: {}...", mailResponse.substr(0, 100));
        return "?N/A?";
    }

    return trim(mailResponse.substr(0, end));
}

std::string ImapMailParser::getBody(const std::string& mailResponse){
    size_t start = mailResponse.find(DOUBLE_CRLF);
    if (start == std::string::npos){
        ERROR("Could not find end of header: {}...", mailResponse.substr(0, 100));
        return "?N/A?";
    }

    std::string fullBody = trim(mailResponse.substr(start + sizeof(DOUBLE_CRLF) - 1));
    return fullBody;
}

void ImapMailParser::normalizeHeader(std::string& header){
    size_t start;
    while ((start = header.find(TAB)) != std::string::npos)
        header.replace(start, 1, SPACE);
    while ((start = header.find(CRLF_SPACE)) != std::string::npos)
        header.erase(start, 2);
}

std::map<std::string, std::string> ImapMailParser::parseHeader(const std::string& header){
    size_t start = 0, end = 0;
    std::string line, key, value;
    std::map<std::string, std::string> headerDict;


    while (start < header.size() && end != std::string::npos){
        end = header.find(CRLF, start);
        line = header.substr(start, end - start);
        std::pair<std::string, std::string> headerItem = parseHeaderItem(line);
        if (headerItem.first != "")
            headerDict[headerItem.first] = headerItem.second;

        start = end + sizeof(CRLF) - 1;
    }

    return headerDict;
}

std::pair<std::string, std::string> ImapMailParser::parseHeaderItem(const std::string& header_line){
    size_t key_end = header_line.find(COLON);
    std::pair<std::string, std::string> ret = std::make_pair("", "");

    if (key_end == std::string::npos){
        DBG("Not a valid header item: {}", header_line);
        return ret;
    }
    ret.first = header_line.substr(0, key_end);
    if (key_end < header_line.size() - 3)
        ret.second = header_line.substr(key_end + 2);
    return ret;
}

void ImapMailParser::decodeHeaderValues(std::map<std::string, std::string>& headerDict){
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

std::string ImapMailParser::decodeSingleLine(const std::string& line){
    std::string encodingType = extractEncodingTypeFromEncodedString(line);
    std::string encodedText = extractEncodedTextFromString(line);
    std::string decodedText;

    if (toupper(encodingType[0]) == PRINTABLE_QUOTED_ENCODING)
        decodedText = decodeQuotedPrintableString(encodedText);
    else
        decodedText = decodeBase64String(encodedText);
    return decodedText;
}

std::string ImapMailParser::extractEncodingTypeFromEncodedString(const std::string& s){
    size_t start = s.find(QUESTION_MARK);
    start = s.find(QUESTION_MARK, start + 1);
    return s.substr(start + 1, 1);
}

std::string ImapMailParser::extractEncodedTextFromString(const std::string& s){
    size_t start = s.find(QUESTION_MARK);
    start = s.find(QUESTION_MARK, start + 1);
    start = s.find(QUESTION_MARK, start + 1);
    size_t end = s.find(QUESTION_MARK, start + 1);
    return s.substr(start + 1, end - start - 1);
}

bool ImapMailParser::isMultipart(const std::string& content_type){
    return content_type.find("multipart") != std::string::npos;
}

std::string ImapMailParser::getBoundaryFromHeaderValue(const std::string& headerValue){
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

std::vector<std::string> ImapMailParser::splitBodyByBoundary(const std::string& body, std::string boundary){
    std::vector<std::string> splitMessages;

    if (boundary != "") {
        splitMessages = splitString(body, boundary);
        if (splitMessages.size() > 2) {
            splitMessages.erase(splitMessages.begin());
            splitMessages.erase(splitMessages.end() - 1);
        }
    } else {
        splitMessages.push_back(body);
    }

    return splitMessages;
}

std::string ImapMailParser::stripBodyFromMultipartHeader(const std::string &body)
{
    std::string ret = getBody(body);
    size_t endOfBody = body.find_last_of(DOUBLE_CRLF);
    return ret.substr(0, endOfBody);
}

bool ImapMailParser::isBodyMultiPart(const std::string &body)
{
//    std::string trimmedBody = trim(body);
    size_t startOfBody = body.find(CRLF);
    std::string bodyWithoutFirstBoundary = body.substr(startOfBody + 1);
    if (!hasMailPartHeaders(bodyWithoutFirstBoundary))
        return false;

    std::string header = getHeader(bodyWithoutFirstBoundary);
    normalizeHeader(header);
    std::map<std::string, std::string> headerDict = parseHeader(header);
    if (!headerDict.contains(CONTENT_TYPE_HEADER_KEY))
        return false;

    std::string contentType = headerDict[CONTENT_TYPE_HEADER_KEY];
    return isMultipart(contentType);
}

bool ImapMailParser::hasMailPartHeaders(const std::string &mailPartString)
{
    std::vector<std::string> mailPartLines = splitString(mailPartString, CRLF);
    int firstNotEmptyLineNo = -1;
    while (mailPartLines[++firstNotEmptyLineNo] == "" && firstNotEmptyLineNo < mailPartLines.size() - 1);
    if (firstNotEmptyLineNo >= mailPartLines.size())
        return false;

    std::pair<std::string, std::string> firstPossibleHeader = parseHeaderItem(mailPartLines[firstNotEmptyLineNo]);
    bool isKeyValid = firstPossibleHeader.first.find(' ') == std::string::npos; // keys should have no space. If it has a space, it is not a header.

    return !firstPossibleHeader.first.empty() && isKeyValid;
}

MailPart ImapMailParser::parseMailPart(const std::string &mailPartString, const std::string& globalEncoding, const std::string& globalContentType)
{
    MailPart ret;
    std::map<std::string, std::string> headerDict;
    std::string body;

    if (hasMailPartHeaders(mailPartString)){
        std::string header = getHeader(mailPartString);
        normalizeHeader(header);
        body = getBody(mailPartString);
        headerDict = parseHeader(header);
    } else {
        body = mailPartString;
    }

    ret.enc = getMailPartEncoding(headerDict, globalEncoding);
    ret.ct = getMailPartContentType(headerDict, globalContentType);
    ret.content = body;

    if (ret.ct == CONTENT_TYPE::ATTACHMENT)
        ret.name = getAttachmentName(headerDict);

    return ret;
}

ENCODING ImapMailParser::getMailPartEncoding(std::map<std::string, std::string> &headerDict, const std::string &globalEncoding)
{
    std::string enc = "";
    if (!globalEncoding.empty()){
        enc = globalEncoding;
    } else if (headerDict.contains(CONTENT_TRANSFER_ENCODING_HEADER_KEY)){
        enc = headerDict[CONTENT_TRANSFER_ENCODING_HEADER_KEY];
    }

    std::transform(enc.begin(), enc.end(), enc.begin(), [](const char c){return std::tolower(c);});

    if (enc == "quoted-printable")
        return ENCODING::QUOTED_PRINTABLE;
    if (enc == "base64")
        return ENCODING::BASE64;
    return ENCODING::NONE;
}

/**
 * @brief ImapMailFetcher::getMailPartContentType
 * @param headerDict
 * @param globalContentType
 * @return
 */
CONTENT_TYPE ImapMailParser::getMailPartContentType(std::map<std::string, std::string> &headerDict, const std::string &globalContentType)
{
    std::string ct = "";

    if (headerDict.contains(CONTENT_TYPE_HEADER_KEY)){
        ct = headerDict[CONTENT_TYPE_HEADER_KEY];
    } else if (!globalContentType.empty()){
        ct = globalContentType;
    }

    if (ct.find("name=") != std::string::npos)
        return CONTENT_TYPE::ATTACHMENT;
    if (ct.find("text/plain") != std::string::npos)
        return CONTENT_TYPE::TEXT;
    if (ct.find("text/html") != std::string::npos)
        return CONTENT_TYPE::HTML;
    return CONTENT_TYPE::OTHER;
}

/**
 * @brief ImapMailFetcher::getAttachmentName Extract name of attachment from Content-Type header.
 * @param headerDict List of header elements.
 * @return Name of attachment from the Content-Type header. Empty string if the name is not present in the header.
 */
std::string ImapMailParser::getAttachmentName(std::map<std::string, std::string> &headerDict)
{
    if (!headerDict.contains(CONTENT_TYPE_HEADER_KEY))
        return "";

    std::string contentType = headerDict[CONTENT_TYPE_HEADER_KEY];
    const std::string ATTACHMENT_NAME_KEY = "name=\"";

    if (contentType.find(ATTACHMENT_NAME_KEY) == std::string::npos)
        return "";

    size_t start = contentType.find(ATTACHMENT_NAME_KEY) + ATTACHMENT_NAME_KEY.size();
    size_t end = contentType.find('"', start + 1);
    std::string attachmentName = contentType.substr(start, end - start);
    return attachmentName;
}

int ImapMailParser::extractUidFromResponse(const std::string &response)
{
    const std::string UID_START = "FETCH (UID ";
    std::vector<std::string> splitResponse = splitString(response, CRLF);
    int ret = 0;
    for (const std::string& line: splitResponse){
        if (line.find(UID_START) != std::string::npos){
            size_t start = line.find(UID_START) + UID_START.length();
            size_t end = line.find(' ', start);
            std::string uid_s = line.substr(start, end-start);
            try {
                ret = std::stoi(uid_s);
            } catch (std::exception e){
                ERROR("Could not extract uid from {}: {}", line, e.what());
            }
            break;
        }
    }
    return ret;
}
