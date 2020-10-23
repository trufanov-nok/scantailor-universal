#ifndef PROCESSINGINDICATIONPROPAGATOR_H
#define PROCESSINGINDICATIONPROPAGATOR_H

#include <QObject>
#include "PageId.h"

class ProcessingIndicationPropagator: public QObject
{
    Q_OBJECT
public:
    ProcessingIndicationPropagator() {}
    ~ProcessingIndicationPropagator() { emit emitAllProcessingFinished(); }
    void emitPageProcessingStarted(const PageId& page_id);
    void emitPageProcessingStarted(const QSet<PageId>& page_ids);
    void emitPageProcessingFinished(const PageId& page_id);
    void emitPageProcessingFinished(const QSet<PageId>& page_ids);
    void emitAllProcessingFinished();

    static ProcessingIndicationPropagator& instance();
signals:
    void displayProcessingIndicator(const PageId& page_id, bool on);
    void allProcessingFinished();
};

#endif // PROCESSINGINDICATIONPROPAGATOR_H
