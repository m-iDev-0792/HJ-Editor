#include "mainwindow.h"
#include "ui_mainwindow.h"
#include<QDebug>
#include<QFileDialog>
#include <QFile>
#include <QTextStream>
#include "createAST.h"
void resetGlobalVars(){
  globalOutput.clear();
  globalASMOutput.clear();
  symTableStack.clear();
  funcTable.clear();//函数名 -> 函数标签   的映射,函数声明的时候分配一个标签,函数名不准相同,没有函数重载
  curTokPos=0;
  curToken.clear();//don't forget init curToken!!!   =tokens.at(0).tokenValue;
  lineNum=1;//记录行号,方便返回错误位置
  tokenNum=0;
  errorNum=0;
  labelNum=0;
  globalVarNum=0;
  lastLoopEntry.clear();
  lastLoopExit.clear();
  IfNum=0;
  ForNum=0;
  WhileNum=0;
  cmpNum=0;
  curFuncRetNum=0;
  tokens.clear();
}

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow)
{

  ui->setupUi(this);
  setUpHighlighter();
  //init status bar
  ui->outputText->parentWindow=this;
  ui->statusBar->showMessage(tr("Ready"));
  //--------init toolbar------------
  //ui->statusBar->setStyleSheet("QStatusBar{background:rgb(50,50,50);}");
  ui->mainToolBar->setMovable(false);
  ui->mainToolBar->setStyleSheet("QToolButton:hover {background-color:darkgray} QToolBar {background: rgb(82,82,82);border: none;}");
  //--------------------------------

  runIcon.addPixmap(QPixmap(":/image/Run.png"));
  stopIcon.addPixmap(QPixmap(":/image/stop.png"));

  //---------窗口背景颜色-------------
  QPalette windowPalette=this->palette();
  windowPalette.setColor(QPalette::Active,QPalette::Window,QColor(82,82,82));
  windowPalette.setColor(QPalette::Inactive,QPalette::Window,QColor(82,82,82));
  this->setPalette(windowPalette);
  //--------------------------------
  initFileData();
  connect(ui->actionNewFile,SIGNAL(triggered(bool)),this,SLOT(newFile()));
  connect(ui->actionOpen,SIGNAL(triggered(bool)),this,SLOT(openFile()));
  connect(ui->actionSave_File,SIGNAL(triggered(bool)),this,SLOT(saveFile()));
  connect(ui->actionUndo,SIGNAL(triggered(bool)),this,SLOT(undo()));
  connect(ui->actionRedo,SIGNAL(triggered(bool)),this,SLOT(redo()));
  connect(ui->editor,SIGNAL(textChanged()),this,SLOT(changeSaveState()));
  connect(ui->actionRun,SIGNAL(triggered(bool)),this,SLOT(run()));
  connect(ui->actionBuild,SIGNAL(triggered(bool)),this,SLOT(build()));
  connect(ui->actionAbout,SIGNAL(triggered(bool)),this,SLOT(about()));


  //仿照QProcess构造一个虚拟机类，虚拟机类要务必实现这些信号
//  connect(&process,SIGNAL(finished(int)),this,SLOT(runFinished(int)));
//  connect(&process,SIGNAL(readyReadStandardOutput()),this,SLOT(updateOutput()));
//  connect(&process,SIGNAL(readyReadStandardError()),this,SLOT(updateError()));
  fileSaved=false;
  firstLoad=true;
  codeRunner=new HJvm(asmCodes);
  connect(codeRunner,SIGNAL(finished(string)),this,SLOT(displayOutputWhenFinished(string)));
}

MainWindow::~MainWindow()
{
  delete ui;
}
void MainWindow::setUpHighlighter(){
  QFont font;
  font.setFamily("Courier");
  font.setFixedPitch(true);
  //font.setPointSize(20);
  ui->editor->setFont(font);
  ui->editor->setTabStopWidth(fontMetrics().width(QLatin1Char('9'))*4);
  highlighter=new Highlighter(ui->editor->document());
}

