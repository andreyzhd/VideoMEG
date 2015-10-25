#include <QDateTime>
#include <QDebug>
#include <QFile>

#include "settings.h"
#include "markerfilewriter.h"

MarkerFileWriter::MarkerFileWriter()
{    
}


MarkerFileWriter::~MarkerFileWriter()
{

}


void MarkerFileWriter::setComment(QString _comment)
{
    comment = _comment;
}


void MarkerFileWriter::init(quint64 _timestamp, QString _type, QString _comment)
{
    QDateTime   dt;

    type = _type;
    comment = _comment;
    dt.setTime_t(_timestamp/1000);

    fname = Settings::getSettings().markersStoragePath + "/" +
            QString::number(_timestamp) + "--" +
            dt.toString("yyyy-MM-dd--hh-mm-ss");
}


void MarkerFileWriter::flush()
{
    if(!fname.isEmpty())
    {
        QFile file(fname);
        file.open(QIODevice::WriteOnly | QIODevice::Text);
        QTextStream stream(&file);

        stream << type << endl << comment;
        file.close();
    }
}
