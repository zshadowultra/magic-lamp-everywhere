/*
    Based on KWin Magic Lamp effect
    SPDX-FileCopyrightText: 2008 Martin Gräßlin <mgraesslin@kde.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "genieopen.h"
#include "effect/effecthandler.h"
#include <KConfigGroup>
#include <KSharedConfig>

using namespace std::chrono_literals;

namespace KWin
{

GenieOpenEffect::GenieOpenEffect()
{
    reconfigure(ReconfigureAll);
    connect(effects, &EffectsHandler::windowAdded,        this, &GenieOpenEffect::slotWindowAdded);
    connect(effects, &EffectsHandler::windowClosed,       this, &GenieOpenEffect::slotWindowClosed);
    connect(effects, &EffectsHandler::windowDeleted,      this, &GenieOpenEffect::slotWindowDeleted);
    setVertexSnappingMode(RenderGeometry::VertexSnappingMode::None);
}

bool GenieOpenEffect::supported()
{
    return OffscreenEffect::supported() && effects->animationsSupported();
}

void GenieOpenEffect::reconfigure(ReconfigureFlags)
{
    KSharedConfig::openConfig("kwinrc")->reparseConfiguration();
    KConfigGroup cfg = KSharedConfig::openConfig("kwinrc")->group("Effect-GenieOpen");
    int ms = qBound(50, cfg.readEntry("Duration", 250), 2000);
    m_duration = std::chrono::milliseconds(static_cast<int>(animationTime(std::chrono::milliseconds(ms))));

    // Launcher rect — use configured value, or auto-detect from leftmost dock position
    int lx = cfg.readEntry("LauncherX", -1);
    int ly = cfg.readEntry("LauncherY", -1);
    int lw = cfg.readEntry("LauncherW", 38);
    int lh = cfg.readEntry("LauncherH", 38);
    if (lx < 0 || ly < 0) {
        // Auto-detect: find the dock and use its left edge at screen bottom
        for (EffectWindow *window : effects->stackingOrder()) {
            if (window->isDock()) {
                const QRectF dock = window->frameGeometry();
                lx = dock.x();
                ly = dock.y();
                lw = lh = qMin((int)dock.height(), 48);
                break;
            }
        }
        if (lx < 0) { lx = 16; ly = 830; } // last resort fallback
    }
    m_launcherRect = QRect(lx, ly, lw, lh);

    // System tray rect (for notifications/popups)
    int sx = cfg.readEntry("SysTrayX", 1095);
    int sy = cfg.readEntry("SysTrayY", 16);
    int sw = cfg.readEntry("SysTrayW", 296);
    int sh = cfg.readEntry("SysTrayH", 30);
    m_sysTrayRect = QRect(sx, sy, sw, sh);

    m_closeEnabled = cfg.readEntry("CloseAnimation", true);
    m_popupEnabled      = cfg.readEntry("PopupAnimation",   true);
    int popupMs = qBound(50, cfg.readEntry("PopupDuration", 120), 1000);
    m_popupDuration = std::chrono::milliseconds(static_cast<int>(animationTime(std::chrono::milliseconds(popupMs))));
}

void GenieOpenEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
{
    data.mask |= PAINT_SCREEN_WITH_TRANSFORMED_WINDOWS;
    effects->prePaintScreen(data, presentTime);
}

void GenieOpenEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime)
{
    auto it = m_animations.find(w);
    if (it != m_animations.end()) {
        // If window is being minimized, cancel our animation so the default
        // Magic Lamp effect can handle the minimize-to-taskbar animation
        if (!it->isClose && w->isMinimized()) {
            unredirect(w);
            m_animations.erase(it);
            effects->prePaintWindow(view, w, data, presentTime);
            return;
        }
        it->timeLine.advance(presentTime);
        data.setTransformed();
    }
    effects->prePaintWindow(view, w, data, presentTime);
}

void GenieOpenEffect::apply(EffectWindow *w, int mask, WindowPaintData &data, WindowQuadList &quads)
{
    Q_UNUSED(mask)
    Q_UNUSED(data)

    auto it = m_animations.constFind(w);
    if (it == m_animations.constEnd())
        return;

    const qreal progress = it->timeLine.value();

    QRect geo  = w->frameGeometry().toRect();
    QRect icon = w->iconGeometry().toRect();

    IconPosition position = Bottom;

    // Helper: given a point (cursor), determine genie direction and snap icon to window edge
    auto cursorToIcon = [&](QPoint pt) -> std::pair<IconPosition, QRect> {
        // Determine direction by which quadrant of the window the cursor is in
        // (works whether cursor is inside or outside the window)
        const int dx_left  = pt.x() - geo.x();
        const int dx_right = geo.right() - pt.x();
        const int dy_top   = pt.y() - geo.y();
        const int dy_bot   = geo.bottom() - pt.y();
        // Use signed distances: negative means cursor is outside on that side
        // Pick the axis with the smallest absolute distance to determine direction
        // Prefer vertical axis (top/bottom) as genie looks better vertically
        IconPosition pos;
        if (std::abs(dy_top) <= std::abs(dy_bot) && std::abs(dy_top) <= std::abs(dx_left) && std::abs(dy_top) <= std::abs(dx_right))
            pos = Top;
        else if (std::abs(dy_bot) <= std::abs(dx_left) && std::abs(dy_bot) <= std::abs(dx_right))
            pos = Bottom;
        else if (std::abs(dx_left) <= std::abs(dx_right))
            pos = Left;
        else
            pos = Right;
        // Snap icon to the corresponding edge at cursor's lateral position
        QPoint snapped = pt;
        switch (pos) {
        case Top:    snapped.setY(geo.y());      break;
        case Bottom: snapped.setY(geo.bottom()); break;
        case Left:   snapped.setX(geo.x());      break;
        case Right:  snapped.setX(geo.right());  break;
        }
        return {pos, QRect(snapped, QSize(0, 0))};
    };

    // For popups/dialogs, expand from cursor position at time of open
    if (it->isPopup) {
        auto [pos, ic] = cursorToIcon(it->openCursorPos);
        position = pos;
        icon = ic;
    // For close animations, collapse toward cursor position at time of close
    } else if (it->isClose) {
        auto [pos, ic] = cursorToIcon(it->closeCursorPos);
        position = pos;
        icon = ic;
    } else if (!icon.isValid()) {
        // Choose origin based on window type
        if (w->isNotification() || w->isCriticalNotification() || w->isOnScreenDisplay()) {
            // Notifications come from system tray
            icon = m_sysTrayRect;
            const QRectF screen = effects->clientArea(ScreenArea, w);
            position = (m_sysTrayRect.center().y() > screen.center().y()) ? Bottom : Top;
        } else {
            // Normal apps with no taskbar icon come from launcher
            icon = m_launcherRect;
            const QRectF screen = effects->clientArea(ScreenArea, w);
            position = (m_launcherRect.center().y() > screen.center().y()) ? Bottom : Top;
        }
    } else {
        // Find the panel containing the icon to determine position
        EffectWindow *panel = nullptr;
        for (EffectWindow *window : effects->stackingOrder()) {
            if (window->isDock() && window->frameGeometry().intersects(icon)) {
                panel = window;
                break;
            }
        }
        if (panel) {
            const QRectF windowScreen = effects->clientArea(ScreenArea, w);
            if (panel->width() >= panel->height())
                position = (icon.center().y() <= windowScreen.center().y()) ? Top : Bottom;
            else
                position = (icon.center().x() <= windowScreen.center().x()) ? Left : Right;

            if (panel->isHidden()) {
                const QRectF ps = effects->clientArea(ScreenArea, panel);
                switch (position) {
                case Bottom: icon.moveTop(ps.y() + ps.height());   break;
                case Top:    icon.moveTop(ps.y() - icon.height()); break;
                case Left:   icon.moveLeft(ps.x() - icon.width()); break;
                case Right:  icon.moveLeft(ps.x() + ps.width());   break;
                }
            }
        } else {
            // No panel found — determine direction by where icon is relative to window
            if (icon.center().y() > geo.center().y())
                position = Bottom;
            else if (icon.center().y() < geo.center().y())
                position = Top;
            else if (icon.center().x() < geo.center().x())
                position = Left;
            else
                position = Right;
        }
    }

    quads = quads.makeGrid(40);

    float quadFactor;
    float offset[2] = {0, 0};
    float p_progress[2] = {0, 0};
    WindowQuad lastQuad;
    lastQuad[0].setX(-1); lastQuad[0].setY(-1);
    lastQuad[1].setX(-1); lastQuad[1].setY(-1);
    lastQuad[2].setX(-1); lastQuad[2].setY(-1);

    if (position == Bottom) {
        const float height_cube = float(geo.height()) * float(geo.height()) * float(geo.height());
        const float maxY = icon.y() - geo.y();

        for (WindowQuad &quad : quads) {
            if (quad[0].y() != lastQuad[0].y() || quad[2].y() != lastQuad[2].y()) {
                quadFactor = quad[0].y() + (geo.height() - quad[0].y()) * progress;
                offset[0] = (icon.y() + quad[0].y() - geo.y()) * progress * ((quadFactor * quadFactor * quadFactor) / height_cube);
                quadFactor = quad[2].y() + (geo.height() - quad[2].y()) * progress;
                offset[1] = (icon.y() + quad[2].y() - geo.y()) * progress * ((quadFactor * quadFactor * quadFactor) / height_cube);
                p_progress[1] = std::clamp(offset[1] / (icon.y() - geo.y() - float(quad[2].y())), 0.0f, 1.0f);
                p_progress[0] = std::clamp(offset[0] / (icon.y() - geo.y() - float(quad[0].y())), 0.0f, 1.0f);
            } else {
                lastQuad = quad;
            }
            p_progress[0] = std::abs(p_progress[0]);
            p_progress[1] = std::abs(p_progress[1]);
            quad[0].setX((icon.x() + icon.width() * (quad[0].x() / geo.width()) - (quad[0].x() + geo.x())) * p_progress[0] + quad[0].x());
            quad[1].setX((icon.x() + icon.width() * (quad[1].x() / geo.width()) - (quad[1].x() + geo.x())) * p_progress[0] + quad[1].x());
            quad[2].setX((icon.x() + icon.width() * (quad[2].x() / geo.width()) - (quad[2].x() + geo.x())) * p_progress[1] + quad[2].x());
            quad[3].setX((icon.x() + icon.width() * (quad[3].x() / geo.width()) - (quad[3].x() + geo.x())) * p_progress[1] + quad[3].x());
            quad[0].setY(std::min(maxY, float(quad[0].y()) + offset[0]));
            quad[1].setY(std::min(maxY, float(quad[1].y()) + offset[0]));
            quad[2].setY(std::min(maxY, float(quad[2].y()) + offset[1]));
            quad[3].setY(std::min(maxY, float(quad[3].y()) + offset[1]));
        }
    } else if (position == Top) {
        const float height_cube = float(geo.height()) * float(geo.height()) * float(geo.height());
        const float minY = icon.y() + icon.height() - geo.y();

        for (WindowQuad &quad : quads) {
            if (quad[0].y() != lastQuad[0].y() || quad[2].y() != lastQuad[2].y()) {
                quadFactor = geo.height() - quad[0].y() + (quad[0].y()) * progress;
                offset[0] = (geo.y() - icon.height() + geo.height() + quad[0].y() - icon.y()) * progress * ((quadFactor * quadFactor * quadFactor) / height_cube);
                quadFactor = geo.height() - quad[2].y() + (quad[2].y()) * progress;
                offset[1] = (geo.y() - icon.height() + geo.height() + quad[2].y() - icon.y()) * progress * ((quadFactor * quadFactor * quadFactor) / height_cube);
                p_progress[0] = std::clamp(offset[0] / (geo.y() - icon.height() + geo.height() - icon.y() - float(geo.height() - quad[0].y())), 0.0f, 1.0f);
                p_progress[1] = std::clamp(offset[1] / (geo.y() - icon.height() + geo.height() - icon.y() - float(geo.height() - quad[2].y())), 0.0f, 1.0f);
            } else {
                lastQuad = quad;
            }
            offset[0] = -offset[0]; offset[1] = -offset[1];
            p_progress[0] = std::abs(p_progress[0]); p_progress[1] = std::abs(p_progress[1]);
            quad[0].setX((icon.x() + icon.width() * (quad[0].x() / geo.width()) - (quad[0].x() + geo.x())) * p_progress[0] + quad[0].x());
            quad[1].setX((icon.x() + icon.width() * (quad[1].x() / geo.width()) - (quad[1].x() + geo.x())) * p_progress[0] + quad[1].x());
            quad[2].setX((icon.x() + icon.width() * (quad[2].x() / geo.width()) - (quad[2].x() + geo.x())) * p_progress[1] + quad[2].x());
            quad[3].setX((icon.x() + icon.width() * (quad[3].x() / geo.width()) - (quad[3].x() + geo.x())) * p_progress[1] + quad[3].x());
            quad[0].setY(std::max(minY, float(quad[0].y()) + offset[0]));
            quad[1].setY(std::max(minY, float(quad[1].y()) + offset[0]));
            quad[2].setY(std::max(minY, float(quad[2].y()) + offset[1]));
            quad[3].setY(std::max(minY, float(quad[3].y()) + offset[1]));
        }
    } else if (position == Left) {
        const float width_cube = float(geo.width()) * float(geo.width()) * float(geo.width());
        const float minX = icon.x() + icon.width() - geo.x();

        for (WindowQuad &quad : quads) {
            if (quad[0].x() != lastQuad[0].x() || quad[1].x() != lastQuad[1].x()) {
                quadFactor = geo.width() - quad[0].x() + (quad[0].x()) * progress;
                offset[0] = (geo.x() - icon.width() + geo.width() + quad[0].x() - icon.x()) * progress * ((quadFactor * quadFactor * quadFactor) / width_cube);
                quadFactor = geo.width() - quad[1].x() + (quad[1].x()) * progress;
                offset[1] = (geo.x() - icon.width() + geo.width() + quad[1].x() - icon.x()) * progress * ((quadFactor * quadFactor * quadFactor) / width_cube);
                p_progress[0] = std::clamp(offset[0] / (geo.x() - icon.width() + geo.width() - icon.x() - float(geo.width() - quad[0].x())), 0.0f, 1.0f);
                p_progress[1] = std::clamp(offset[1] / (geo.x() - icon.width() + geo.width() - icon.x() - float(geo.width() - quad[1].x())), 0.0f, 1.0f);
            } else {
                lastQuad = quad;
            }
            offset[0] = -offset[0]; offset[1] = -offset[1];
            p_progress[0] = std::abs(p_progress[0]); p_progress[1] = std::abs(p_progress[1]);
            quad[0].setY((icon.y() + icon.height() * (quad[0].y() / geo.height()) - (quad[0].y() + geo.y())) * p_progress[0] + quad[0].y());
            quad[1].setY((icon.y() + icon.height() * (quad[1].y() / geo.height()) - (quad[1].y() + geo.y())) * p_progress[1] + quad[1].y());
            quad[2].setY((icon.y() + icon.height() * (quad[2].y() / geo.height()) - (quad[2].y() + geo.y())) * p_progress[1] + quad[2].y());
            quad[3].setY((icon.y() + icon.height() * (quad[3].y() / geo.height()) - (quad[3].y() + geo.y())) * p_progress[0] + quad[3].y());
            quad[0].setX(std::max(minX, float(quad[0].x()) + offset[0]));
            quad[1].setX(std::max(minX, float(quad[1].x()) + offset[1]));
            quad[2].setX(std::max(minX, float(quad[2].x()) + offset[1]));
            quad[3].setX(std::max(minX, float(quad[3].x()) + offset[0]));
        }
    } else if (position == Right) {
        const float width_cube = float(geo.width()) * float(geo.width()) * float(geo.width());
        const float maxX = icon.x() - geo.x();

        for (WindowQuad &quad : quads) {
            if (quad[0].x() != lastQuad[0].x() || quad[1].x() != lastQuad[1].x()) {
                quadFactor = quad[0].x() + (geo.width() - quad[0].x()) * progress;
                offset[0] = (icon.x() + quad[0].x() - geo.x()) * progress * ((quadFactor * quadFactor * quadFactor) / width_cube);
                quadFactor = quad[1].x() + (geo.width() - quad[1].x()) * progress;
                offset[1] = (icon.x() + quad[1].x() - geo.x()) * progress * ((quadFactor * quadFactor * quadFactor) / width_cube);
                p_progress[0] = std::clamp(offset[0] / (icon.x() - geo.x() - float(quad[0].x())), 0.0f, 1.0f);
                p_progress[1] = std::clamp(offset[1] / (icon.x() - geo.x() - float(quad[1].x())), 0.0f, 1.0f);
            } else {
                lastQuad = quad;
            }
            p_progress[0] = std::abs(p_progress[0]); p_progress[1] = std::abs(p_progress[1]);
            quad[0].setY((icon.y() + icon.height() * (quad[0].y() / geo.height()) - (quad[0].y() + geo.y())) * p_progress[0] + quad[0].y());
            quad[1].setY((icon.y() + icon.height() * (quad[1].y() / geo.height()) - (quad[1].y() + geo.y())) * p_progress[1] + quad[1].y());
            quad[2].setY((icon.y() + icon.height() * (quad[2].y() / geo.height()) - (quad[2].y() + geo.y())) * p_progress[1] + quad[2].y());
            quad[3].setY((icon.y() + icon.height() * (quad[3].y() / geo.height()) - (quad[3].y() + geo.y())) * p_progress[0] + quad[3].y());
            quad[0].setX(std::min(maxX, float(quad[0].x()) + offset[0]));
            quad[1].setX(std::min(maxX, float(quad[1].x()) + offset[1]));
            quad[2].setX(std::min(maxX, float(quad[2].x()) + offset[1]));
            quad[3].setX(std::min(maxX, float(quad[3].x()) + offset[0]));
        }
    }
}

void GenieOpenEffect::postPaintScreen()
{
    auto it = m_animations.begin();
    while (it != m_animations.end()) {
        if (it->timeLine.done()) {
            if (it->isClose)
                it.key()->setData(WindowClosedGrabRole, QVariant());
            unredirect(it.key());
            it = m_animations.erase(it);
        } else {
            ++it;
        }
    }
    if (!m_animations.isEmpty())
        effects->addRepaintFull();
    effects->postPaintScreen();
}

void GenieOpenEffect::slotWindowAdded(EffectWindow *w)
{
    qWarning() << "[GenieOpen] windowAdded:" << w->caption() << "class:" << w->windowClass() << "normal:" << w->isNormalWindow() << "popup:" << w->isPopupWindow() << "special:" << w->isSpecialWindow();

    if (effects->activeFullScreenEffect())
        return;
    if (w->isDesktop() || w->isDock() || w->isSplash() || w->isSpecialWindow())
        return;
    if (w->isTooltip() || w->isComboBox())
        return;
    if ((w->isDropdownMenu() || w->isPopupMenu()) && !m_popupEnabled)
        return;
    if (w->width() < 50 || w->height() < 50)
        return;
    if (m_animations.contains(w))
        return;
    // Don't animate windows being restored from minimize (alt-tab)
    if (w->isMinimized())
        return;

    const QString wClass = w->windowClass().trimmed().toLower();

    // Skip KWin internal windows — empty or whitespace-only class
    if (wClass.trimmed().isEmpty())
        return;

    // Skip plasmashell windows (launcher, notifications, panels)
    if (wClass.contains("plasmashell"))
        return;

    // Skip kwin's own windows
    if (wClass.startsWith("kwin"))
        return;

    const bool isPopup = !w->isNormalWindow() || w->isPopupWindow() || w->isDialog()
                         || w->isDropdownMenu() || w->isPopupMenu();

    qWarning() << "[GenieOpen] Starting animation for:" << w->caption();

    if (m_animations.contains(w)) {
        unredirect(w);
        m_animations.remove(w);
    }

    GenieAnimation &anim = m_animations[w];
    anim.isPopup      = isPopup;
    anim.openCursorPos = isPopup ? cursorPos().toPoint() : QPoint();
    anim.visibleRef   = EffectWindowVisibleRef(w, EffectWindow::PAINT_DISABLED_BY_MINIMIZE);
    anim.timeLine.setDirection(TimeLine::Backward);
    anim.timeLine.setDuration(isPopup ? m_popupDuration : m_duration);
    anim.timeLine.setEasingCurve(QEasingCurve::Linear);

    redirect(w);
    effects->addRepaintFull();
}

void GenieOpenEffect::slotWindowClosed(EffectWindow *w)
{
    qWarning() << "[GenieOpen] windowClosed:" << w->caption() << "class:" << w->windowClass() << "closeEnabled:" << m_closeEnabled;

    if (!m_closeEnabled)
        return;
    if (w->isDesktop() || w->isDock() || w->isSplash() || w->isSpecialWindow())
        return;
    if (w->isTooltip() || w->isComboBox())
        return;
    if ((w->isDropdownMenu() || w->isPopupMenu() || w->isPopupWindow()) && !m_popupEnabled)
        return;

    const QString wClass = w->windowClass().trimmed().toLower();
    if (wClass.isEmpty() || wClass.contains(QStringLiteral("plasmashell")) || wClass.startsWith(QStringLiteral("kwin")))
        return;

    qWarning() << "[GenieOpen] Starting close animation for:" << w->caption();

    // Remove any existing open animation for this window
    if (m_animations.contains(w)) {
        unredirect(w);
        m_animations.remove(w);
    }

    GenieAnimation &anim = m_animations[w];
    anim.isClose        = true;
    anim.closeCursorPos = cursorPos().toPoint();
    w->setData(WindowClosedGrabRole, QVariant::fromValue(static_cast<void*>(this)));
    anim.timeLine.setDirection(TimeLine::Forward);
    anim.timeLine.setDuration(m_duration);
    anim.timeLine.setEasingCurve(QEasingCurve::Linear);
    anim.timeLine.reset();

    redirect(w);
    effects->addRepaintFull();
}

void GenieOpenEffect::slotWindowDeleted(EffectWindow *w)
{
    auto it = m_animations.find(w);
    if (it != m_animations.end()) {
        if (it->isClose)
            w->setData(WindowClosedGrabRole, QVariant());
        unredirect(w);
        m_animations.erase(it);
    }
}

bool GenieOpenEffect::isActive() const
{
    return !m_animations.isEmpty();
}

} // namespace KWin