void MainWindow::resizeEvent(QResizeEvent *event){
  QMainWindow::resizeEvent(event);
  ui->editor->setGeometry(10,0,width()-20,height()-ui->statusBar->height()-ui->mainToolBar->height()-150-15);
  ui->outputText->setGeometry(10,ui->editor->height()+10,this->width()-20,150);
}
void MainWindow::initFileData(){
  fileName=tr("Untitled.hj");
  filePath=tr("~/Desktop/Untitled.hj");
  fileSaved=true;
  isRunning=false;
}
void MainWindow::undo(){
  ui->editor->undo();
}
void MainWindow::redo(){
  ui->editor->redo();
}
void MainWindow::saveFile(){
  if(!firstLoad){
      autoSave();
      return;
    }
  QString savePath=QFileDialog::getSaveFileName(this,tr("选择保存路径与文件名"),tr("/Users/hezhenbang/Desktop/")+fileName,tr("HJ File(*.hj)"));
  if(!savePath.isEmpty()){
      QFile out(savePath);
      out.open(QIODevice::WriteOnly|QIODevice::Text);
      QTextStream str(&out);
      str<<ui->editor->toPlainText();
      out.close();
      fileSaved=true;
      QRegularExpression re(tr("(?<=\\/)\\w+\\.hj"));
      fileName=re.match(savePath).captured();
      filePath=savePath;
      this->setWindowTitle(tr("HJ IDE - ")+fileName);
      firstLoad=false;
    }
}
void MainWindow::newFile(){
  MainWindow *newWindow=new MainWindow();
  QRect newPos=this->geometry();
  newWindow->setGeometry(newPos.x()+10,newPos.y()+10,newPos.width(),newPos.height());
  newWindow->show();
}
void MainWindow::openFile(){
  if(!fileSaved){
      if(!(firstLoad&&(ui->editor->toPlainText()==QString("")))){
          if(QMessageBox::Save==QMessageBox::question(this,tr("文件未保存"),tr("当前文件没有保存，是否保存？"),QMessageBox::Save,QMessageBox::Cancel))
            saveFile();
        }
    }
  QString openPath=QFileDialog::getOpenFileName(this,tr("选择要打开的文件"),tr("/Users/hezhenbang/Desktop"),tr("HJ File(*.hj)"));
  if(!openPath.isEmpty()){
      QFile in(openPath);
      in.open(QIODevice::ReadOnly|QIODevice::Text);
      QTextStream str(&in);
      ui->editor->setPlainText(str.readAll());
      QRegularExpression re(tr("(?<=\\/)\\w+\\.hj"));
      fileName=re.match(openPath).captured();
      this->setWindowTitle(tr("HJ IDE - ")+fileName);
      filePath=openPath;
      fileSaved=true;
      firstLoad=false;
    }
}
void MainWindow::autoSave(){
  QFile out(filePath);
  out.open(QIODevice::WriteOnly|QIODevice::Text);
  QTextStream str(&out);
  str<<ui->editor->toPlainText();
  out.close();
  fileSaved=true;
  this->setWindowTitle(tr("HJ IDE - ")+fileName);
  firstLoad=false;
}

