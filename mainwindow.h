#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "highlighter.h"
#include <QRegularExpression>
#include <QMessageBox>
#include <QProcess>
#include <QDebug>
namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();
private:
  QIcon runIcon;
  QIcon stopIcon;
  Ui::MainWindow *ui;
  Highlighter *highlighter;
  QProcess process;
  void setUpHighlighter();
  //---------记录文件信息----------
  QString fileName;
  QString filePath;
  bool fileSaved;
  bool isRunning;
  //bool fileEdited;
  void initFileData();
  bool firstLoad;
  //-----------------------------


  //---------code running data---
  QString output;
  QString error;
  //-----------------------------
public slots:
  void changeSaveState(){
    //qDebug()<<"changed";
    if(firstLoad&&fileSaved){
        this->setWindowTitle(tr("HJ Editor - ")+fileName);
        firstLoad=false;
        return;
      }
    fileSaved=false;
    this->setWindowTitle(tr("HJ Editor - ")+fileName+tr("*"));
  }

  //---------工具栏响应函数---------
  void newFile();
  void saveFile();
  void openFile();
  void undo();
  void redo();
  void run();
  //------------------------------
  void runFinished(int code);
  void updateOutput();
  void updateError();
  void about();
public:
  void inputData(QString data);
protected:
  void resizeEvent(QResizeEvent* event)override;
  void closeEvent(QCloseEvent* event)override;
};

#endif // MAINWINDOW_H
