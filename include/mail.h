#ifndef MAIL_H
#define MAIL_H

#include <vector>
#include "mailpart.h"

struct Mail {
    int uid;
    std::string folder;
    std::string subject;
    std::string from;
    std::string date_string;
    std::vector<MailPart> parts;
    bool arePartsAvailable() {
        return parts.size() > 0;
    }
};

#endif // MAIL_H
