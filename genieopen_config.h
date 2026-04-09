#pragma once

#include <kcmodule.h>
#include <KPluginFactory>
#include "ui_genieopen_config.h"

namespace KWin
{

class GenieOpenEffectConfig : public KCModule
{
    Q_OBJECT
public:
    explicit GenieOpenEffectConfig(QObject *parent, const KPluginMetaData &data);

public Q_SLOTS:
    void save() override;

private:
    Ui::GenieOpenEffectConfigForm m_ui;
};

} // namespace KWin
