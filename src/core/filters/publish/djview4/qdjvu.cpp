//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006-  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License, either version 2 of the license,
//C- or (at your option) any later version. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

#if AUTOCONF
# include "config.h"
#endif

#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjvu.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVector>
#if QT_VERSION >= 0x50000
# include <QUrlQuery>
#endif
#if DDJVUAPI_VERSION < 17
# error "DDJVUAPI_VERSION>=17 is required !"
#endif


// ----------------------------------------
// QDJVUCONTEXT


/*! \class QDjVuContext
    \brief Represents a \a ddjvu_context_t object.
    
    This QObject subclass holds a \a ddjvu_context_t object and 
    transparently hooks the DDJVUAPI message queue into 
    the Qt messaging system. DDJVUAPI messages are then
    forwarded to the virtual function \a QDjVuContext::handle
    which transforms then into signals emitted by 
    the proper object from the message loop thread. 
*/

/*! Construct a \a QDJVuContext object.
    Argument \a programname is the name of the program.
    This name is used to report error messages and locate
    localized messages. Argument \a parent defines its
    parent in the \a QObject hierarchy. */

QDjVuContext::QDjVuContext(const char *programname, QObject *parent)
  : QObject(parent), context(0), flag(false)
{
  context = ddjvu_context_create(programname);
  ddjvu_message_set_callback(context, callback, (void*)this);
  ddjvu_cache_set_size(context, 30*1024*1024);
}

QDjVuContext::~QDjVuContext()
{
  ddjvu_context_release(context);
  context = 0;
}

/*! \property QDjVuContext::cacheSize
    \brief The size of the decoded page cache in bytes. 
    The default cache size is 30 megabytes. */

long 
QDjVuContext::cacheSize() const
{
  return ddjvu_cache_get_size(context);
}

void 
QDjVuContext::setCacheSize(long size)
{
  ddjvu_cache_set_size(context, size);
}

void 
QDjVuContext::callback(ddjvu_context_t *, void *closure)
{
  QDjVuContext *qcontext = (QDjVuContext*)closure;
  if (! qcontext->flag)
    {
      qcontext->flag = true;
      QEvent *qevent = new QEvent(QEvent::User);
      QCoreApplication::postEvent(qcontext, qevent);
    }
}


bool
QDjVuContext::event(QEvent *event)
{
  if (event->type() == QEvent::User)
    {
      flag = false;
      ddjvu_message_t *message;
      while ((message = ddjvu_message_peek(context)))
        {
          handle(message);
          ddjvu_message_pop(context);
        }
      return true;
    }
  return QObject::event(event);
}


/*! Processes DDJVUAPI messages.
    The Qt message loop automatically passes all DDJVUAPI messages 
    to this virtual function . This function then forwards them
    to the \a handle function of the suitable \a QDjVuDocument,
    \a QDjVuPage or \a QDjVuJob object. Most messages eventually
    result into the emission of a suitable signal. */

bool
QDjVuContext::handle(ddjvu_message_t *msg)
{
  if (msg->m_any.page)
    {
      QObject *p = (QObject*)ddjvu_page_get_user_data(msg->m_any.page);
      QDjVuPage *q = (p) ? qobject_cast<QDjVuPage*>(p) : 0;
      if (q && q->handle(msg))
        return true;
    }
  if (msg->m_any.job)
    {
      ddjvu_job_t *djob = 0;
      ddjvu_job_t *pjob = 0;
      if (msg->m_any.document)
        djob = ddjvu_document_job(msg->m_any.document);
      if (msg->m_any.page)
        pjob = ddjvu_page_job(msg->m_any.page);
      if (msg->m_any.job != djob && msg->m_any.job != pjob)
        {
          QObject *p = (QObject*)ddjvu_job_get_user_data(msg->m_any.job);
          QDjVuJob *q = (p) ? qobject_cast<QDjVuJob*>(p) : 0;
          if (q && q->handle(msg))
            return true;
        }
    }
  if (msg->m_any.document)
    {
      QObject *p = (QObject*)ddjvu_document_get_user_data(msg->m_any.document);
      QDjVuDocument *q = (p) ? qobject_cast<QDjVuDocument*>(p) : 0;
      if (q && q->handle(msg))
        return true;
    }
  switch(msg->m_any.tag)
    {
    case DDJVU_ERROR:
      emit error(QString::fromLocal8Bit(msg->m_error.message),
                 QString::fromLocal8Bit(msg->m_error.filename), 
                 msg->m_error.lineno);
      return true;
    case DDJVU_INFO:
      emit info(QString::fromLocal8Bit(msg->m_info.message));
      return true;
    default:
      break;
    }
  return false;
}

