#ifndef IMAPMAILPARSER_H
#define IMAPMAILPARSER_H

#include "curlrequest.h"
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
#define CONTENT_DISPOSITION_HEADER_KEY  "Content-Disposition"

#define CONTENT_DISPOSITION_INLINE  "inline"

#define SMIME_SIGNED_HEADER  "This is an S/MIME signed message"

class ImapMailParser
{
private:
    std::unique_ptr<CurlRequest> cr;
    std::unique_ptr<CurlRequestScheduler> crs;

    std::string getHeader(const std::string& mailResponse);
    std::string getNormalizedHeader(const std::string& mailResponse);
    void normalizeHeader(std::string& header);

    std::string getBody(const std::string& mailResponse);


    std::map<std::string, std::string> parseHeader(const std::string& header);
    std::map<std::string, std::string> extractAndParseHeader(const std::string& mailResponse);
    std::pair<std::string, std::string> parseHeaderItem(const std::string& header_line);

    void decodeHeaderValues(std::map<std::string, std::string>& headerDict);
    std::string decodeSingleLine(const std::string& line);
    std::string extractEncodingTypeFromEncodedString(const std::string& s);
    std::string extractEncodedTextFromString(const std::string& s);

    bool isMultipart(const std::optional<std::string>& content_type);
    std::string getBoundaryFromHeaderValue(const std::optional<std::string>& headerValue);


    std::vector<std::string> splitBodyByBoundary(const std::string& body, const std::optional<std::string> boundary);
    std::string stripBodyFromMultipartHeader(const std::string& body);
    bool isBodyMultiPart(const std::string& body);
    bool isMessageSMIMESigned(const std::string& body);
    std::string stripBodyFromSMIMEHeader(const std::string& body);


    bool hasMailPartHeaders(const std::string& mailPartString);
    struct MailPart parseMailPart(const std::string& mailPartString, const std::string& globalEncoding, const std::string& globalContentType);
    void mergeInlineMailparts(std::vector<struct MailPart>& mailParts);

    ENCODING getMailPartEncoding(std::map<std::string, std::string>& headerDict, const std::string& globalEncoding);
    CONTENT_TYPE getMailPartContentType(std::map<std::string, std::string>& headerDict, const std::string& globalContentType);
    std::optional<std::string> getGlobalContentType(std::map<std::string, std::string>& headerDict);
    std::string getAttachmentName(std::map<std::string, std::string>& headerDict);

    std::pair<std::string, std::string> parseSenderNameAndEmail(const std::string& fromHeader);

    int extractUidFromResponse(const std::string& response);

public:
    ImapMailParser();
    Mail parseImapResponseToMail(ResponseContent rc, const std::string& folder);
};

#endif // IMAPMAILPARSER_H
