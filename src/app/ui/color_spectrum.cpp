// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_spectrum.h"

#include "app/color_utils.h"
#include "app/modules/gfx.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "she/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/resize_event.h"
#include "ui/system.h"

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

ColorSpectrum::ColorSpectrum()
  : m_lastSaturation(100)
{
  setAlign(HORIZONTAL);
  setBorder(gfx::Border(3*ui::guiscale()));
}

app::Color ColorSpectrum::getColorByPosition(const gfx::Point& pos)
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty())
    return app::Color::fromMask();

  int u, v, umax;
  int baroffset = getBarsSize();

  double hue, sat, val, alpha;

  int vmid = (align() & HORIZONTAL ? (rc.h-baroffset)/2 : rc.w/2);
  vmid = MAX(1, vmid);

  bool inSat = hasCapture() ? m_capturedBar == CapturedBar::SATURATION_BAR : inSaturationBarArea(pos);
  bool inAlpha = hasCapture() ? m_capturedBar == CapturedBar::ALPHA_BAR : inAlphaBarArea(pos);

  if (inSat) {
    u = pos.x - rc.x;
    umax = MAX(1, rc.w-1);

    hue = m_color.getHue();
    sat = m_color.getSaturation();
    val = m_color.getValue();
    alpha = m_color.getAlpha();

    double lit = (200.0 - sat) * val / 200.0;

    val = lit + MID(0.0, 1.0 * u / umax, 1.0) * std::min(lit, 100.0 - lit);
    sat = val == 0 ? 0 : 200.0 * (1.0 - lit / val);
  }
  else if (inAlpha) {
    u = pos.x - rc.x;
    umax = MAX(1, rc.w-1);

    hue = m_color.getHue();
    sat = m_color.getSaturation();
    val = m_color.getValue();
    alpha = (255 * u / umax);
  } else {
    if (align() & HORIZONTAL) {
      u = pos.x - rc.x;
      v = pos.y - rc.y;
      umax = MAX(1, rc.w-1);
    }
    else {
      u = pos.y - rc.y;
      v = pos.x - rc.x;
      umax = MAX(1, rc.h-baroffset-1);
    }

    sat = m_color.getSaturation();
    val = m_color.getValue();
    double lit = (200.0 - sat) * val / 200.0;
    double satl;
    if (lit <= 0.0 || lit >= 100.0) {
      satl = m_lastSaturation;
    } else {
      m_lastSaturation = satl = (val - lit) / std::min(lit, 100.0 - lit) * 100.0;
    }
    lit = MID(0.0, 100.0 - 50.0 * v / vmid, 100.0);

    hue = 360.0 * u / umax;
    val = lit + satl * std::min(lit, 100.0 - lit) / 100.0;
    sat = val == 0 ? 0 : 200.0 * (1.0 - lit / val);
    alpha = m_color.getAlpha();
  }

  return app::Color::fromHsv(
    MID(0.0, hue, 360.0),
    MID(0.0, sat, 100.0),
    MID(0.0, val, 100.0),
    MID(0, alpha, 255));
}