/*! \fn QDjVuContext::isValid()
    Indicate if this object is associated with a 
    \a ddjvu_context_t object. Use this function
    to determine if the construction was successful. */

/*! \fn QDjVuContext::operator ddjvu_context_t*()
    Return a pointer to the corresponding 
    \a ddjvu_context_t object. */

/*! \fn QDjVuContext::error(QString msg, QString filename, int lineno)
  
    This signal is emitted when processing a DDJVUAPI error message
    that is not attached to any document, page, or job.
    This only hapens when a DDJVUAPI function is used incorrectly
    or when calling \a QDjVuDocument::setFileName or 
    \a QDjVuDocument::setUrl. 
*/

/*! \fn QDjVuContext::info(QString msg)

    This signal is emitted when processing a DDJVUAPI info message
    that is not attached to any document, page, or job.
    This is quite rare and not very useful.
*/


// ----------------------------------------
// QDJVUDOCUMENTPRIVATE


class QDjVuDocumentPrivate : public QObject
{
  Q_OBJECT
public:
  QMutex mutex;
  bool autoDelete;
  int refCount;
  QSet<QObject*> running;
  bool docReady;
  QDjVuDocument *docPointer;
  minivar_t documentOutline;
  minivar_t documentAnnotations;
  QVector<minivar_t> pageAnnotations;
  QVector<minivar_t> pageText;
  
  QDjVuDocumentPrivate();
  void add(QObject *p);
  void add(QDjVuPage *p);
public slots:
  void docinfo();
protected slots:
  void remove(QObject *p);
  void pageinfo();
  void emitidle();
signals:
  void idle();
};

QDjVuDocumentPrivate::QDjVuDocumentPrivate()
  : mutex(QMutex::Recursive),
    autoDelete(false), 
    refCount(0),
    docReady(false), 
    docPointer(0)
{
}

void
QDjVuDocumentPrivate::add(QObject *p)
{
  connect(p, SIGNAL(destroyed(QObject*)), this, SLOT(remove(QObject*)));
  mutex.lock();
  running.insert(p);
  mutex.unlock();
}

void
QDjVuDocumentPrivate::emitidle()
{
  emit idle();
}

void
QDjVuDocumentPrivate::remove(QObject *p)
{
  mutex.lock();
  running.remove(p);
  int size = running.size();
  disconnect(p, 0, this, 0);
  mutex.unlock();
  if (! size)
    QTimer::singleShot(0, this, SLOT(emitidle()));
}

void
QDjVuDocumentPrivate::add(QDjVuPage *p)
{
  if (ddjvu_page_decoding_done(*p)) return;
  connect(p, SIGNAL(pageinfo()), this, SLOT(pageinfo()));
  add((QObject*)(p));
}

void
QDjVuDocumentPrivate::pageinfo()
{
  QDjVuPage *p = qobject_cast<QDjVuPage*>(sender());
  if (p && ddjvu_page_decoding_done(*p))
    remove(p);
}


void
QDjVuDocumentPrivate::docinfo()
{
  if (!docReady && ddjvu_document_decoding_done(*docPointer))
    {
      QMutexLocker locker(&mutex);
      int pageNum = ddjvu_document_get_pagenum(*docPointer);
      documentOutline = miniexp_dummy;
      documentAnnotations = miniexp_dummy;
      pageAnnotations.resize(pageNum);
      pageText.resize(pageNum);
      for (int i=0; i<pageNum; i++)
        pageAnnotations[i] = pageText[i] = miniexp_dummy;
      docReady = true;
    }
}





