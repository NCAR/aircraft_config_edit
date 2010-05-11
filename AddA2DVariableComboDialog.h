#ifndef _config_AddA2DVariableComboDialog_h
#define _config_AddA2DVariableComboDialog_h

#include "ui_AddA2DVariableComboDialog.h"
#include <iostream>
#include <QMessageBox>
#include "Document.h"

namespace config
{

class AddA2DVariableComboDialog : public QDialog, public Ui_AddA2DVariableComboDialog
{
    Q_OBJECT

public slots:
    void accept() ;

    void reject() {
        VariableText->clear();
        LongNameText->clear();
        UnitsText->clear();
        Calib1Text->clear();
        Calib2Text->clear();
        Calib3Text->clear();
        Calib4Text->clear();
        Calib5Text->clear();
        Calib6Text->clear();
        this->hide();
        }

    void show();
    //bool setUpDialog();

public:

    AddA2DVariableComboDialog(QWidget * parent = 0);

    ~AddA2DVariableComboDialog() {}

    void setDocument(Document * document) {_document = document;}

protected:

    QMessageBox * _errorMessage;
    Document * _document;
};

}

#endif
