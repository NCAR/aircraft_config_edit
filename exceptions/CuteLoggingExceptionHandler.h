#ifndef _CuteLoggingExceptionHandler_h
#define _CuteLoggingExceptionHandler_h


#include "UserFriendlyExceptionHandler.h"
#include <QTextEdit>
#include <QDialog>
#include <string>


class CuteLoggingExceptionHandler : public UserFriendlyExceptionHandler,
    public QDialog
{

public:

CuteLoggingExceptionHandler(QWidget * parent = 0);
~CuteLoggingExceptionHandler() {}

virtual void display(std::string& where, std::string& what);


virtual void log(std::string& where, std::string& what) {
    textwin->append(QString::fromStdString(where+": "+what));
    }

virtual void show();
virtual void hide();
virtual void setVisible(bool checked=true);


protected:

  // Qt4.3+ we can  use QPlainTextEdit which "is optimized for use as a log display"
  // http://www.nabble.com/Log-viewer-td21499499.html
  QTextEdit * textwin;

};

#endif