// ----------------------------------------
// QDJVUDOCUMENT


/*! \class QDjVuDocument
    \brief Represents a \a ddjvu_document_t object. */

QDjVuDocument::~QDjVuDocument()
{
  if (document)
    {
      ddjvu_document_set_user_data(document, 0);
      ddjvu_document_release(document);
      document = 0;
    }
  if (priv)
    {
      delete priv;
      priv = 0;
    }
}

/*! Construct an empty \a QDjVuDocument object.
  Argument \a parent indicates its parent in the \a QObject hierarchy. 
  When argument \a autoDelete is set to true, the object also 
  maintains a reference count that can be modified using functions
  \a ref() and \a deref(). The object is deleted when function \a deref()
  decrements the reference count to zero. */

QDjVuDocument::QDjVuDocument(bool autoDelete, QObject *parent)
  : QObject(parent), document(0), priv(new QDjVuDocumentPrivate)
{
  priv->autoDelete = autoDelete;
  priv->docPointer = this;
  connect(priv, SIGNAL(idle()), this, SIGNAL(idle()));
  connect(this, SIGNAL(docinfo()), priv, SLOT(docinfo()));
}

/*! \overload */

QDjVuDocument::QDjVuDocument(QObject *parent)
  : QObject(parent), document(0), priv(new QDjVuDocumentPrivate)
{
  priv->autoDelete = false;
  priv->docPointer = this;
  connect(priv, SIGNAL(idle()), this, SIGNAL(idle()));
  connect(this, SIGNAL(docinfo()), priv, SLOT(docinfo()));
}

/*! Increments the reference count. */

void 
QDjVuDocument::ref()
{
  priv->mutex.lock();
  priv->refCount++;
  priv->mutex.unlock();
}

/*! Decrements the reference count. 
  If the object was created with flag \a autoDelete,
  function \a deleteLater() is called when the 
  reference count reaches 0. */

void 
QDjVuDocument::deref()
{
  priv->mutex.lock();
  bool finished = !(--priv->refCount) && priv->autoDelete;
  priv->mutex.unlock();
  if (finished)
    delete this;
}


/*! Associates the \a QDjVuDocument object with the 
    \a QDjVuContext object \ctx in order to decode
    the DjVu file \a f. */

bool 
QDjVuDocument::setFileName(QDjVuContext *ctx, QString f, bool cache)
{
  QMutexLocker locker(&priv->mutex);  
  if (isValid())
    {
      ddjvu_document_set_user_data(document, 0);
      ddjvu_document_release(document);
      priv->running.clear();
      document = 0;
    }
  QFileInfo info(f);
  if (! info.isReadable())
    {
      qWarning("QDjVuDocument::setFileName: cannot read file");
      return false;
    }
#if DDJVUAPI_VERSION >= 19
  QByteArray b = f.toUtf8();
  document = ddjvu_document_create_by_filename_utf8(*ctx, b, cache);
#else
  QByteArray b = QFile::encodeName(f);
  document = ddjvu_document_create_by_filename(*ctx, b, cache);
#endif
  if (! document)
    {
      qWarning("QDjVuDocument::setFileName: cannot create decoder");    
      return false;
    }
  ddjvu_document_set_user_data(document, (void*)this);
  priv->docinfo();
  return true;
}

/*! Associates the \a QDjVuDocument object with
    with the \a QDjVuContext object \ctx in order
    to decode the DjVu data located at URL \a url.
    This is only useful inside a subclass of this class
    because you must redefine virtual function \a newstream
    in order to provide access to the data. */

