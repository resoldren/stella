//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef KEYBOARD_WIDGET_HXX
#define KEYBOARD_WIDGET_HXX

#include "Control.hxx"
#include "Event.hxx"
#include "ControllerWidget.hxx"

class KeyboardWidget : public ControllerWidget
{
  public:
    KeyboardWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                   Controller& controller);
    virtual ~KeyboardWidget() = default;

  private:
    CheckboxWidget* myBox[12];
    Event::Type* myEvent;

    static Event::Type ourLeftEvents[12], ourRightEvents[12];

  private:
    void loadConfig() override;
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;

    // Following constructors and assignment operators not supported
    KeyboardWidget() = delete;
    KeyboardWidget(const KeyboardWidget&) = delete;
    KeyboardWidget(KeyboardWidget&&) = delete;
    KeyboardWidget& operator=(const KeyboardWidget&) = delete;
    KeyboardWidget& operator=(KeyboardWidget&&) = delete;
};

#endif
