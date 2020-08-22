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

#ifndef QDJVU_H
#define QDJVU_H

#if AUTOCONF
# include "config.h"
#endif

// The following definition allow for using 
// this file without the libdjvu include files.

#ifndef DDJVUAPI_H
typedef struct ddjvu_context_s    ddjvu_context_t;
typedef union  ddjvu_message_s    ddjvu_message_t;
typedef struct ddjvu_job_s        ddjvu_job_t;
typedef struct ddjvu_document_s   ddjvu_document_t;
typedef struct ddjvu_page_s       ddjvu_page_t;
#endif
#ifndef MINIEXP_H
typedef struct miniexp_s* miniexp_t;
#endif


// Qt includes

#include <QObject>
#include <QString>
#include <QUrl>


// The following classes integrate the ddjvuapi with 
// the Qt memory management and event management scheme. 
// All ddjvuapi events are converted into signals sent
// when the Qt event loop is called. These signal are
// emitted by all objects listed in the event message.
//
// All classes below define implicit conversion 
// into pointers to the corresponding ddjvuapi objects.

class QDjVuContext;
class QDjVuDocument;
class QDjVuDocumentPrivate;
class QDjVuPage;
class QDjVuJob;


class QDjVuContext : public QObject 
{
  Q_OBJECT
  Q_PROPERTY(long cacheSize READ cacheSize WRITE setCacheSize)

private:
  static void callback(ddjvu_context_t *context, void *closure);
  ddjvu_context_t *context;
  bool flag;  // might become private pointer in the future
  
protected:
  virtual bool handle(ddjvu_message_t*);

public:
  virtual ~QDjVuContext();
  QDjVuContext(const char *programname=0, QObject *parent=0);
  long cacheSize() const;
  void setCacheSize(long);
  virtual bool event(QEvent*);
  operator ddjvu_context_t*() { return context; }
  
signals:
  void error(QString msg, QString filename, int lineno);
  void info(QString msg);
};


class QDjVuDocument : public QObject
{
  Q_OBJECT

private:
  friend class QDjVuContext;
  friend class QDjVuPage;
  friend class QDjVuJob;
  ddjvu_document_t *document;
  QDjVuDocumentPrivate *priv;

protected:
  virtual bool handle(ddjvu_message_t*);

public:
  virtual ~QDjVuDocument();
  QDjVuDocument(QObject *parent);
  QDjVuDocument(bool autoDelete=false, QObject *parent=0);
  void ref();
  void deref();
  bool setFileName(QDjVuContext *ctx, QString filename, bool cache=true);
protected:
  bool setUrl(QDjVuContext *ctx, QUrl url);
  bool setUrl(QDjVuContext *ctx, QUrl url, bool cache);
  virtual void newstream(int streamid, QString name, QUrl url);
public:
  void streamWrite(int streamid, const char *data, unsigned long len );
  void streamClose(int streamid, bool stop = false);
  operator ddjvu_document_t*() { return document; }
  virtual bool isValid() { return document != 0; }
public:
  int runningProcesses(void);
  miniexp_t getDocumentAnnotations();
  miniexp_t getDocumentOutline();
  miniexp_t getPageAnnotations(int pageno, bool start=true);
  miniexp_t getPageText(int pageno, bool start=true);


signals:
  void error(QString msg, QString filename, int lineno);
  void info(QString msg);
  void docinfo(void);
  void pageinfo(void);
  void thumbnail(int pagenum);
  void idle(void);
};


class QDjVuPage : public QObject
{
  Q_OBJECT

private:
  friend class QDjVuContext;
  friend class QDjVuDocument;
  ddjvu_page_t *page;
  int pageno; // might become private pointer in the future

protected:
  virtual bool handle(ddjvu_message_t*);
  
public:
  virtual ~QDjVuPage();
  QDjVuPage(QDjVuDocument *doc, int pageno, QObject *parent=0);
  operator ddjvu_page_t*() { return page; }
  bool isValid() { return page!=0; }
  int pageNo();

signals:
  void error(QString msg, QString filename, int lineno);
  void info(QString msg);
  void chunk(QString chunkid);
  void pageinfo();
  void relayout();
  void redisplay();
};


class QDjVuJob : public QObject
{
  Q_OBJECT
  
private:
  friend class QDjVuContext;
  friend class QDjVuDocument;
  ddjvu_job_t *job;

protected:
  virtual bool handle(ddjvu_message_t*);
  
public:
  virtual ~QDjVuJob();
  QDjVuJob(ddjvu_job_t *job, QObject *parent=0);
  operator ddjvu_job_t*() { return job; }
  
signals:
  void error(QString msg, QString filename, int lineno);
  void info(QString msg);
  void progress(int percent);
};




#endif

/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "Q[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
