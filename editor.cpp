#include "editor.h"
#include <QScrollBar>
#include <QTextDocumentFragment>
#include <QTextBlock>
#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QMainWindow>

editor::editor(QTabWidget *parent, QMainWindow* mainWindow)
    : QWidget{parent},
    textEdit(new QPlainTextEdit(this)),
    lineNumberTextEdit(new QPlainTextEdit(this)),
    layout(new QHBoxLayout(this)),
    parent(parent),
    mainWindow(mainWindow),
    searchAndReplace(std::make_unique<SearchAndReplace>(this->textEdit)),
    syntaxHighlighter(std::make_unique<SyntaxHighlighter>(this->textEdit->document()))
// reminder** (The order they are initialized here does not matter, what matters is the order they are declared in the header
{
    font.setFixedPitch(true);
    // storing it into a single widget
    // basically reapplying all the values from the .ui file for the old widgets
    // (translating the markup to code here)
    lineNumberTextEdit->setMinimumSize(20, 40);
    lineNumberTextEdit->setMaximumSize(50, 16777215);
    lineNumberTextEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    // Has lines setting horizontal and vertical stretch to 0 within this policy, not sure of code alternative

    lineNumberTextEdit->setFont(font);
    // usually underlined, removing indefinetly

    lineNumberTextEdit->setFocusPolicy(Qt::NoFocus);
    lineNumberTextEdit->setContextMenuPolicy(Qt::NoContextMenu);
    lineNumberTextEdit->setInputMethodHints(Qt::ImhNone);

    lineNumberTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    lineNumberTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lineNumberTextEdit->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
    lineNumberTextEdit->setReadOnly(true);
    lineNumberTextEdit->setTextInteractionFlags(Qt::NoTextInteraction);

    lineNumberTextEdit->viewport()->setCursor(Qt::ArrowCursor); // stops cursor from changing when hovering over line number
    lineNumberTextEdit->setStyleSheet("QPlainTextEdit {background-color: black;}");
    lineNumberTextEdit->verticalScrollBar()->hide();

    lineNumberTextEdit->setLineWrapMode(QPlainTextEdit::NoWrap);


    textEdit->setMinimumSize(40, 40);
    textEdit->setFont(font);
    textEdit->setSizeAdjustPolicy(QAbstractScrollArea::SizeAdjustPolicy::AdjustToContents);
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    textEdit->setCenterOnScroll(false);
    textEdit->setBackgroundVisible(false);

    QFontMetrics metrics(font);
    int spaceWidth = metrics.horizontalAdvance(' ');
    textEdit->setTabStopDistance(4 * spaceWidth); // tab is 4 spaces, (currently it sets distance not 4 space presses)


    connect(lineNumberTextEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, &editor::synchronizeScrollBars);
    connect(textEdit->verticalScrollBar(), &QScrollBar::valueChanged, this, &editor::synchronizeScrollBars);
    // connects the scroll bars of the text box the user types in with the line number text

    connect(textEdit, &QPlainTextEdit::modificationChanged, this, &editor::updateTabTitle);

    connect(textEdit, &QPlainTextEdit::blockCountChanged, this, &editor::calculateNumberOfLines);

    // to fill out the entire tab like in the original layout
    layout->addWidget(lineNumberTextEdit);
    layout->addWidget(textEdit);
    this->setLayout(layout);
}

editor::~editor()
{
    if(unsavedChanges()){
        std::string text = "File (" +  this->currentFile.toStdString() +") has some unsaved changes, would you like to save them?";
        auto saved = QMessageBox::question(mainWindow,
                                           tr("Unsaved Changes"),
                                           tr(text.c_str()),
                                           QMessageBox::Save | QMessageBox::Discard, QMessageBox::Save);
        if(saved == QMessageBox::Save){
            saveFile();
        }

    }
}


void editor::synchronizeScrollBars()
{
    int textEditScrollValue = textEdit->verticalScrollBar()->value();
    int lineNumScrollValue = lineNumberTextEdit->verticalScrollBar()->value();

    if (textEditScrollValue != lineNumScrollValue) {
        lineNumberTextEdit->verticalScrollBar()->setValue(textEditScrollValue);
        textEdit->verticalScrollBar()->setValue(textEditScrollValue);
    }
}

