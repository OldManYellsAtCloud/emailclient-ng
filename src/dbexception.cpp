#include "dbexception.h"

DbException::DbException(std::string s): what_msg {s} {}

const char *DbException::what() const noexcept
{
    return what_msg.c_str();
}
