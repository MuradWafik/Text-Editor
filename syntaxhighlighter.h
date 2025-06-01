#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QStringView>
#include <QRegularExpression>
#include <QColor>
#include <QSyntaxHighlighter>
#include <QPlainTextEdit>

// its now gonna follow an approach pretty similar to qt's example implementation on their docs
class SyntaxHighlighter : public QSyntaxHighlighter
{
public:
    SyntaxHighlighter(QTextDocument* parent);
protected:
    void highlightBlock(const QString& text) override;
public:
    // switch to file later on to allow it to change (themes...)
    inline static QColor commentColor{22, 120, 13};
    inline static QColor stringColor{120, 88, 13};
    inline static QColor keywordColor{28, 76, 189};
    inline static QColor classColor{186, 194, 31};
    inline static QColor functionColor{145, 20, 47};
private:

    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    inline void addRule(const QRegularExpression& regexp,  QTextCharFormat& format, bool bold = true){
        if(bold) format.setFontWeight(QFont::Bold);
        HighlightingRule rule{regexp, format};

        highlightingRules.append(rule);
    }

    inline bool isOverlapping(int start, int length, const QVector<QPair<int, int>> &ranges) const
    {
        int end = start + length;
        for (const auto &range : ranges) {
            int rangeStart = range.first;
            int rangeEnd = range.second;
            if (start < rangeEnd && end > rangeStart) {
                return true;
            }
        }
        return false;
    }

private:
    QTextCharFormat keywordFormat;
    const inline static QRegularExpression keywordsRegex{R"(\b(def|class|if|else|elif|return|import|from|while|for|in|try|except|with|as|pass|yield|async|await|None|True|False)\b)"};

    QTextCharFormat functionFormat;
    const inline static QRegularExpression functionRegex{R"(\bdef\s+([a-zA-Z_][a-zA-Z0-9_]*)\b)"};

    QTextCharFormat classFormat;
    // const inline static QRegularExpression classRegex{R"(\bclass\s+([a-zA-Z_][a-zA9_]*)\b)"};
    const inline static QRegularExpression classRegex{R"(\bclass\s+([a-zA-Z_][a-zA-Z0-9_]*)\b)"};


    // QTextCharFormat multiLineStringFormat; // disregarding for now
    QTextCharFormat singleLineStringFormat;
    const inline static QRegularExpression singleLineStringRegex{R"(\".*\")"};

    QTextCharFormat commentFormat;
    const inline static QRegularExpression commentRegex{R"(#[^\n]*)"};


    QList<HighlightingRule> highlightingRules;
};

#endif // SYNTAXHIGHLIGHTER_H