void editor::calculateNumberOfLines(int newBlockCount)
{
    if(previousNumberOfLines == 0) {
        return; // so it doesn't append on startup just a singular line with the full amount
    }

    if(previousNumberOfLines > newBlockCount) {
        // they deleted a line, also works for multiple lines since it does a loop
        int linesToRemove = previousNumberOfLines - newBlockCount;
        QTextCursor cursor(lineNumberTextEdit->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);

        for (int i = 0; i < linesToRemove; i++){
            cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
            cursor.deletePreviousChar();
        }

    }
    else{
        // they added one or more lines, append the new line numbers
        int linesToAdd = newBlockCount - previousNumberOfLines;
        for (int i = 1; i <= linesToAdd; i++){
            lineNumberTextEdit->appendPlainText(QString::number(previousNumberOfLines + i));
        }
    }

    previousNumberOfLines = newBlockCount;
    // updates the previous line numbers variable so that it can reflect later on what type of line change was done
}

void editor::createLineNumbersOnFileOpen(const int lineNumbers)
{

    lineNumberTextEdit->clear(); // clears the text in case user opens another file

    for(int thisLineNumber = 1; thisLineNumber <= lineNumbers; thisLineNumber++){

        lineNumberTextEdit->appendPlainText(QString::number(thisLineNumber));
    }
}

void editor::commentLines()
{
    /* this is a very uneleagant solution though, it modifies the users current cursor, and
     * if the document was unmodified, it disregards and isModified becomes true (just cause the colors changed)
     */
    auto textCursor = textEdit->textCursor();
    // how commenting out multiple lines works
    // multiple lines selected -> if they are all comments it uncomments
    // otherwise it adds another comment symbol, though this makes more sense with "//" coments and not "#"
    // additionally, if the user has no text selected, the comment toggle affects the start of the line,
    // otherwise it goes by each line of the selection

    if(textCursor.hasSelection()){
        // const int startIndex = textCursor.selectionStart();
        QString text = textCursor.selection().toPlainText();

        QStringList textSplit = text.split("\n");
        // qDebug() << textSplit.at(2);
        bool everyLineStartsWithComment = true; // assumes every line is a comment, then loops to check, if one doesnt, it breaks
        for(QString &line : textSplit){
            if(!line.startsWith("#")){
                everyLineStartsWithComment = false;
                break;
            }
        }
        if(everyLineStartsWithComment) removeComments();
        else addComments();

    }
    else{

        QString currentLine = textCursor.block().text();
        if(currentLine.startsWith("#")) {
            removeComments();
        }
        else {
            addComments();
        }
    }
}

void editor::addComments()
{
    auto textCursor = textEdit->textCursor();
    // case when not every line is a comment

    // original cursor positions to revert
    int start = textCursor.selectionStart();
    // int end = textCursor.selectionEnd();

    QString text = textCursor.selection().toPlainText();
    QStringList textSplit = text.split("\n");
    QStringList finalText;
    const QChar commentSymbol = '#';
    if(textCursor.hasSelection()){


        for(QString& line : textSplit){
            finalText << commentSymbol + line;
        }
        // turns the list back to a single string
        QString commentedText = finalText.join("\n");

        // replaces text
        textCursor.insertText(commentedText);

        // brings cursor back to original position
        textCursor.setPosition(start);
        textCursor.setPosition(start + commentedText.length(), QTextCursor::KeepAnchor);
        textEdit->setTextCursor(textCursor);
    }

    else{
        textCursor.movePosition(QTextCursor::StartOfLine);
        textCursor.insertText("#");
    }
}


