// Aseprite    - Copyright (C) 2001-2015  David Capello
// LibreSprite - Copyright (C) 2021       LibreSprite contributors
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/modules/gui.h"
#include "app/modules/i18n.h"
#include "base/bind.h"
#include "ui/ui.h"

#include <cstdio>

namespace app {

using namespace ui;

class AboutCommand : public Command {
public:
  AboutCommand();
  Command* clone() const override { return new AboutCommand(*this); }

protected:
  void onExecute(Context* context) override;

private: 
  Box* makeTextStrip(const std::string &directive, const std::vector<std::string> &links);
};

AboutCommand::AboutCommand()
  : Command("About",
            "About",
            CmdUIOnlyFlag)
{
}

void AboutCommand::onExecute(Context* context)
{
  std::unique_ptr<Window> window(new Window(Window::WithTitleBar, "About " PACKAGE));

  Box* box1 = new Box(VERTICAL);
  window->addChild(box1);

  Grid* grid = new Grid(2, false);
  box1->addChild(grid);
  
  Label* title = new Label(PACKAGE_AND_VERSION);
  title->setI18N();
  grid->addChildInCell(title, 2, 1, CENTER);

  Label* subtitle = new Label("Animated sprite editor & pixel art tool");
  subtitle->setI18N();
  grid->addChildInCell(subtitle, 2, 1, CENTER);

  grid->addChildInCell(makeTextStrip(i18n("A {0:free and open source} fork of {1:Aseprite}"), {
    "https://github.com/LibreSprite/LibreSprite/blob/master/LICENSE.txt",
    "https://www.aseprite.org/"
  }), 2, 1, CENTER);

  Separator* authors_separator1 = new Separator("Authors:", HORIZONTAL | TOP);
  authors_separator1->setI18N();
  grid->addChildInCell(authors_separator1, 2, 1, 0);

  grid->addChildInCell(makeTextStrip(i18n("*{0:Felipe Manga}, {1:Mauricio Ordoñez} -Maintainers"), {
    "https://github.com/felipemanga/",
    "https://github.com/maujin111/"
  }), 2, 1, 0);
  grid->addChildInCell(makeTextStrip(i18n("*{0:David Capello} -Original Aseprite program"), {
    "https://davidcapello.com/"
  }), 2, 1, 0);
  grid->addChildInCell(makeTextStrip(i18n("*{0:Ilija Melentijevic} -Default skin & graphics introduced in v0.8"), {
    "https://ilkke.blogspot.com/"
  }), 2, 1, 0);
  grid->addChildInCell(makeTextStrip(i18n("*{0:Contributors on GitHub}"), {
    WEBSITE_CONTRIBUTORS
  }), 2, 1, 0);

  Separator* authors_separator2 = new Separator("", HORIZONTAL);
  grid->addChildInCell(authors_separator2, 2, 1, 0);

  Label* copyright = new Label(COPYRIGHT);
  grid->addChildInCell(copyright, 2, 1, 0);

  Label* website = new LinkLabel(WEBSITE);
  grid->addChildInCell(website, 2, 1, 0);

  Box* bottom_box = new Box(HORIZONTAL);
  grid->addChildInCell(bottom_box, 2, 1, 0);
  Box* bottom_box_left = new Box(HORIZONTAL);
  bottom_box_left->setExpansive(true);
  bottom_box->addChild(bottom_box_left);
  Button* close_button = new Button("&Close");
  close_button->setI18N();
  close_button->setFocusMagnet(true);
  bottom_box->addChild(close_button);
  Box* bottom_box_right = new Box(HORIZONTAL);
  bottom_box_right->setExpansive(true);
  bottom_box->addChild(bottom_box_right);

  close_button->setBorder(
    gfx::Border(
      close_button->border().left() + 16*guiscale(),
      close_button->border().top(),
      close_button->border().right() + 16*guiscale(),
      close_button->border().bottom()));

  close_button->Click.connect(base::Bind<void>(&Window::closeWindow, window.get(), close_button));

  window->openWindowInForeground();
}

/**
  Create inline text strip with clickable links.
  Format is `{Index of link:Text in link} Non-interactive text outside braces`
  We do this because the order of text in a sentence is not guaranteed to be consistent between languages
  (e.g. `A {0:free and open source} fork of {1:Aseprite}` -> `{1:Aseprite}の{0:無料かつオープンソース}フォーク`)
*/
Box* AboutCommand::makeTextStrip(const std::string &directive, const std::vector<std::string> &links) {
  Box* box = new Box(HORIZONTAL);
  box->setChildSpacing(-1);
  std::string currentText = "";
  int linkIndex = 0;
  int dirLength = directive.length();
  bool isParsingIndex = false;

  for (int i = 0; i < dirLength; i++) {
    char currentChar = directive[i];
    if (isParsingIndex) {
      // ':' = End of index number
      if (currentChar == ':') {
        isParsingIndex = false;
      } else {
        // Append digit to index
        ASSERT(currentChar >= '0' && currentChar <= '9');
        linkIndex = linkIndex * 10 + (currentChar - '0');
      }
    } else switch (currentChar) {

      // '\' = Escape character
      case '\\':
        i++;
        currentText.push_back(directive[i]);
        break;

      // '{' = Beginning of link directive
      case '{':
        if (!currentText.empty()) {
          Label* label = new Label(currentText);
          box->addChild(label);
          currentText.clear();
        }
        linkIndex = 0;
        isParsingIndex = true;
        break;

      // '}' = End of link directive
      case '}':
        if (!currentText.empty()) {
          LinkLabel* label = new LinkLabel(links[linkIndex], currentText);
          box->addChild(label);
          currentText.clear();
        }
        break;
      
      // Any other character = Append character to string
      default:
        currentText.push_back(currentChar);
    }
  }
  
  // Add leftover text
  if (!currentText.empty()) {
    Label* label = new Label(currentText);
    box->addChild(label);
    currentText.clear();
  }

  return box;
}

Command* CommandFactory::createAboutCommand()
{
  return new AboutCommand;
}

} // namespace app
