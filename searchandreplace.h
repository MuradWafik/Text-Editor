#ifndef SEARCHANDREPLACE_H
#define SEARCHANDREPLACE_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QCheckBox>
#include <QFrame>


class SearchAndReplace : public QDockWidget
{
    // Q_OBJECT // this line was not included when i made a new class with the qt dialog, will omit for now


public:
    explicit SearchAndReplace(QPlainTextEdit* editor);
    ~SearchAndReplace();

protected slots:
    void closeEvent(QCloseEvent *event) override;


public slots:
    // funcs
    void searchForText(const QString& text);
    void setupUI();
    void connectSignalsAndSlots();
    void onReplaceClicked();
    void removeHighlights();
    void showWidget();
    void goToPreviousSelection();
    void goToNextSelection();

private:
    QLineEdit* searchTextLineEdit;
    QLineEdit* replaceTextLineEdit;
    QVector<QTextCursor> foundOccurrences;
    QCheckBox* isCaseSensitive;
    QCheckBox* isMatchWholeWord;
    QPushButton* replaceTextButton;
    QPlainTextEdit* editor;

    QPushButton* nextMatchButton;
    QPushButton* prevMatchButton;
    QLabel* occurenceIteratorLabel;
    int selectedOccurenceIndex;
};

#endif // SEARCHANDREPLACE_H
