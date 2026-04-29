

#include "mainwindow.h"
#include "highlighter.h"

#include <QSplitter>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QMenuBar>
#include <QAction>
#include <QApplication>


extern "C" {
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "analyzer.h"
#include "interpreter.h"
extern void ast_print(const ASTNode *node, int indent);
}





#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif
#include <cstdio>
#include <QTemporaryFile>

QString MainWindow::captureASTPrint(const char *source)
{
    
    QString result;
    
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        return "Error: could not create temp file\n";
    }
    tempFile.close(); 

    QString tmpPath = tempFile.fileName();
    FILE *tmp = fopen(tmpPath.toUtf8().constData(), "w+");
    if (!tmp) return "Error: could not open temp file via C API\n";
    
    
    fflush(stdout);
#ifdef _WIN32
    int saved_stdout = _dup(_fileno(stdout));
    _dup2(_fileno(tmp), _fileno(stdout));
#else
    int saved_stdout = dup(fileno(stdout));
    dup2(fileno(tmp), fileno(stdout));
#endif

    
    Lexer lex;
    lexer_init(&lex, source);
    int token_count;
    Token *tokens = lexer_tokenize_all(&lex, &token_count);

    if (tokens) {
        printf("--- Tokens ---\n");
        for (int i = 0; i < token_count && i < 10; i++) {
            printf("[%d] %s: '%.*s'\n", i, token_type_name(tokens[i].type), 
                   tokens[i].length, tokens[i].lexeme);
        }
        
        Parser parser;
        parser_init(&parser, tokens, token_count);
        ASTNode *program = parser_parse(&parser);

        if (parser_had_error(&parser)) {
            printf("Parse error: %s\n", parser_error_message(&parser));
        } else {
            Analyzer analyzer;
            analyzer_init(&analyzer);
            analyzer_analyze(&analyzer, program);

            if (analyzer_had_error(&analyzer)) {
                printf("--- Semantic Errors ---\n");
                for (int i = 0; i < analyzer.error_count; i++) {
                    printf("Line %d: %s\n",
                           analyzer.errors[i].line,
                           analyzer.errors[i].message);
                }
                printf("---\n\n");
            }

            printf("--- Abstract Syntax Tree ---\n");
            ast_print(program, 0);
            printf("----------------------------\n");
        }

        ast_node_free(program);
        free(tokens);
    } else {
        printf("Error: tokenization failed\n");
    }

    
    fflush(stdout);
#ifdef _WIN32
    _dup2(saved_stdout, _fileno(stdout));
    _close(saved_stdout);
#else
    dup2(saved_stdout, fileno(stdout));
    close(saved_stdout);
#endif

    fseek(tmp, 0, SEEK_SET);
    char buf[4096];
    while (fgets(buf, sizeof(buf), tmp)) {
        result += QString::fromUtf8(buf);
    }
    fclose(tmp);
    

    return result;
}





MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_editor(nullptr),
      m_output(nullptr),
      m_filePath(nullptr),
      m_statusLabel(nullptr),
      m_highlighter(nullptr)
{
    setWindowTitle("PyLite IDE v1.0");
    setupUI();
    setupMenuBar();

    
    m_editor->setPlainText(
        "def fizzbuzz(n):\n"
        "    for i in range(1, n):\n"
        "        if i % 15 == 0:\n"
        "            print(\"FizzBuzz\")\n"
        "        elif i % 3 == 0:\n"
        "            print(\"Fizz\")\n"
        "        elif i % 5 == 0:\n"
        "            print(\"Buzz\")\n"
        "        else:\n"
        "            print(i)\n"
        "\n"
        "fizzbuzz(21)\n"
    );
}





void MainWindow::setupUI()
{
    
    QFont mono("Consolas", 11);
    mono.setStyleHint(QFont::Monospace);

    
    m_editor = new QPlainTextEdit(this);
    m_editor->setFont(mono);
    m_editor->setTabStopDistance(QFontMetrics(mono).horizontalAdvance(' ') * 4);
    m_editor->setStyleSheet(
        "QPlainTextEdit {"
        "  background-color: #1e1e2e;"
        "  color: #cdd6f4;"
        "  border: none;"
        "  selection-background-color: #45475a;"
        "}"
    );

    
    m_highlighter = new PythonHighlighter(m_editor->document());

    
    m_output = new QTextEdit(this);
    m_output->setFont(mono);
    m_output->setReadOnly(true);
    m_output->setStyleSheet(
        "QTextEdit {"
        "  background-color: #11111b;"
        "  color: #a6e3a1;"
        "  border: none;"
        "  selection-background-color: #45475a;"
        "}"
    );

    
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_editor);
    splitter->addWidget(m_output);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    setCentralWidget(splitter);

    
    QToolBar *toolbar = addToolBar("Main");
    toolbar->setMovable(false);
    toolbar->setStyleSheet(
        "QToolBar {"
        "  background-color: #313244;"
        "  border: none;"
        "  padding: 4px;"
        "  spacing: 6px;"
        "}"
        "QToolButton {"
        "  background-color: #45475a;"
        "  color: #cdd6f4;"
        "  border: 1px solid #585b70;"
        "  border-radius: 4px;"
        "  padding: 6px 14px;"
        "  font-weight: bold;"
        "}"
        "QToolButton:hover {"
        "  background-color: #585b70;"
        "}"
    );

    QAction *runAction = toolbar->addAction("▶ Run");
    connect(runAction, &QAction::triggered, this, &MainWindow::onRun);

    QAction *openAction = toolbar->addAction("📂 Open");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);

    QAction *clearAction = toolbar->addAction("🗑 Clear");
    connect(clearAction, &QAction::triggered, this, &MainWindow::onClear);

    toolbar->addSeparator();

    
    m_filePath = new QLineEdit(this);
    m_filePath->setPlaceholderText("File path (or type code in editor)...");
    m_filePath->setStyleSheet(
        "QLineEdit {"
        "  background-color: #1e1e2e;"
        "  color: #cdd6f4;"
        "  border: 1px solid #585b70;"
        "  border-radius: 4px;"
        "  padding: 4px 8px;"
        "  min-width: 300px;"
        "}"
    );
    toolbar->addWidget(m_filePath);

    
    m_statusLabel = new QLabel("Ready", this);
    statusBar()->addPermanentWidget(m_statusLabel);
    statusBar()->setStyleSheet(
        "QStatusBar {"
        "  background-color: #181825;"
        "  color: #bac2de;"
        "}"
    );

    
    setStyleSheet(
        "QMainWindow {"
        "  background-color: #1e1e2e;"
        "}"
    );
}

