#include "TiffCompressionInfo.h"

#include <QResource>

QMap<QString, TiffCompressionInfo>
init_data()
{
    Q_INIT_RESOURCE(core_resources);

    QMap<QString, TiffCompressionInfo> data;
    const QResource tiff_data(":/TiffCompressionMethods.tsv");
    const QStringList sl = QString::fromUtf8((char const*)tiff_data.data(), tiff_data.size()).split('\n');

    QStringList vals;
    for (QString const& s: sl) {
        vals = s.split('\t');
        if (vals.count() < 5 ||
                vals[0].startsWith("#")) continue;
        TiffCompressionInfo info;
        info.name = vals[0];
        info.id = vals[1].toInt();
        info.description = vals[2];
        info.always_shown = vals[3].toInt();
        info.for_bw_only = vals[4].toInt();
        data[info.name] = info;
    }
    return data;
}

QMap<QString, TiffCompressionInfo> TiffCompressions::compression_data = init_data();

const TiffCompressionInfo&
TiffCompressions::info(QString const& name)
{
    return compression_data[name];
}
