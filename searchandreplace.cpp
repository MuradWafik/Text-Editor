#include "searchandreplace.h"
#include <QBoxLayout>
#include <QStyle>
// #include "ui_mainwindow.h"
#include <QApplication>

SearchAndReplace::SearchAndReplace(QPlainTextEdit* editor)
    : QDockWidget(editor),
    editor(editor)
{
    setupUI(); // makes the ui items and signal connections in constructor
    connectSignalsAndSlots();
    this->hide();

}
SearchAndReplace::~SearchAndReplace() {
    foundOccurrences.clear();
    delete isMatchWholeWord;
    delete isCaseSensitive;
}

void SearchAndReplace::setupUI(){
    // FULL NEW STRUCTURE
    //  Blank EMPTY widget -- search and replace container
    // QVbox  -- full layout parent
    //     QHbox	-- line edits and iterate layout
    //         QVBOX -- search and replace parent
    //             Search -- searchtextlineedit
    //             Replace -- replacetextlineedit

    //         Empty Widget -- iterator blank widget
    //             QVBOX --iterate words layout
    //                 Iterator label box
    //                 QHbox -- prevandnextbuttonslayout
    //                      Prev button
    //                      Next button

    //     Empty Widget -- bottomempty widget
    //         QHbox -- bottom layout
    //             Replace Button
    //             Another Qhbox aligned to right --checkboxesparent
    //                 Match whole word button
    //                 Case sensitive button


    // Ceates the ui objects in the structure above
    QFrame* searchAndReplaceContainer = new QFrame(this);
    searchAndReplaceContainer->setFrameShape(QFrame::Box);
    searchAndReplaceContainer->setAutoFillBackground(true);
    this->setWidget(searchAndReplaceContainer);


    QVBoxLayout* fullLayoutParent = new QVBoxLayout(searchAndReplaceContainer);

    QHBoxLayout* lineEditsAndIterateLayout = new QHBoxLayout;
    QVBoxLayout* searchAndReplaceParent = new QVBoxLayout;

    searchTextLineEdit = new QLineEdit;
    searchAndReplaceParent->addWidget(searchTextLineEdit);
    replaceTextLineEdit = new QLineEdit;
    searchAndReplaceParent->addWidget(replaceTextLineEdit);


    QFrame* iteratorBlankWidget = new QFrame;
    iteratorBlankWidget->setFrameShape(QFrame::Box);

    QVBoxLayout* iterateWordsLayout = new QVBoxLayout(iteratorBlankWidget);

    occurenceIteratorLabel = new QLabel("0 / 0");
    occurenceIteratorLabel->setAlignment(Qt::AlignCenter | Qt::AlignHCenter);
    occurenceIteratorLabel->setStyleSheet("QLabel{font-size: 20px;}");
    iterateWordsLayout->addWidget(occurenceIteratorLabel);


    QHBoxLayout* prevAndNextButtonsLayout = new QHBoxLayout;
    prevMatchButton = new QPushButton;
    connect(prevMatchButton, &QPushButton::clicked, this, &SearchAndReplace::goToPreviousSelection);
    nextMatchButton = new QPushButton;
    connect(nextMatchButton, &QPushButton::clicked, this, &SearchAndReplace::goToNextSelection);


    nextMatchButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
    prevMatchButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowBack));
    // means left unless the layout of language is right to left

    nextMatchButton->setStyleSheet("QPushButton:hover{ background-color: lightgray;}");

    prevMatchButton->setStyleSheet("QPushButton:hover{ background-color: lightgray;}");
    prevAndNextButtonsLayout->addWidget(prevMatchButton);
    prevAndNextButtonsLayout->addWidget(nextMatchButton);


    QFrame* bottomEmptyWidget = new QFrame;
    bottomEmptyWidget->setAutoFillBackground(true);
    bottomEmptyWidget->setFrameShape(QFrame::Box);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomEmptyWidget);

    replaceTextButton = new QPushButton("Replace All");

    QHBoxLayout* checkBoxesParent = new QHBoxLayout;

    isCaseSensitive = new QCheckBox;
    checkBoxesParent->addWidget(isCaseSensitive);


    // checkBoxesParent->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    isMatchWholeWord = new QCheckBox;
    checkBoxesParent->addWidget(isMatchWholeWord);

    iterateWordsLayout->addLayout(prevAndNextButtonsLayout);

    bottomLayout->addWidget(replaceTextButton);
    bottomLayout->addLayout(checkBoxesParent);


    lineEditsAndIterateLayout->addLayout(searchAndReplaceParent);
    lineEditsAndIterateLayout->addWidget(iteratorBlankWidget);

    fullLayoutParent->addLayout(lineEditsAndIterateLayout);
    fullLayoutParent->addWidget(bottomEmptyWidget);


    isCaseSensitive->setToolTip("Case Sensitive");
    QIcon caseSensitveIcon = QIcon(":/imgs/gui-case-sensitive.svg");
    isCaseSensitive->setIcon(caseSensitveIcon);


    isMatchWholeWord->setToolTip("Match Whole Word");
    QIcon matchWholeIcon = QIcon(":/imgs/whole-word.svg");
    isMatchWholeWord->setIcon(matchWholeIcon);

    qApp->setStyleSheet(" QCheckBox:hover{background-color: light-gray;}");

    searchTextLineEdit->setPlaceholderText("Search Text");
    searchTextLineEdit->setFixedSize(160, 20);
    replaceTextLineEdit->setPlaceholderText("Replace With");
    replaceTextLineEdit->setFixedSize(160, 20);
    replaceTextButton->setFixedSize(120, 20);


    searchAndReplaceParent->setSpacing(2);
    checkBoxesParent->setSpacing(2);

}

