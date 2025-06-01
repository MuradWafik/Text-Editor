#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <QFont>
namespace util{
    inline static QString getShellCommand()
    {
    // differentiates the terminial start based on the operating system
        #ifdef _WIN32 // maybe change to user choice, maybe could use powershell
            return "cmd.exe";
        #else
            return "/bin/sh";
        #endif
    }

    // the prefix of the command to run the python file (the interpreter)
    inline static QString getPythonRunCommand()
    {
    #ifdef _WIN32
        return "python";
    #else
        return "python3";
    #endif
    }
}

#endif // UTIL_H
