#ifndef SETTINGSHELPER_H
#define SETTINGSHELPER_H

#include <QSettings>

enum class autoSaveType{
    NeverAutoSave,
    SaveOnOpenNewFile,
    SaveAfterDuratoion
};
Q_DECLARE_METATYPE(autoSaveType)

class SettingsHelper
{
public:
    SettingsHelper() = delete;
public: // Object representations of the string value keys


private:
    QSettings settings{"Murad", "notepad"}; // thats the name for now i guess..
};

#endif // SETTINGSHELPER_H
