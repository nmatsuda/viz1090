// viz1090, a vizualizer for dump1090 ADSB output
//
// Copyright (C) 2020, Nathan Matsuda <info@nathanmatsuda.com>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//  *  Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//
//  *  Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef MENU_PANEL_H
#define MENU_PANEL_H

#include <SDL2/SDL.h>

#include <functional>
#include <string>
#include <vector>

#include "ui/Button.h"
#include "ui/RenderContext.h"

namespace viz1090 {

/// A foreground panel containing menu buttons
/// Displayed centered on screen when open
class MenuPanel {
public:
  using ActionCallback = std::function<void()>;
  using ThemeSelectedCallback = std::function<void(const std::string&)>;
  using ThemeListProvider = std::function<std::vector<std::string>()>;
  using CurrentThemeProvider = std::function<std::string()>;

  MenuPanel();

  /// Add a button to the menu
  void addButton(const std::string& label, ActionCallback callback);

  /// Set up theme selection support
  void setThemeSupport(ThemeListProvider listProvider,
                       CurrentThemeProvider currentProvider,
                       ThemeSelectedCallback selectedCallback);

  /// Draw the menu panel (call only when open)
  void draw(const RenderContext& ctx);

  /// Handle click events - returns true if click was handled
  bool handleClick(int x, int y);

  /// Open/close the panel
  void open() { open_ = true; showingThemes_ = false; }
  void close() { open_ = false; showingThemes_ = false; }
  void toggle() { if (open_) close(); else open(); }
  [[nodiscard]] bool isOpen() const { return open_; }

private:
  void drawMainMenu(const RenderContext& ctx);
  void drawThemeList(const RenderContext& ctx);
  void rebuildThemeButtons();

  std::vector<Button> buttons_;
  Button closeButton_;
  bool open_{false};

  // Theme selection support
  bool showingThemes_{false};
  std::vector<Button> themeButtons_;
  Button backButton_;
  ThemeListProvider themeListProvider_;
  CurrentThemeProvider currentThemeProvider_;
  ThemeSelectedCallback themeSelectedCallback_;

  // Cached panel bounds
  SDL_Rect panelBounds_{0, 0, 0, 0};
};

}  // namespace viz1090

#endif  // MENU_PANEL_H
