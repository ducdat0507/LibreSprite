// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/color_selector.h"

namespace app {

  class ColorTintShadeTone : public ColorSelector {
  public:
    ColorTintShadeTone();

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;

  private:
    enum CapturedBar {
      NONE = 0,
      HUE_BAR = 1,
      ALPHA_BAR = 2,
    };

    bool inHueBarArea(const gfx::Point& pos) const;
    bool inAlphaBarArea(const gfx::Point& pos) const;
    int getOneBarSize() const;
    int getBarsCount() const;
    int getBarsSize() const;

    // True when the user pressed the mouse button in the hue slider.
    // It's used to avoid swapping in both areas (tint/shades/tones
    // area vs hue slider) when we drag the mouse above this widget.
    int m_capturedBar;
  };

} // namespace app