bool
QDjVuDocument::setUrl(QDjVuContext *ctx, QUrl url, bool cache)
{
  QMutexLocker locker(&priv->mutex);  
  if (isValid())
    {
      ddjvu_document_set_user_data(document, 0);
      ddjvu_document_release(document);
      priv->running.clear();
      document = 0;
    }
  QByteArray b = url.toEncoded();
  if (! b.size())
    {
      qWarning("QDjVuDocument::setUrl: invalid url");
      return false;
    }
  
  document = ddjvu_document_create(*ctx, b, cache);
  if (! document)
    {
      qWarning("QDjVuDocument::setUrl: cannot create");
      return false;
    }
  ddjvu_document_set_user_data(document, (void*)this);
  priv->docinfo();
  return true;
}


static bool
string_is_on(QString val)
{
  QString v = val.toLower();
  return v == "yes" || v == "on" || v == "true" || v == "1";
}

static bool
string_is_off(QString val)
{
  QString v = val.toLower();
  return v == "no" || v == "off" || v == "false" || v == "0";
}


/*! Overloaded version of \a setUrl.
    Cache setup is determined heuristically from the url
    and the url arguments. */

bool
QDjVuDocument::setUrl(QDjVuContext *ctx, QUrl url)
{
  bool cache = true;
  if (url.path().section('/', -1).indexOf('.') < 0)
    cache = false;
  bool djvuopts = false;
  QPair<QString,QString> pair;
#if QT_VERSION >= 0x50000
  QList<QPair<QString,QString> > qitems = QUrlQuery(url).queryItems();
#else
  QList<QPair<QString,QString> > qitems = url.queryItems();
#endif
  foreach(pair, qitems)
    {
      if (pair.first.toLower() == "djvuopts")
        djvuopts = true;
      else if (!djvuopts || pair.first.toLower() != "cache")
        continue;
      else if (string_is_on(pair.second))
        cache = true;
      else if (string_is_off(pair.second))
        cache = false;
    }
  return setUrl(ctx, url, cache);
}


/*! This virtual function is called when receiving
    a DDJVUAPI \a m_newstream message. This happens
    when the decoder has been setup with functon \a setUrl.
    You must then override this virtual function in order
    to setup the data transfer.  Data is passed to the decoder
    using the \a streamWrite and \a streamClose member functions
    with the specified \a streamid. See also the DDJVUAPI 
    documentation for the \a m_newstream message. */

void
QDjVuDocument::newstream(int, QString, QUrl)
{
  qWarning("QDjVuDocument::newstream called but not implemented");
}

/*! Write data into the decoder stream \a streamid. */

void 
QDjVuDocument::streamWrite(int streamid, 
                           const char *data, unsigned long len )
{
  QMutexLocker locker(&priv->mutex);  
  if (! isValid())
    qWarning("QDjVuDocument::streamWrite: invalid document");
  else
    ddjvu_stream_write(document, streamid, data, len);
}

/*! Close the decoder stream \a streamid.
    Setting argument \a stop to \a true indicates
    that the stream was closed because the data
    transfer was interrupted by the user. */

void 
QDjVuDocument::streamClose(int streamid, bool stop)
{
  QMutexLocker locker(&priv->mutex);  
  if (! isValid())
    qWarning("QDjVuDocument::streamClose: invalid document");
  else
    ddjvu_stream_close(document, streamid, stop);
}

/*! Processes DDJVUAPI messages for this document. 
    The default implementation emits signals for
    the \a m_error, \a m_info, \a m_docinfo, \a m_pageinfo
    and \a m_thumbnail messsages. It also calls
    the virtual function \a newstream when processing
    a \m newstream message. The return value is a boolean indicating
    if the message has been processed or rejected. */

