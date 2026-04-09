#include "genieopen_config.h"
#include "genieopenconfig.h"
#include <KPluginFactory>
#include <KSharedConfig>
#include <QDBusConnection>
#include <QDBusInterface>

namespace KWin
{

GenieOpenEffectConfig::GenieOpenEffectConfig(QObject *parent, const KPluginMetaData &data)
    : KCModule(parent, data)
{
    m_ui.setupUi(widget());
    GenieOpenConfig::instance(KSharedConfig::openConfig("kwinrc"));
    addConfig(GenieOpenConfig::self(), widget());
}

void GenieOpenEffectConfig::save()
{
    KCModule::save();
    QDBusInterface iface(QStringLiteral("org.kde.KWin"),
                         QStringLiteral("/Effects"),
                         QStringLiteral("org.kde.kwin.Effects"),
                         QDBusConnection::sessionBus());
    iface.call(QStringLiteral("reconfigureEffect"), QStringLiteral("kwin_genie_open"));
}

K_PLUGIN_CLASS_WITH_JSON(GenieOpenEffectConfig, "genieopen_config.json")

} // namespace KWin

#include "genieopen_config.moc"
