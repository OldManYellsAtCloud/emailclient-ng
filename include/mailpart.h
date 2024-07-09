#ifndef MAILPART_H
#define MAILPART_H

#include <string>


enum ENCODING {
    BASE64, QUOTED_PRINTABLE, NONE
};

enum CONTENT_TYPE {
    TEXT, HTML, ATTACHMENT, OTHER
};

struct MailPart {
    std::string content;
    std::string name;
    CONTENT_TYPE ct;
    ENCODING enc;
    bool is_inline;
};

#endif // MAILPART_H
