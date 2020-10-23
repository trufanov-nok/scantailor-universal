#include "ProcessingIndicationPropagator.h"
#include <QSet>

static ProcessingIndicationPropagator _static_instance;

ProcessingIndicationPropagator&
ProcessingIndicationPropagator::instance()
{
    return _static_instance;
}

void
ProcessingIndicationPropagator::emitPageProcessingStarted(const PageId& page_id)
{
    emit displayProcessingIndicator(page_id, true);
}

void
ProcessingIndicationPropagator::emitPageProcessingStarted(const QSet<PageId>& page_ids)
{
    for (const PageId& p: page_ids) {
        emit displayProcessingIndicator(p, true);
    }
}

void
ProcessingIndicationPropagator::emitPageProcessingFinished(const PageId& page_id)
{
    emit displayProcessingIndicator(page_id, false);
}


void
ProcessingIndicationPropagator::emitPageProcessingFinished(const QSet<PageId>& page_ids)
{
    for (const PageId& p: page_ids) {
        emit displayProcessingIndicator(p, false);
    }
}


void
ProcessingIndicationPropagator::emitAllProcessingFinished()
{
    emit allProcessingFinished();
}
