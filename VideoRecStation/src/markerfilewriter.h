#ifndef MARKERFILEWRITER_H
#define MARKERFILEWRITER_H

#include <QString>

class MarkerFileWriter
{
        // TODO: current implementation is really dirty. Fix it.
public:
    MarkerFileWriter();
    virtual ~MarkerFileWriter();
    void init(quint64 _timestamp, QString _type, QString _comment);
    void setComment(QString _comment);
    void flush();

private:
    QString type;
    QString comment;
    QString fname;
};

#endif // MARKERFILEWRITER_H