bool
QDjVuDocument::handle(ddjvu_message_t *msg)
{
  switch(msg->m_any.tag)
    {
    case DDJVU_DOCINFO:
      ddjvu_document_check_pagedata(document, 0);
      emit docinfo();
      return true;
    case DDJVU_PAGEINFO:
      emit pageinfo();
      return true;
    case DDJVU_THUMBNAIL:
      emit thumbnail(msg->m_thumbnail.pagenum);
      return true;
    case DDJVU_NEWSTREAM:
      {
        QUrl url;
        QString name;
        if (msg->m_newstream.url)
          url = QUrl::fromEncoded(msg->m_newstream.url);
        if (msg->m_newstream.name)
          name = QString::fromLatin1(msg->m_newstream.name);
        newstream(msg->m_newstream.streamid, name, url);
      }
      return true;
    case DDJVU_ERROR:
      emit error(QString::fromLocal8Bit(msg->m_error.message),
                 QString::fromLocal8Bit(msg->m_error.filename), 
                 msg->m_error.lineno);
      return true;
    case DDJVU_INFO:
      emit info(QString::fromLocal8Bit(msg->m_info.message));
      return true;
    default:
      break;
    }
  return false;
}

/*! Return number of decoding threads running for this document. */

int 
QDjVuDocument::runningProcesses(void)
{
  return priv->running.size();
}


/*! Obtains the cached document annotations.
  This function returns \a miniexp_dummy if this 
  information is not yet available.
  Check again some time after 
  receiving signal \a docinfo(). */

miniexp_t 
QDjVuDocument::getDocumentAnnotations()
{
  QMutexLocker locker(&priv->mutex);  
  if (! priv->docReady)
    return miniexp_dummy;
  if (priv->documentAnnotations != miniexp_dummy)
    return priv->documentAnnotations;
#if DDJVUAPI_VERSION <= 17
  priv->documentAnnotations = miniexp_nil;
#else
  priv->documentAnnotations = ddjvu_document_get_anno(document,1);
  ddjvu_miniexp_release(document, priv->documentAnnotations);
#endif
  return priv->documentAnnotations;
}

/*! Obtains the cached document outline.
  This function returns \a miniexp_dummy if this 
  information is not yet available.
  Check again some time after 
  receiving signal \a docinfo(). */

miniexp_t 
QDjVuDocument::getDocumentOutline()
{
  QMutexLocker locker(&priv->mutex);  
  if (! priv->docReady)
    return miniexp_dummy;
  if (priv->documentOutline != miniexp_dummy)
    return priv->documentOutline;
  priv->documentOutline = ddjvu_document_get_outline(document);
  ddjvu_miniexp_release(document, priv->documentOutline);
  return priv->documentOutline;
}

/*! Obtains the cached annotations for page \a pageno.
  If this information is not yet available, 
  this function returns \a miniexp_dummy 
  and, if \a start is true, starts loading the page data.
  Check again after receiving signal \a pageinfo(). */

miniexp_t 
QDjVuDocument::getPageAnnotations(int pageno, bool start)
{
  QMutexLocker locker(&priv->mutex);  
  if (! priv->docReady)
    return miniexp_dummy;
  if (pageno<0 || pageno>=priv->pageAnnotations.size())
    return miniexp_dummy;
  minivar_t expr = priv->pageAnnotations[pageno];
  if (expr != miniexp_dummy)
    return expr;
  if (! (start || ddjvu_document_check_pagedata(*this, pageno)))
    return expr;
  expr = ddjvu_document_get_pageanno(document, pageno);
  ddjvu_miniexp_release(document, expr);
  if (expr != miniexp_dummy)
    priv->pageAnnotations[pageno] = expr;
  return expr;
}

/*! Obtains the cached hidden text for page \a pageno.
  If this information is not yet available, 
  this function returns \a miniexp_dummy 
  and, if \a start is true, starts loading the page data.
  Check again after receiving signal \a pageinfo(). */

miniexp_t 
QDjVuDocument::getPageText(int pageno, bool start)
{
  QMutexLocker locker(&priv->mutex);  
  if (! priv->docReady)
    return miniexp_dummy;
  if (pageno<0 || pageno>=priv->pageText.size())
    return miniexp_dummy;
  minivar_t expr = priv->pageText[pageno];
  if (expr != miniexp_dummy)
    return expr;
  if (! (start || ddjvu_document_check_pagedata(*this, pageno)))
    return expr;
  expr = ddjvu_document_get_pagetext(document, pageno, 0);
  ddjvu_miniexp_release(document, expr);
  if (expr != miniexp_dummy)
    priv->pageText[pageno] = expr;
  return expr;
}



