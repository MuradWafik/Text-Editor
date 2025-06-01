#include "syntaxhighlighter.h"
#include <QTextBlock>
#include <QTextCursor>
SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent) :
    QSyntaxHighlighter(parent)
{

    HighlightingRule rule;

    keywordFormat.setForeground(keywordColor);
    addRule(keywordsRegex, keywordFormat);

    functionFormat.setForeground(functionColor);
    addRule(functionRegex, functionFormat);

    classFormat.setForeground(classColor);
    addRule(classRegex, classFormat);

    singleLineStringFormat.setForeground(stringColor);
    addRule(singleLineStringRegex, singleLineStringFormat);

    commentFormat.setForeground(commentColor);
    // addRule(commentRegex, commentFormat);
    // highlightingRules.emplace_back(commentRegex, commentFormat);
}

// manual comment detection to avoid errors with comments embedded in strings and vice versa
void SyntaxHighlighter::highlightBlock(const QString &text)
{

    int commentStart = -1;
    bool inString = false;
    QChar stringChar;
    bool escape = false;

    for (int i = 0; i < text.length(); ++i){
        QChar c = text[i];

        if (inString){
            if (escape){
                escape = false;
            }
            else if (c == '\\') {
                escape = true;
            }
            else if (c == stringChar){
                inString = false;
            }
        }
        else{

            if (c == '"' || c == '\''){
                inString = true;
                stringChar = c;
            }
            else if (c == '#'){
                commentStart = i;
                break;
            }
        }
    }

    QVector<QPair<int, int>> protectedRanges;

    if (commentStart >= 0){
        int length = text.length() - commentStart;
        setFormat(commentStart, length, commentFormat); // apply the formatting to the comment
        protectedRanges.append(qMakePair(commentStart, text.length()));
    }

    // then go back to applying the regular rules
    for (const HighlightingRule &rule : std::as_const(highlightingRules)){
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            int start = match.capturedStart();
            int length = match.capturedLength();

            bool overlaps = false;
            for (const auto& range : protectedRanges) {
                if (start < range.second && (start + length) > range.first) {
                    overlaps = true; // skip the match if its within a comment already (dont apply highlighting on it)
                    break;
                }
            }
            if(!overlaps){
                setFormat(start, length, rule.format);
            }
        }
    }
}