void ColorSpectrum::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  theme->drawRect(g, clientBounds(),
                  theme->parts.editorNormal().get(),
                  bgColor());

  gfx::Rect rc = clientChildrenBounds();
  if (rc.isEmpty())
    return;
    
  int barcount = getBarsCount();
  int barsize = getOneBarSize();
  int baroffset = barcount * barsize;

  int vmid = (align() & HORIZONTAL ? (rc.h-baroffset)/2 : rc.w/2);
  vmid = MAX(1, vmid);

  for (int y=0; y<(rc.h-baroffset); ++y) {
    for (int x=0; x<rc.w; ++x) {
      int u, v, umax;
      if (align() & HORIZONTAL) {
        u = x;
        v = y;
        umax = MAX(1, rc.w-1);
      }
      else {
        u = y;
        v = x;
        umax = MAX(1, rc.h-baroffset-1);
      }

      double hue = 360.0 * u / umax;
      double sat = (v < vmid ? 100.0 * v / vmid : 100.0);
      double val = (v < vmid ? 100.0 : 100.0-(100.0 * (v-vmid) / vmid));

      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsv(
          MID(0.0, hue, 360.0),
          MID(0.0, sat, 100.0),
          MID(0.0, val, 100.0)));

      g->putPixel(color, rc.x+x, rc.y+y);
    }
  }

  // Saturation bar
  if (barcount > 0) {
    double hue = m_color.getHue();
    double sat = m_color.getSaturation();
    double val = m_color.getValue();
    double lit = (200.0 - sat) * val / 200.0;
    double u;
    for (int x=0; x<rc.w; ++x) {
      u = 1.0 * x / rc.w;
      val = lit + u * std::min(lit, 100.0 - lit);
      sat = val == 0 ? 0 : 200.0 * (1.0 - lit / val);
      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsv(
          hue,
          sat,
          val
        ));

      g->drawVLine(color, rc.x+x, rc.y+rc.h-baroffset, barsize);
    }
  }

  // Alpha bar
  if (barcount > 1) {
    Rect gridrect = RectT(rc.x, rc.y+rc.h-baroffset+barsize, rc.w, barsize);
    draw_alpha_picker(g, gridrect, m_color);
  }

  if (m_color.getType() != app::Color::MaskType) {
    double hue = m_color.getHue();
    double sat = m_color.getSaturation();
    double val = m_color.getValue();
    double lit = (200.0 - sat) * val / 200.0;
    gfx::Point pos(rc.x + int(hue * rc.w / 360.0),
                   rc.y + rc.h - baroffset - int(lit * (rc.h - baroffset) / 100.0));

    she::Surface* wheel = theme->parts.colorWheelIndicator()->bitmap(0);
    she::Surface* line = theme->parts.colorLineIndicator()->bitmap(0);

    g->drawColoredRgbaSurface(
      wheel,
      m_color.getLuma() > 50.0 ? gfx::rgba(0, 0, 0): gfx::rgba(255, 255, 255),
      pos.x-wheel->width()/2,
      pos.y-wheel->height()/2);

    if (barcount > 0) {
      double lit = (200.0 - sat) * val / 200.0;
      if (lit > 0.0 && lit < 100.0) {
        double satl = (val - lit) / std::min(lit, 100.0 - lit) * 100.0;
        pos.x = rc.x + int(rc.w * satl / 100.0);
        pos.y = rc.y + rc.h - baroffset + barsize/2;
        g->drawColoredRgbaSurface(
          line,
          m_color.getLuma() > 50.0 ? gfx::rgba(0, 0, 0) : gfx::rgba(255, 255, 255),
          pos.x-line->width()/2,
          pos.y-line->height()/2);
      }
    }

    // Alpha bar
    if (barcount > 1) {
      pos.x = rc.x + int(rc.w * m_color.getAlpha() / 255.0);
      pos.y = rc.y + rc.h - baroffset + 3*barsize/2;
      g->drawColoredRgbaSurface(
        line,
        m_color.getLuma() > 50.0 ? gfx::rgba(0, 0, 0) : gfx::rgba(255, 255, 255),
        pos.x-line->width()/2,
        pos.y-line->height()/2);
    }
  }
}

bool ColorSpectrum::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      captureMouse();
      // Continue...

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (msg->type() == kMouseDownMessage) {
        if (inSaturationBarArea(mouseMsg->position())) 
          m_capturedBar = CapturedBar::SATURATION_BAR;
        else if (inAlphaBarArea(mouseMsg->position())) 
          m_capturedBar = CapturedBar::ALPHA_BAR;
        else 
          m_capturedBar = CapturedBar::NONE;
      }

      app::Color color = getColorByPosition(mouseMsg->position());
      if (color != app::Color::fromMask()) {
        StatusBar::instance()->showColor(0, "", color);
        if (hasCapture())
          ColorChange(color, mouseMsg->buttons());
      }
      break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
      }
      return true;

    case kSetCursorMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      if (childrenBounds().contains(mouseMsg->position())) {
        ui::set_mouse_cursor(kEyedropperCursor);
        return true;
      }
      break;
    }

  }

  return ColorSelector::onProcessMessage(msg);
}

bool ColorSpectrum::inSaturationBarArea(const gfx::Point& pos) const
{
  gfx::Rect rc = childrenBounds();
  int barsize = getOneBarSize();
  int baroffset = barsize * getBarsCount();
  if (rc.isEmpty() || !rc.contains(pos))
    return false;
  else {
    int offset = rc.y + rc.h - baroffset;
    return (pos.y >= offset && pos.y < offset+barsize);
  }
}

bool ColorSpectrum::inAlphaBarArea(const gfx::Point& pos) const
{
  gfx::Rect rc = childrenBounds();
  int barsize = getOneBarSize();
  int baroffset = barsize * getBarsCount();
  if (rc.isEmpty() || !rc.contains(pos))
    return false;
  else {
    int offset = rc.y + rc.h - baroffset + barsize;
    return (pos.y >= offset && pos.y < offset+barsize);
  }
}

int ColorSpectrum::getOneBarSize() const
{
  return 8*guiscale();
}

int ColorSpectrum::getBarsCount() const
{
  gfx::Rect rc = clientChildrenBounds();
  int onesize = getOneBarSize();
  return MID(0, rc.h / onesize / 2 - 1, 2);
}

int ColorSpectrum::getBarsSize() const
{
  return getBarsCount() * getOneBarSize();
}

} // namespace app
