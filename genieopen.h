/*
    Based on KWin Magic Lamp effect
    SPDX-FileCopyrightText: 2008 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "effect/effectwindow.h"
#include "effect/offscreeneffect.h"
#include "effect/timeline.h"

namespace KWin
{

struct GenieAnimation {
    EffectWindowVisibleRef visibleRef;
    TimeLine timeLine;
    bool isClose = false;
    bool isPopup = false;
};

class GenieOpenEffect : public OffscreenEffect
{
    Q_OBJECT

public:
    GenieOpenEffect();

    void reconfigure(ReconfigureFlags) override;
    void prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime) override;
    void postPaintScreen() override;
    bool isActive() const override;
    int  requestedEffectChainPosition() const override { return 50; }
    static bool supported();

protected:
    void apply(EffectWindow *w, int mask, WindowPaintData &data, WindowQuadList &quads) override;

private Q_SLOTS:
    void slotWindowAdded(EffectWindow *w);
    void slotWindowClosed(EffectWindow *w);
    void slotWindowDeleted(EffectWindow *w);

private:
    enum IconPosition { Top, Bottom, Left, Right };

    std::chrono::milliseconds m_duration;
    std::chrono::milliseconds m_popupDuration;
    QHash<EffectWindow *, GenieAnimation> m_animations;
    QRect m_launcherRect;
    QRect m_sysTrayRect;
    bool m_closeEnabled   = false;
    bool m_popupEnabled   = true;
};

} // namespace KWin
