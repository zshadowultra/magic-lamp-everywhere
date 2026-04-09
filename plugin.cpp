#include "genieopen.h"

KWIN_EFFECT_FACTORY_SUPPORTED_ENABLED(KWin::GenieOpenEffect,
                                      "metadata.json",
                                      return KWin::GenieOpenEffect::supported();,
                                      return false;)

#include "plugin.moc"