void SearchAndReplace::connectSignalsAndSlots() {

    connect(isMatchWholeWord, &QCheckBox::clicked, this, [this](){
        searchForText(searchTextLineEdit->text());
    });

    connect(isCaseSensitive, &QCheckBox::clicked, this, [this](){
        searchForText(searchTextLineEdit->text());
    }); // redoes the search if either button is clicked

    connect(replaceTextButton, &QPushButton::clicked, this, &SearchAndReplace::onReplaceClicked);

    connect(searchTextLineEdit, &QLineEdit::textEdited, this, [this]{
        searchForText(searchTextLineEdit->text());
    });
}

void SearchAndReplace::onReplaceClicked() {
    QString replaceText = replaceTextLineEdit->text();
    if (replaceText.isEmpty() || foundOccurrences.isEmpty()) {
        return;  // If the replacement text is empty or no occurrences found, do nothing.
    }

    QTextCursor cursor = editor->textCursor();
    cursor.beginEditBlock();  // Start a single undo block for the whole replacement operation.

    for (QTextCursor &occurrence : foundOccurrences) { // loops through the vector storing all instances of the items to removed
        occurrence.beginEditBlock();
        occurrence.insertText(replaceText);
        occurrence.endEditBlock();
    }

    cursor.endEditBlock();  // End the undo block.
    foundOccurrences.clear();  // Clear occurrences after replacement
}


