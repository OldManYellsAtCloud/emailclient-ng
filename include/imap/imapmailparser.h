#ifndef IMAPMAILPARSER_H
#define IMAPMAILPARSER_H

#include "imaprequest.h"
#include "curlrequestscheduler.h"

#include "mail.h"

#define CRLF "\r\n"
#define SPACE " "
#define CRLF_SPACE "\r\n "
#define DOUBLE_CRLF "\r\n\r\n"
#define TAB '\t'
#define COLON ':'

#define PRINTABLE_QUOTED_ENCODING 'Q'

#define SUBJECT_HEADER_KEY  "Subject"
#define FROM_HEADER_KEY     "From"
#define DATE_HEADER_KEY     "Date"
#define CONTENT_TYPE_HEADER_KEY  "Content-Type"
#define CONTENT_TRANSFER_ENCODING_HEADER_KEY  "Content-Transfer-Encoding"

class ImapMailParser
{
private:
    std::unique_ptr<ImapRequest> ir;
    std::unique_ptr<CurlRequestScheduler> crs;

    std::string getHeader(const std::string& mailResponse);
    std::string getBody(const std::string& mailResponse);
    void normalizeHeader(std::string& header);
    std::map<std::string, std::string> parseHeader(const std::string& header);
    std::pair<std::string, std::string> parseHeaderItem(const std::string& header_line);

    void decodeHeaderValues(std::map<std::string, std::string>& headerDict);
    std::string decodeSingleLine(const std::string& line);
    std::string extractEncodingTypeFromEncodedString(const std::string& s);
    std::string extractEncodedTextFromString(const std::string& s);

    bool isMultipart(const std::string& content_type);
    std::string getBoundaryFromHeaderValue(const std::string& headerValue);

    std::vector<std::string> splitBodyByBoundary(const std::string& body, const std::string boundary);
    std::string stripBodyFromMultipartHeader(const std::string& body);
    bool isBodyMultiPart(const std::string& body);


    bool hasMailPartHeaders(const std::string& mailPartString);
    struct MailPart parseMailPart(const std::string& mailPartString, const std::string& globalEncoding, const std::string& globalContentType);

    ENCODING getMailPartEncoding(std::map<std::string, std::string>& headerDict, const std::string& globalEncoding);
    CONTENT_TYPE getMailPartContentType(std::map<std::string, std::string>& headerDict, const std::string& globalContentType);
    std::string getAttachmentName(std::map<std::string, std::string>& headerDict);

    int extractUidFromResponse(const std::string& response);

public:
    ImapMailParser();
    /*Mail fetchMail(std::string folder, int index);
    Mail fetchMail(std::string folder, std::string index);*/

    Mail parseImapResponseToMail(ResponseContent rc, const std::string& folder);
};

#endif // IMAPMAILPARSER_H