void MainWindow::setupMenuBar()
{
    QMenuBar *menu = menuBar();
    menu->setStyleSheet(
        "QMenuBar {"
        "  background-color: #181825;"
        "  color: #cdd6f4;"
        "}"
        "QMenuBar::item:selected {"
        "  background-color: #45475a;"
        "}"
    );

    QMenu *fileMenu = menu->addMenu("&File");
    fileMenu->addAction("&Open", this, &MainWindow::onOpenFile, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit", qApp, &QApplication::quit, QKeySequence::Quit);

    QMenu *runMenu = menu->addMenu("&Run");
    runMenu->addAction("&Run", this, &MainWindow::onRun, QKeySequence(Qt::CTRL | Qt::Key_R));
}





void MainWindow::onRun()
{
    m_output->clear();
    QString source;

    
    QString path = m_filePath->text().trimmed();
    if (!path.isEmpty()) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            appendOutput("Error: cannot open '" + path + "'\n", QColor("#f38ba8"));
            m_statusLabel->setText("Error");
            return;
        }
        QTextStream in(&f);
        source = in.readAll();
        f.close();
        m_editor->setPlainText(source);
    } else {
        source = m_editor->toPlainText();
    }

    if (source.isEmpty()) {
        appendOutput("No source code to parse.\n", QColor("#fab387"));
        return;
    }

    m_statusLabel->setText("Running...");

    QByteArray bytes = source.toUtf8();
    const char *src = bytes.constData();

    Lexer lex;
    lexer_init(&lex, src);
    int token_count;
    Token *tokens = lexer_tokenize_all(&lex, &token_count);
    if (!tokens) {
        appendOutput("Error: tokenization failed\n", QColor("#f38ba8"));
        m_statusLabel->setText("Error");
        return;
    }

    Parser parser;
    parser_init(&parser, tokens, token_count);
    ASTNode *program = parser_parse(&parser);

    if (parser_had_error(&parser)) {
        appendOutput(QString("SyntaxError: %1\n").arg(parser_error_message(&parser)), QColor("#f38ba8"));
        ast_node_free(program);
        free(tokens);
        m_statusLabel->setText("Syntax Error");
        return;
    }

    /* Capture stdout into a temp file during interpret */
    QTemporaryFile tempFile;
    tempFile.open();
    tempFile.close();
    QString tmpPath = tempFile.fileName();
    FILE *tmp = fopen(tmpPath.toUtf8().constData(), "w+");
    if (!tmp) {
        appendOutput("Error: could not create temp file\n", QColor("#f38ba8"));
        ast_node_free(program); free(tokens);
        return;
    }

    fflush(stdout);
    fflush(stderr);
#ifdef _WIN32
    int saved_stdout = _dup(_fileno(stdout));
    int saved_stderr = _dup(_fileno(stderr));
    _dup2(_fileno(tmp), _fileno(stdout));
    _dup2(_fileno(tmp), _fileno(stderr));
#else
    int saved_stdout = dup(fileno(stdout));
    int saved_stderr = dup(fileno(stderr));
    dup2(fileno(tmp), fileno(stdout));
    dup2(fileno(tmp), fileno(stderr));
#endif

    interpret(program);

    fflush(stdout);
    fflush(stderr);
#ifdef _WIN32
    _dup2(saved_stdout, _fileno(stdout));
    _dup2(saved_stderr, _fileno(stderr));
    _close(saved_stdout);
    _close(saved_stderr);
#else
    dup2(saved_stdout, fileno(stdout));
    dup2(saved_stderr, fileno(stderr));
    close(saved_stdout);
    close(saved_stderr);
#endif

    fseek(tmp, 0, SEEK_SET);
    char buf[4096];
    QString result;
    while (fgets(buf, sizeof(buf), tmp)) {
        result += QString::fromUtf8(buf);
    }
    fclose(tmp);

    if (!result.isEmpty()) {
        appendOutput(result, QColor("#a6e3a1"));
    } else {
        appendOutput("(no output)\n", QColor("#6c7086"));
    }

    ast_node_free(program);
    free(tokens);
    m_statusLabel->setText("Done");
}

void MainWindow::onOpenFile()
{
    QString path = QFileDialog::getOpenFileName(
        this, "Open Python File", QString(), "Python Files (*.py);;All Files (*)");
    if (path.isEmpty()) return;

    m_filePath->setText(path);

    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        m_editor->setPlainText(in.readAll());
        f.close();
        m_statusLabel->setText("Loaded: " + path);
    }
}

void MainWindow::onClear()
{
    m_output->clear();
    m_statusLabel->setText("Ready");
}

void MainWindow::appendOutput(const QString &text, const QColor &color)
{
    m_output->setTextColor(color);
    m_output->append(text);
}
