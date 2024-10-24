// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#pragma once

#include "app/ui/color_selector.h"
#include "ui/button.h"

namespace app {

  class ColorSpectrum : public ColorSelector {
  public:
    ColorSpectrum();

    // IColorSource
    app::Color getColorByPosition(const gfx::Point& pos) override;

  protected:
    void onPaint(ui::PaintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;

  private: 
    double m_lastSaturation;

    enum CapturedBar {
      NONE = 0,
      SATURATION_BAR = 1,
      ALPHA_BAR = 2,
    };

    bool inSaturationBarArea(const gfx::Point& pos) const;
    bool inAlphaBarArea(const gfx::Point& pos) const;
    int getOneBarSize() const;
    int getBarsCount() const;
    int getBarsSize() const;

    int m_capturedBar;
  };

} // namespace app