/*! \fn QDjVuDocument::isValid()
    Indicate if this object is associated with a valid
    \a ddjvu_document_t object. Use this function to
    determine whether \a setFileName or \a setUrl
    was successful. */

/*! \fn QDjVuDocument::operator ddjvu_document_t*()
    Return a pointer to the corresponding 
    \a ddjvu_document_t object. */

/*! \fn QDjVuDocument::error(QString msg, QString filename, int lineno)
    This signal is emitted when processing a DDJVUAPI \a m_error 
    message that is related to this document. */

/*! \fn QDjVuDocument::info(QString msg)
    This signal is emitted when processing a DDJVUAPI \a m_info 
    message that is related to this document. */

/*! \fn QDjVuDocument::docinfo()
    This signal is emitted when the document initialization 
    is complete. Use \a ddjvu_document_decoding_status to
    determine whether the operation was successful. */

/*! \fn QDjVuDocument::pageinfo()
    This signal is emitted when the data for a new page
    has been received in full. This is useful when used
    in relation with the DDJVUAPI function
    \a ddjvu_document_get_pageinfo. */

/*! \fn QDjVuDocument::thumbnail(int pagenum)
    This signal is emitted when the thumbnail for page 
    \a pagenum is ready. This is useful when used
    in relation with the DDJVUAPI functions
    \a ddjvu_thumnail_status and \a ddjvu_thumbnail_render. */

/*! \fn QDjVuDocument::idle()
    This signal is emitted when all decoding threads
    for this document are terminated. */



// ----------------------------------------
// QDJVUPAGE

/*! \class QDjVuPage
    \brief Represent a \a ddjvu_page_t object. */

/*! Construct a \a QDjVuPage object for page \a pageno
    of document \a doc. Argument \a parent indicates its 
    parent in the \a QObject hierarchy. */

QDjVuPage::QDjVuPage(QDjVuDocument *doc, int pageno, QObject *parent)
  : QObject(parent), page(0), pageno(pageno)
{
  page = ddjvu_page_create_by_pageno(*doc, pageno);
  if (! page)
    {
      qWarning("QDjVuPage: invalid page number");
      return;
    }
  ddjvu_page_set_user_data(page, (void*)this);
  doc->priv->add(this);
}

QDjVuPage::~QDjVuPage()
{
  pageno = -1;
  if (page)
    {
      ddjvu_page_set_user_data(page, 0);
      ddjvu_page_release(page);
      page = 0;
    }
}

/*! Processes DDJVUAPI messages for this page. 
    The default implementation emits signals for
    the \a m_error, \a m_info, \a m_pageinfo, \a m_chunk
    and \a m_relayout and \a m_redisplay messsages. 
    The return value is a boolean indicating
    if the message has been processed or rejected. */

bool
QDjVuPage::handle(ddjvu_message_t *msg)
{

  switch(msg->m_any.tag)
    {
    case DDJVU_PAGEINFO:
      emit pageinfo();
      return true;
    case DDJVU_CHUNK:
      emit chunk(QString::fromLatin1(msg->m_chunk.chunkid));
      return true;
    case DDJVU_RELAYOUT:
      emit relayout();
      return true;
    case DDJVU_REDISPLAY:
      emit redisplay();
      return true;
    case DDJVU_ERROR:
      emit error(QString::fromLocal8Bit(msg->m_error.message),
                 QString::fromLocal8Bit(msg->m_error.filename), 
                 msg->m_error.lineno);
      return true;
    case DDJVU_INFO:
      emit info(QString::fromLocal8Bit(msg->m_info.message));
      return true;
    default:
      break;
    }
  return false;
}

/*! Returns the page number associated with this page. */

int 
QDjVuPage::pageNo()
{
  return pageno;
}

