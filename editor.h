#ifndef EDITOR_H
#define EDITOR_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QTabWidget>
#include "searchandreplace.h"
#include "syntaxhighlighter.h"

class editor : public QWidget
{
    Q_OBJECT
public:
    explicit editor(QTabWidget *parent, QMainWindow* mainWindow);
    ~editor();

    // multiple methods that just call on the same for the main plaintTextEdit
    inline QString getText() const
    {
        return textEdit->toPlainText();
    }

    inline void setText(const QString& text)
    {
        textEdit->setPlainText(text);
    }
    // i guess compiler can access private members on inlined functions
    inline int blockCount() const
    {
        return textEdit->blockCount();
    }
    inline QPlainTextEdit* getPte() const
    {
        return textEdit;
    }

    void commentLines(); // base functionality after clicking Ctrl + /, checks what actions should be done
    void addComments(); // adds comments to blocks of code
    void removeComments(); // uncomments a block of comments

    void saveFile();
    void saveAs();

    // true means there are changes not saved in the file (for actions like opening another)
    inline bool unsavedChanges() const
    {
        return textEdit->document()->isModified();
    }

    inline QString fileName() const
    {
        return currentFile;
    }

    void openFile(QFile& file);

    inline void showSearchAndReplace()
    {
        this->searchAndReplace->showWidget();
    };

protected:
    void resizeEvent(QResizeEvent*) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void createLineNumbersOnFileOpen(int lineNumbers);

private slots:
    void synchronizeScrollBars(); // matches the scroll value for the text and the line numbers
    void calculateNumberOfLines(int newBlockCount);
    void updateTabTitle(); // add the * to the tab title if it has unsaved changes

private:
    inline static QFont font{"Courier"};

    int previousNumberOfLines = 0;
    QString currentFile; // can be const but do want to add functionality to changing the file of an open tab

    // If this goes after the 2 widgets that reference it, app crashes
    QPlainTextEdit *textEdit, *lineNumberTextEdit;

    QHBoxLayout *layout;

    QTabWidget* parent;
    QMainWindow* mainWindow; // non-owning pointer, no management
    std::unique_ptr<SearchAndReplace> searchAndReplace;

    std::unique_ptr<SyntaxHighlighter> syntaxHighlighter;

};


#endif // EDITOR_H
