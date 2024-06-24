#ifndef DBEXCEPTION_H
#define DBEXCEPTION_H

#include <exception>
#include <string>

class DbException : public std::exception
{
private:
    std::string what_msg;
public:
    DbException(std::string s);
    const char *what() const noexcept override;
};

#endif // DBEXCEPTION_H