/*! \fn QDjVuPage::isValid()
    Indicate if this object is associated with a valid
    \a ddjvu_page_t object. Use this function to
    determine whether the construction was successful. */

/*! \fn QDjVuPage::operator ddjvu_page_t * ()
    Return a pointer to the corresponding 
    \a ddjvu_page_t object. */

/*! \fn QDjVuPage::error(QString msg, QString filename, int lineno)
    This signal is emitted when processing a DDJVUAPI \a m_error 
    message that is related to this page. */

/*! \fn QDjVuPage::info(QString msg)
    This signal is emitted when processing a DDJVUAPI \a m_info 
    message that is related to this page. */

/*! \fn QDjVuPage::pageinfo()
    This signal is emitted when processing a DDJVUAPI \a m_pageinfo
    message that is related to this page. This happens twice 
    during the page decoding process. The first time when the 
    basic page information is present, and the second time 
    when the decoding process terminates. 
    Use \a ddjvu_page_decoding_status function
    to determine what is going on. */

/*! \fn QDjVuPage::relayout()
    This signal is emitted when processing a DDJVUAPI \a m_relayout 
    message that is related to this page. */

/*! \fn QDjVuPage::redisplay()
    This signal is emitted when processing a DDJVUAPI \a m_redisplay 
    message that is related to this page. */

/*! \fn QDjVuPage::chunk(QString chunkid)
    This signal is emitted when processing a DDJVUAPI \a m_chunk 
    message that is related to this page. */



// ----------------------------------------
// QDJVUJOB

/*! \class QDjVuJob
    \brief Represents a \a ddjvu_job_t object. */

/*! Construct a \a QDjVuJob object.
    Argument \a job is the \a ddjvu_job_t object.
    Argument \a parent defines its parent in the \a QObject hierarchy. */

QDjVuJob::QDjVuJob(ddjvu_job_t *job, QObject *parent)
  : QObject(parent), job(job)
{
  if (! job)
    qWarning("QDjVuJob: invalid job");
  else
    ddjvu_job_set_user_data(job, (void*)this);
}

QDjVuJob::~QDjVuJob()
{
  ddjvu_job_set_user_data(job, 0);
  ddjvu_job_release(job);
  job = 0;
}

/*! Processes DDJVUAPI messages for this job. 
    The default implementation emits signals for
    the \a m_error, \a m_info, and \a m_progress
    messages.  The return value is a boolean indicating
    if the message has been processed or rejected. */

bool
QDjVuJob::handle(ddjvu_message_t *msg)
{
  switch(msg->m_any.tag)
    {
    case DDJVU_ERROR:
      emit error(QString::fromLocal8Bit(msg->m_error.message),
                 QString::fromLocal8Bit(msg->m_error.filename), 
                 msg->m_error.lineno);
      return true;
    case DDJVU_INFO:
      emit info(QString::fromLocal8Bit(msg->m_info.message));
      return true;
    case DDJVU_PROGRESS:
      emit progress(msg->m_progress.percent);
      return true;
    default:
      break;
    }
  return false;
}

/*! \fn QDjVuJob::isValid()
    Indicate if this object is associated with a 
    \a ddjvu_job_t object. */

/*! \fn QDjVuJob::operator ddjvu_job_t * ()
    Return a pointer to the corresponding 
    \a ddjvu_job_t object. */

/*! \fn QDjVuJob::error(QString msg, QString filename, int lineno)
    This signal is emitted when processing a DDJVUAPI \a m_error 
    message that is related to this job. */

/*! \fn QDjVuJob::info(QString msg)
    This signal is emitted when processing a DDJVUAPI \a m_info 
    message that is related to this job. */

/*! \fn QDjVuJob::progress(int percent)
    This signal is emitted when processing a DDJVUAPI \a m_progress 
    message that is related to this job. 
    Argument <percent> indicates the progress level from 1 to 100. */




// ----------------------------------------
// MOC

#include "qdjvu.moc"


/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
