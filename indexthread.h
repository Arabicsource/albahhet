#ifndef INDEXTHREAD_H
#define INDEXTHREAD_H

#include <QThread>
#include "arabicanalyzer.h"
#include "common.h"

class IndexingThread : public QThread
{
    Q_OBJECT
public:
    IndexingThread();
    void run();
    void stop() { m_stopIndexing = true; }
    void setOptions(bool optimizeIndex, int ramSize, int maxDoc);

protected:
    void indexBook(IndexWriter *writer, const QString &bookID, const QString &bookPath);
    Document* FileDocument(const QString &id, const QString &bookid, const QString &text);
    void startIndexing();

signals:
    void fileIndexed(const QString &bookName);

protected:
    bool m_stopIndexing;
    bool m_optimizeIndex;
    int m_ramSize;
    int m_maxDoc;
};

#endif // INDEXTHREAD_H
