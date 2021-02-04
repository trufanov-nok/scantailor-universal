#ifndef TIFFCOMPRESSIONINFO_H
#define TIFFCOMPRESSIONINFO_H

#include <QMap>

struct TiffCompressionInfo
{
    QString name;
    int id;
    QString description;
    bool always_shown;
    bool for_bw_only;
};

class TiffCompressions
{
public:
    static const TiffCompressionInfo& info(QString const& name);
    static QMap<QString, TiffCompressionInfo>::const_iterator constBegin() { return compression_data.constBegin(); }
    static QMap<QString, TiffCompressionInfo>::const_iterator constEnd() { return compression_data.constEnd(); }
private:
    static QMap<QString, TiffCompressionInfo> compression_data;
};

#endif // TIFFCOMPRESSIONINFO_H
