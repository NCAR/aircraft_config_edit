
#ifndef _PROJECT_ITEM_H
#define _PROJECT_ITEM_H

#include "NidasItem.h"
#include "SiteItem.h"
#include <nidas/core/Project.h>
#include <xercesc/dom/DOMNode.hpp>

using namespace nidas::core;


class ProjectItem : public NidasItem
{

public:
    ProjectItem(Project *project, int row, NidasModel *theModel, NidasItem *parent = 0) ;

    ~ProjectItem();

    NidasItem * child(int i);

    bool removeChild(NidasItem *item);

    QString dataField(int column);

    const QVariant & childLabel(int column) const { return NidasItem::_Site_Label; }
    int childColumnCount() const {return 1;}

//protected: commented while Document still uses these

        // get/convert to the underlying model pointers
    Project *getProject() const { return _project; }

    xercesc::DOMNode* getDOMNode() {
        if (domNode)
          return domNode;
        else return domNode=findDOMNode();
        }

protected:
    xercesc::DOMNode *findDOMNode(); 
    QString name();

private:
    Project* _project;
};

#endif