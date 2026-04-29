

#include "highlighter.h"

PythonHighlighter::PythonHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    
    m_keywordFormat.setForeground(QColor("#cba6f7"));
    m_keywordFormat.setFontWeight(QFont::Bold);
    const QString keywords[] = {
        "\\bif\\b", "\\belif\\b", "\\belse\\b",
        "\\bwhile\\b", "\\bfor\\b", "\\bin\\b",
        "\\bdef\\b", "\\breturn\\b",
        "\\band\\b", "\\bor\\b", "\\bnot\\b",
        "\\bbreak\\b", "\\bcontinue\\b", "\\bpass\\b",
        "\\bclass\\b", "\\bimport\\b", "\\bfrom\\b",
    };
    for (const auto &kw : keywords) {
        rule.pattern = QRegularExpression(kw);
        rule.format = m_keywordFormat;
        m_rules.append(rule);
    }

    
    m_boolNoneFormat.setForeground(QColor("#fab387"));
    m_boolNoneFormat.setFontWeight(QFont::Bold);
    const QString boolNone[] = {
        "\\bTrue\\b", "\\bFalse\\b", "\\bNone\\b"
    };
    for (const auto &bn : boolNone) {
        rule.pattern = QRegularExpression(bn);
        rule.format = m_boolNoneFormat;
        m_rules.append(rule);
    }

    
    m_builtinFormat.setForeground(QColor("#89b4fa"));
    const QString builtins[] = {
        "\\bprint\\b", "\\blen\\b", "\\brange\\b",
        "\\binput\\b", "\\btype\\b", "\\bappend\\b", "\\bpop\\b"
    };
    for (const auto &bi : builtins) {
        rule.pattern = QRegularExpression(bi);
        rule.format = m_builtinFormat;
        m_rules.append(rule);
    }

    
    m_defClassFormat.setForeground(QColor("#a6e3a1"));
    m_defClassFormat.setFontWeight(QFont::Bold);
    rule.pattern = QRegularExpression("\\bdef\\s+(\\w+)");
    rule.format = m_defClassFormat;
    m_rules.append(rule);

    
    m_numberFormat.setForeground(QColor("#fab387"));
    rule.pattern = QRegularExpression("\\b[0-9]+\\b");
    rule.format = m_numberFormat;
    m_rules.append(rule);

    
    m_stringFormat.setForeground(QColor("#a6e3a1"));
    rule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    rule.format = m_stringFormat;
    m_rules.append(rule);
    rule.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    rule.format = m_stringFormat;
    m_rules.append(rule);

    
    m_commentFormat.setForeground(QColor("#6c7086"));
    m_commentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = m_commentFormat;
    m_rules.append(rule);
}

void PythonHighlighter::highlightBlock(const QString &text)
{
    for (const auto &rule : m_rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