void editor::removeComments()
{
    auto textCursor = textEdit->textCursor();
    if (textCursor.hasSelection()) {
        int start = textCursor.selectionStart();
        QString text = textCursor.selection().toPlainText();
        QStringList textSplit = text.split("\n");
        QStringList finalText;
        const QChar commentSymbol = '#';
        for(QString& line : textSplit){
            if (line.startsWith(commentSymbol)) {
                line.remove(0, 1);
            }
            finalText << line;
        }
        QString uncommentedText = finalText.join("\n");
        textCursor.insertText(uncommentedText);
        textCursor.setPosition(start);
        textCursor.setPosition(start + uncommentedText.length(), QTextCursor::KeepAnchor);
        textEdit->setTextCursor(textCursor);
    }
    else {
        textCursor.movePosition(QTextCursor::StartOfLine);
        QString currentLine = textCursor.block().text();

        if (currentLine.startsWith("#")) {
            textCursor.deleteChar();
        }
    }
}

void editor::saveFile()
{
    // Dont think this is applicable anymore, wont have anything to save
    /// if(currentFile.isEmpty()) {
    ///     saveAs();  // on cases where option is available with no file, calls to save as
    ///     return;
    /// }

    try {

        QFile file(currentFile);
        if(!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QString errorMessage{QString("Unable to Save File ") + file.errorString()};
            QMessageBox::warning(mainWindow,
                                 tr("Warning"),
                                 tr(errorMessage.toStdString().c_str())
                                 );
            return;
        }
        QString text = getText();
        QTextStream out(&file);
        out << text;

        file.close();
        textEdit->document()->setModified(false);

        // updateWindowTitle();
        updateTabTitle();

    }
    catch (const std::exception &e) {
        QMessageBox::warning(mainWindow,
                             tr("Warning"),
                             tr(e.what())
                             );
    }
}


void editor::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"));
    if (fileName.isEmpty()) {
        return;  // If the user cancels the save dialog, do nothing.
    }

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(mainWindow, tr("Warning"), "Can Not Save File: " + file.errorString());
        return;
    }

    currentFile = fileName;
    // TODO: reenable save in mainwindow file
    // this->ui->actionSave->setEnabled(true); // can save now since a file is selected

    QTextStream out(&file);
    QString currentText = textEdit->toPlainText();
    out << currentText;  // sets the text content of that file equal to the current text
    file.close();

    // updateWindowTitle();
    updateTabTitle();
}

void editor::openFile(QFile& file)
{
    currentFile = file.fileName();

    QTextStream in(&file);
    QString text = in.readAll();
    textEdit->setPlainText(text);

    previousNumberOfLines = this->textEdit->blockCount();
    // the number of lines for the line counter, also stores the variable to see if the change was line added or removed
    createLineNumbersOnFileOpen(previousNumberOfLines);

    textEdit->document()->setModified(false);
    // it seems that highlighting the text emits the textChanged signal (which caused the save question to always go off)

}

void editor::updateTabTitle()
{
    if(unsavedChanges()){
        QString unsavedChangesText{"*(" +currentFile + ")"};
        // parent->setTabText(parent->currentIndex(), unsavedChangesText);
        int index = parent->indexOf(this); // should be the current index always but for some reason it wasnt, maybe it was another part acting up, not this

        parent->setTabText(index, unsavedChangesText);
            // (tr(unsavedWindowTitle.toStdString().c_str()));
    }
    // else if(!this->ui->editorWidget->getPte()->document()->isModified() && windowTitle() != currentFile){
    else{
        int index = parent->indexOf(this);

        parent->setTabText(index, currentFile);
    }
}


void editor::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);

    // move search and replace widget to the top right
    const int margin = 10; // Margin from the top and right edges
    QPoint topRight = textEdit->rect().topRight();
    searchAndReplace->move(topRight.x()- searchAndReplace->width() - margin, topRight.y() + margin);
}

void editor::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Slash && event->modifiers().testFlag(Qt::ControlModifier)){
        commentLines();
    }

    // else if(event->key() == Qt::Key_P && event->modifiers().testFlag(Qt::ControlModifier)){
    //     // syntaxHighlighter.highlightText();
    //     syntaxHighlighter->highlightBlock(textEdit->toPlainText());
    // }
    else{
        QWidget::keyPressEvent(event);
    }
}
