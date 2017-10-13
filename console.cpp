#include "console.h"

Console::Console(QWidget* parent):QPlainTextEdit(parent)
{
  connect(this,SIGNAL(cursorPositionChanged()),this,SLOT(resetCursorPosition()));
  QColor editorColor;
  editorColor.setRgb(34,39,49);

  //set background color
  QPalette p = this->palette();
  p.setColor(QPalette::Active, QPalette::Base, editorColor);
  p.setColor(QPalette::Inactive, QPalette::Base, editorColor);
  p.setColor(QPalette::Text,Qt::white);
  this->setPalette(p);
}
void Console::keyPressEvent(QKeyEvent *event){
  if(event->key()==Qt::Key_Backspace&&this->textCursor().atBlockStart())return;
  if(event->key()==Qt::Key_Return){
      QString data=(this->textCursor()).block().text()+tr("\n");
      qDebug()<<"sending data:  "<<data;
      parentWindow->inputData(data);
    }
  QPlainTextEdit::keyPressEvent(event);
}
void Console::resetCursorPosition(){
  QTextCursor cursor=this->textCursor();
  cursor.movePosition(QTextCursor::End,QTextCursor::MoveAnchor);
  this->setTextCursor(cursor);
}
