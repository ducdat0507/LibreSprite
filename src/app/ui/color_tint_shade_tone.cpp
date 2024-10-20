// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_tint_shade_tone.h"

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

ColorTintShadeTone::ColorTintShadeTone()
  : m_capturedBar(CapturedBar::NONE)
{
  setBorder(gfx::Border(3*ui::guiscale()));
}

app::Color ColorTintShadeTone::getColorByPosition(const gfx::Point& pos)
{
  gfx::Rect rc = childrenBounds();
  if (rc.isEmpty())
    return app::Color::fromMask();

  int u, v, umax, vmax;
  int huebar = getBarsSize();
  u = pos.x - rc.x;
  v = pos.y - rc.y;
  umax = MAX(1, rc.w-1);
  vmax = MAX(1, rc.h-1-huebar);

  double hue, sat, val;
  int alpha;

  bool inHue = hasCapture() ? m_capturedBar == CapturedBar::HUE_BAR : inHueBarArea(pos);
  bool inAlpha = hasCapture() ? m_capturedBar == CapturedBar::ALPHA_BAR : inAlphaBarArea(pos);

  if (inHue) {
    hue = (360.0 * u / umax);
    sat = m_color.getSaturation();
    val = m_color.getValue();
    alpha = m_color.getAlpha();
  }
  else if (inAlpha) {
    hue = m_color.getHue();
    sat = m_color.getSaturation();
    val = m_color.getValue();
    alpha = (255 * u / umax);
  }
  else {
    hue = m_color.getHue();
    sat = (100.0 * u / umax);
    val = 100.0 - (100.0 * v / vmax);
    alpha = m_color.getAlpha();
  }

  return app::Color::fromHsv(
    MID(0.0, hue, 360.0),
    MID(0.0, sat, 100.0),
    MID(0.0, val, 100.0),
    MID(0, alpha, 255));
}

void ColorTintShadeTone::onPaint(ui::PaintEvent& ev)
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

  double hue = m_color.getHue();
  int umax, vmax;
  umax = MAX(1, rc.w-1);
  vmax = MAX(1, rc.h-1-baroffset);

  for (int y=0; y<rc.h-baroffset; ++y) {
    for (int x=0; x<rc.w; ++x) {
      double sat = (100.0 * x / umax);
      double val = (100.0 - 100.0 * y / vmax);

      gfx::Color color = color_utils::color_for_ui(
        app::Color::fromHsv(
          hue,
          MID(0.0, sat, 100.0),
          MID(0.0, val, 100.0)));

      g->putPixel(color, rc.x+x, rc.y+y);
    }
  }

  if (barcount > 0) {
    for (int y=0; y<barsize; ++y) {
      for (int x=0; x<rc.w; ++x) {
        gfx::Color color = color_utils::color_for_ui(
          app::Color::fromHsv(
            (360.0 * x / rc.w),
            100.0,
            100.0
          ));

        g->putPixel(color, rc.x+x, rc.y+rc.h-baroffset+y);
      }
    }
  }

  if (barcount > 1) {
    Rect gridrect = RectT(rc.x, rc.y+rc.h-baroffset+barsize, rc.w, barsize);
    draw_alpha_picker(g, gridrect, m_color);
  }

  if (m_color.getType() != app::Color::MaskType) {
    double sat = m_color.getSaturation();
    double val = m_color.getValue();
    double alpha = m_color.getAlpha();
    double luma = m_color.getLuma();
    gfx::Point pos(rc.x + int(sat * rc.w / 100.0),
                   rc.y + int((100.0-val) * (rc.h-baroffset) / 100.0));

    she::Surface* wheel = theme->parts.colorWheelIndicator()->bitmap(0);
    she::Surface* line = theme->parts.colorLineIndicator()->bitmap(0);

    g->drawColoredRgbaSurface(
      wheel,
      luma > 50.0 ? gfx::rgba(0, 0, 0) : gfx::rgba(255, 255, 255),
      pos.x-wheel->width()/2,
      pos.y-wheel->height()/2);

    if (barcount > 0) {
      pos.x = rc.x + int(rc.w * hue / 360.0);
      pos.y = rc.y + rc.h - baroffset + barsize/2;
      app::Color color = app::Color::fromHsv(hue, 100.0, 100.0);
      g->drawColoredRgbaSurface(
        line,
        color.getLuma() > 50.0 ? gfx::rgba(0, 0, 0) : gfx::rgba(255, 255, 255),
        pos.x-line->width()/2,
        pos.y-line->height()/2);
    }

    if (barcount > 1) {
      pos.x = rc.x + int(rc.w * alpha / 255.0);
      pos.y = rc.y + rc.h - baroffset + 3*barsize/2;
      g->drawColoredRgbaSurface(
        line,
        m_color.getLuma() > 50.0 ? gfx::rgba(0, 0, 0) : gfx::rgba(255, 255, 255),
        pos.x-line->width()/2,
        pos.y-line->height()/2);
    }
  }
}

bool ColorTintShadeTone::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      if (manager()->getCapture())
        break;

      captureMouse();

      // Continue...

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);

      if (msg->type() == kMouseDownMessage) {
        if (inHueBarArea(mouseMsg->position())) 
          m_capturedBar = CapturedBar::HUE_BAR;
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

bool ColorTintShadeTone::inHueBarArea(const gfx::Point& pos) const
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

bool ColorTintShadeTone::inAlphaBarArea(const gfx::Point& pos) const
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

int ColorTintShadeTone::getOneBarSize() const
{
  return 8*guiscale();
}

int ColorTintShadeTone::getBarsCount() const
{
  gfx::Rect rc = clientChildrenBounds();
  int onesize = getOneBarSize();
  return MID(0, rc.h / onesize / 2 - 1, 2);
}

int ColorTintShadeTone::getBarsSize() const
{
  return getBarsCount() * getOneBarSize();
}

} // namespace app
