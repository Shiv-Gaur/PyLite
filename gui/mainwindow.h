

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class PythonHighlighter;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onRun();
    void onOpenFile();
    void onClear();

private:
    void setupUI();
    void setupMenuBar();
    void appendOutput(const QString &text, const QColor &color = Qt::white);

    
    static QString captureASTPrint(const char *source);

    QPlainTextEdit *m_editor;           
    QTextEdit      *m_output;           
    QLineEdit      *m_filePath;         
    QLabel         *m_statusLabel;      
    PythonHighlighter *m_highlighter;   
};

#endif 