void SearchAndReplace::searchForText(const QString& text){

    removeHighlights(); // removes any text that was previously highlighted
    foundOccurrences.clear(); // clears the vector storing all instances

    if (text.isEmpty()) {
        return; // returns if empty string
    }

    QTextDocument *document = editor->document();
    QTextCursor highlightCursor(document);
    QTextCursor cursor(document);

    cursor.beginEditBlock();

    QTextCharFormat plainFormat(highlightCursor.charFormat());
    QTextCharFormat colorFormat = plainFormat;
    colorFormat.setBackground(Qt::blue);

    QTextDocument::FindFlags flag;

    // or equals operator, if true it is added
    if (isCaseSensitive->isChecked()) flag |= QTextDocument::FindCaseSensitively;

    if (isMatchWholeWord->isChecked()) flag |= QTextDocument::FindWholeWords;

    while (!highlightCursor.isNull() && !highlightCursor.atEnd()){
        highlightCursor = document->find(text, highlightCursor, flag);

        if (!highlightCursor.isNull()){
            // highlightCursor.movePosition(QTextCursor::WordRight, QTextCursor::KeepAnchor);
            highlightCursor.mergeCharFormat(colorFormat);

            foundOccurrences.append(highlightCursor);
        }
    }
    cursor.endEditBlock();

    if(foundOccurrences.size() == 0) return; // if an item is found moves the cursor to the last item

    selectedOccurenceIndex = foundOccurrences.size();
    QString labelText = QString::number(selectedOccurenceIndex) + " / " + QString::number(foundOccurrences.size());
    occurenceIteratorLabel->setText(labelText);

    auto textcursor = editor->textCursor();
    textcursor.setPosition(foundOccurrences.at(selectedOccurenceIndex-1).position());
    editor->setTextCursor(textcursor);
};

void SearchAndReplace::removeHighlights(){
    QTextDocument *document = editor->document();
    QTextCursor cursor(document);
    cursor.beginEditBlock();

    QTextCharFormat plainFormat;
    plainFormat.setBackground(Qt::transparent); // Set background to transparent to remove highlight
    cursor.select(QTextCursor::Document);
    cursor.mergeCharFormat(plainFormat);

    cursor.endEditBlock();
}

void SearchAndReplace::showWidget(){
    this->showNormal();
    searchTextLineEdit->setFocus();
}

void SearchAndReplace::goToPreviousSelection() {
    // same as go to next
    QTextCursor cursor = foundOccurrences.at(selectedOccurenceIndex-1);
    QTextCharFormat oldCursorPosFormat;
    oldCursorPosFormat.setBackground(Qt::blue);
    cursor.setCharFormat(oldCursorPosFormat);

    if (selectedOccurenceIndex == 1) selectedOccurenceIndex = foundOccurrences.size(); // loops it around to restart at the top
    else selectedOccurenceIndex--;

    QTextCursor currentCursor = foundOccurrences.at(selectedOccurenceIndex - 1);
    QTextCharFormat currentSelectionFormat;
    currentSelectionFormat.setBackground(Qt::yellow);
    currentCursor.setCharFormat(currentSelectionFormat);

    QString text = QString::number(selectedOccurenceIndex) + " / " + QString::number(foundOccurrences.size());
    occurenceIteratorLabel->setText(text);


    editor->setTextCursor(currentCursor);
}

void SearchAndReplace::goToNextSelection() {

    // resets the color of the previously selected word back to blue
    QTextCursor cursor = foundOccurrences.at(selectedOccurenceIndex-1);
    QTextCharFormat oldCursorPosFormat;
    oldCursorPosFormat.setBackground(Qt::blue);
    cursor.setCharFormat(oldCursorPosFormat);

    if (selectedOccurenceIndex == foundOccurrences.size()) selectedOccurenceIndex = 1;
    else selectedOccurenceIndex++;

    // changes the current highlighted word color
    QTextCursor currentCursor = foundOccurrences.at(selectedOccurenceIndex - 1);
    QTextCharFormat currentSelectionFormat;
    currentSelectionFormat.setBackground(Qt::cyan);
    currentCursor.setCharFormat(currentSelectionFormat);


    QString text = QString::number(selectedOccurenceIndex) + " / " + QString::number(foundOccurrences.size());
    occurenceIteratorLabel->setText(text);

    // sets text cursor to the current selection
    editor->setTextCursor(currentCursor);
}

void SearchAndReplace::closeEvent(QCloseEvent *event)
{
    removeHighlights(); // closed highlights then calls on the default close event
    QDockWidget::closeEvent(event);
}
