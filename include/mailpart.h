#ifndef MAILPART_H
#define MAILPART_H

#include <string>

enum encoding {
    BASE64, QUOTED_PRINTABLE, NONE
};

enum content_type {
    TEXT, HTML, ATTACHMENT, OTHER
};

struct MailPart {
    std::string content;
    std::string name;
    content_type ct;
    encoding enc;
};

#endif // MAILPART_H