void MainWindow::run(){
  if(isRunning){
      ui->actionRun->setIcon(stopIcon);
      return;
    }
  if(!fileSaved){
      if(firstLoad){
          if(ui->editor->toPlainText()==QString(""))return;//没有文本直接返回
          //如果不为空，问问要不要保存
          if(QMessageBox::Save==QMessageBox::question(this,tr("文件未保存"),tr("文件保存后才能运行，是否保存？"),QMessageBox::Save,QMessageBox::Cancel))
            saveFile();
          else
            return;//不保存直接返回
      }else{
          autoSave();
      }
    }
  if(fileSaved){
      //先编译
      ui->outputText->clear();
      resetGlobalVars();

      initTokIDtoStr();
      ifstream file(filePath.toStdString());
      yyFlexLexer tokenAnalysiser(&file,&cout);//&cout
      tokenAnalysiser.yylex();//analysis tokens
      tokens.push_back(token(string("$"),0));//add end of file
      //init global variables
      curTokType=tokens.at(0).tokenType;
      curToken=tokens.at(0).tokenValue;
      replaceComment();
      //init end

      auto r=parseMain();
      if(errorNum==0)r->codegen();
      else{
          ui->outputText->appendPlainText(QString::fromStdString(globalOutput));
          return;//出现错误退出
        }
      //编译完毕
    if(errorNum>0){
        ui->outputText->appendPlainText(QString::fromStdString(globalOutput));
        return;//出现错误退出
      }
    isRunning=true;
    ui->statusBar->showMessage(tr("程序运行中..."));
    ui->outputText->clear();
    output.clear();
    error.clear();

    //正式运行程序
    codeRunner->resetCode(asmCodes);
    codeRunner->start();
    ui->outputText->setFocus();
//    ui->actionRun->setIcon(stopIcon);
    ui->actionRun->setIcon(runIcon);
    }
}
void MainWindow::build(){
  if(firstLoad&&ui->editor->toPlainText()==QString("")){
      return;
    }else if(firstLoad&&ui->editor->toPlainText()!=QString("")){
            if(QMessageBox::Save==QMessageBox::question(this,tr("文件未保存"),tr("文件保存后才能编译，是否保存？"),QMessageBox::Save,QMessageBox::Cancel))
            saveFile();
            else{
                return;
            }
    }
  if(isRunning){
      process.terminate();
      ui->actionRun->setIcon(runIcon);
      return;
    }
  if(!fileSaved){
//      if(QMessageBox::Save==QMessageBox::question(this,tr("文件未保存"),tr("文件保存后才能运行，是否保存？"),QMessageBox::Save,QMessageBox::Cancel))
//      saveFile();
      //改成自动保存
      autoSave();
    }
  if(fileSaved){
      ui->outputText->clear();
      resetGlobalVars();

      initTokIDtoStr();
      ifstream file(filePath.toStdString());
//      ofstream outputLex(filePath.toStdString()+string("lex"));
//      ofstream outputASM(filePath.toStdString()+string("asm"));
      yyFlexLexer tokenAnalysiser(&file,&cout);//&cout
      tokenAnalysiser.yylex();//analysis tokens
      tokens.push_back(token(string("$"),0));//add end of file
//      cout<<"词法分析结果:"<<endl;
//      for(auto &t:tokens){
//          cout<<"token字符串:"<<((t.tokenValue==string("\n"))?"回车":t.tokenValue)<<"类型:"<<tokIDtoStr[t.tokenType]<<endl;
//      }
      //init global variables
      curTokType=tokens.at(0).tokenType;
      curToken=tokens.at(0).tokenValue;
      replaceComment();
      //init end

      auto r=parseMain();
      if(errorNum==0)r->codegen();
      ui->outputText->appendPlainText(QString::fromStdString(globalOutput));
      if(errorNum==0)ui->outputText->appendPlainText(QString::fromStdString(globalASMOutput));
    }
}

void MainWindow::runFinished(int code){
  ui->actionRun->setIcon(runIcon);
  isRunning=false;
  qDebug()<<tr("exit code=")<<code;
  ui->statusBar->showMessage(tr("Ready"));
}
void MainWindow::displayOutputWhenFinished(string givenOutput){
  ui->actionRun->setIcon(runIcon);
  isRunning=false;
  ui->statusBar->showMessage(tr("Ready"));
  ui->outputText->clear();
  ui->outputText->setPlainText(QString::fromStdString(givenOutput));
  cout<<"运行完成！"<<endl;
  cout<<givenOutput<<endl;
}

void MainWindow::updateOutput(){
  output=QString::fromLocal8Bit(process.readAllStandardOutput());
  ui->outputText->setPlainText(ui->outputText->toPlainText()+output);
}
void MainWindow::updateError(){
  error=QString::fromLocal8Bit(process.readAllStandardError());
  ui->outputText->setPlainText(ui->outputText->toPlainText()+error);
  process.terminate();
  isRunning=false;
}

void MainWindow::inputData(QString data){
  if(isRunning)process.write(data.toLocal8Bit());
}

void MainWindow::closeEvent(QCloseEvent *event){
  if(firstLoad&&(ui->editor->toPlainText()==QString("")))return;
  if(!fileSaved){
      if(QMessageBox::Save==QMessageBox::question(this,tr("未保存就要退出？"),tr("当前文件没有保存，是否保存？不保存文件改动将会丢失"),QMessageBox::Save,QMessageBox::Cancel))
        saveFile();
      fileSaved=true;
    }
}
void MainWindow::about(){
  QMessageBox::information(this,tr("关于"),tr(" HJ-IDE v1.0 \n 何振邦,Rabbin倾情奉献 \n更多信息访问huajihome.cn"),QMessageBox::Ok);
}
