#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QObject>
#include <QPainter>
#include <QListWidget>
#include <QListWidgetItem>
#include "completelistwidget.h"
#include <algorithm>
QT_BEGIN_NAMESPACE
class QPaintEvent;
class QResizeEvent;
class QSize;
class QWidget;
QT_END_NAMESPACE

class LineNumberArea;

//![codeeditordefinition]

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = 0);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth();
    void setUpCompleteList();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void highlightCurrentLine();
    void updateLineNumberArea(const QRect &, int);
    void showCompleteWidget();
    //void completeWidgetKeyDown();

private:
    QWidget *lineNumberArea;
    QColor lineColor;
    QColor editorColor;
    QStringList completeList;//储存自动填充的关键字
    //QListWidget *completeWidget;
    CompleteListWidget *completeWidget;
    QString getWordOfCursor();
    int completeState;
    int getCompleteWidgetX();
};

//![codeeditordefinition]
//![extraarea]

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeEditor *editor) : QWidget(editor) {
        codeEditor = editor;
    }

    QSize sizeHint() const override {
        return QSize(codeEditor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        codeEditor->lineNumberAreaPaintEvent(event);

    }

private:
    CodeEditor *codeEditor;
};
enum CompleteState{
  Ignore=0,
  Showing=1,
  Hide=2
};

//![extraarea]

#endif
