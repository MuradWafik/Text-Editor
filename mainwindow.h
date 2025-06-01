#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QMessageBox>
#include <QScrollBar>
#include <QDebug>
#include <QProcess>
#include <QTreeView>
#include <QFileSystemModel>
#include <QTextDocumentFragment>
#include "editor.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void getAllFilesInDirectory();
    void getAllFilesInDirectory(const QString &directory);
    void setUIChanges();

    void initTerminalBox(const QString& path);
    void connectSignals();

    void deleteAllTabs();

private slots:
    void openFileAction();

    void updateStatusBarCursorPosition(); // update the text of Line Number and Coloumn number on the bottom status bar

    void on_StdoutAvailable(); // terminal output
    void on_StderrAvailable(); // errors from the terminal

    void writeToTerminal(); // pressing return in the line edit..

    void runButton();
    void showTerminal();

    void openFileWhileEditing(const QString& filePath);

    bool openFile(const QString &filePath);

    void updateTerminalAndOutput(const QString& path);

    void openFolderDialog();

    void showCustomContextMenu(const QPoint &pos);

    void createPythonFile(const QPoint &clickPoint);
    void createTextFile(const QPoint &clickPoint);

    void newPythonFile();

    void newTextFile();

private:
    Ui::MainWindow *ui;
    QProcess *process;

    QFileSystemModel *fileModel; // the file explorer  on the left for treeview

    editor* openEditor = nullptr;

    QLabel* lineAndColStatusLabel;

    QDir currentDirectory;

    // QLabel* searchAndReplaceStatusLabel; // the bottom status bar for text occurunces replaced, i gueess disregard for now?

};
#endif // MAINWINDOW_H
