#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "lgfx_ESP32_2432S028.h"
#include <vector>

#pragma once

#define DARKERGREY 0x3166


//Forward declaration
class UiKeypad;
class UiButton;

using Callback = void(*)(UiButton *);

class UiTheme
{
    public:
        UiTheme(int textColor=TFT_WHITE, 
                int bgColor=DARKERGREY, 
                int borderColor=TFT_SKYBLUE, 
                int shadowColor=TFT_BLACK, 
                const GFXfont *font = &fonts::DejaVu18) : _textColor(textColor), _bodyColor(bgColor), 
                    _borderColor(borderColor), _shadowColor(shadowColor), _font(font)
        {}

        int _textColor;
        int _bodyColor;
        int _borderColor;
        int _shadowColor;
        const GFXfont *_font;       
};
extern UiTheme defaultTheme;
extern UiTheme blueTheme;

// A panel is the rectangular container of other GUI components.
// It can freely be placed on the lcd screen. The components are placed 
// relative to the panels origin (left upper corner).
// An optional keypad for entering numbers can be associated with the panel.
// The user has to derive his custom panels from this class. For each
// custom panel he must implement a keyhandler function, that processes 
// the inputs on the touch screen. 
class UiPanel
{   
    public:
        static std::vector<UiPanel *> panels; // Holds all panels defined in main
        static void redrawPanels() // Redraw all panels. Called when Keypad is closed
        { 
            for (int i = 0; i < panels.size(); i++) panels.at(i)->show(); 
        }

        UiPanel(LGFX &lcd, bool hidden) : 
            _lcd(lcd), _hidden(hidden)
        {}

        UiPanel(LGFX &lcd, int bgColor, bool hidden) : 
            _lcd(lcd), _bgColor(bgColor), _hidden(hidden)
        {}

        UiPanel(LGFX &lcd, int x, int y, int w, int h, int bgColor, bool hidden) : 
            _lcd(lcd), _x(x), _y(y), _w(w), _h(h), _bgColor(bgColor), _hidden(hidden)
        {}

        UiPanel(LGFX &lcd, int x, int y, int w, int h,  bool hidden) : 
            _lcd(lcd), _x(x), _y(y),  _w(w), _h(h), _hidden(hidden)
        {}

        virtual void show(); 
        void hide(UiPanel *pCaller=nullptr);
        bool isHidden();
        void addKeypad(UiKeypad *pKeypad);
        int getPanelColor();
        void panelText(int x, int y, String text, int textColor=TFT_BLACK,  GFXfont=fonts::DejaVu18);
        LGFX &getScreen();
        
    protected:
        LGFX &_lcd;
        int _x = 0;
        int _y = 0;
        int _w = _lcd.width(); 
        int _h = _lcd.height();
        int _bgColor = TFT_BLACK;    
        bool _hidden = true;
        UiKeypad *_pKeypad = nullptr;
};


// Button acts as pushbutton or input/output value field.
// The components UiLed and UiSlider are derived classes from UiButton
class UiButton
{
    public:
        UiButton(UiPanel *parent, int x, int y, int w, int h, UiTheme &theme, String value="", String label="") : 
            _parent(parent), _x(x), _y(y), _w(w), _h(h), _theme(theme), _value(value), _label(label)
        {}    

        UiButton(UiPanel *parent, int x, int y, int w, int h, String value="", String label="") : 
            _parent(parent), _x(x), _y(y), _w(w), _h(h), _value(value), _label(label)
        {}

        UiButton(UiPanel *parent, int x, int y, int w, int h) : 
            _parent(parent), _x(x), _y(y), _w(w), _h(h)
        {}

        virtual void draw();
        virtual bool touched(int x, int y);
        void clearValue();
        String getValue();
        void getValue(String &value);
        void getValue(int &value);
        void getValue(double &value);
        void updateValue(String value);
        void updateValue(int value);
        void updateValue(double value);
        void clearLabel();
        void setLabel(String label);
        String getLabel();
        void setRange(int min, int max);
        void setRange(double min, double max);
        bool rangeIsInteger();
        void addSlider(UiButton* pSlider);
        bool hasSlider();
        UiButton *getSlider();

    protected:  
        int _x = 0;
        int _y = 0;
        int _w; 
        int _h;
        int _d = 8;
        int _r = 4;
        int    _minInt = 0;
        int    _maxInt = 0;
        int    _valInt = 0;
        double _minDouble = 0.0;
        double _maxDouble = 0.0;
        double _valDouble = 0.0;
        bool _rangeIsInteger = true; 
        UiPanel *_parent;
        UiButton *_pSlider = nullptr;
        LGFX &_lcd = _parent->getScreen();
        UiTheme &_theme=defaultTheme;
        String _value="";
        String _label="";
};  //--- UiButton ---


// LED button on/off
class UiLed : public UiButton
{
    public:
        UiLed(UiPanel *parent, int x, int y, int radius, int color, UiTheme &theme, String label="", bool isOn=false) : 
            UiButton(parent, x, y, 2*radius, 2*radius, theme, "", label), _radius(radius), _color(color), _isOn(isOn)
        {}

        UiLed(UiPanel *parent, int x, int y, int radius, int color, String label="", bool isOn=false) : 
            UiButton(parent, x, y, 2*radius, 2*radius, "", label), _radius(radius), _color(color), _isOn(isOn)
        {}

        void draw();
        bool touched(int x, int y);
        void setLabel(String txt);
        bool isOn();
        void on();
        void off();
        void toggle();

        private:
        int  _radius; // radius of slider button
        bool _isOn = false;
        int  _color; // color of the slider button        
};


// A horizontal slider with optional linked value field
class UiHslider : public UiButton
{
    public:
        UiHslider(UiPanel *parent, int x, int y, int w, int h, int color, UiTheme &theme, String label="") : 
            UiButton(parent, x, y, w, h, theme, "", label), _color(color)
            {_value = (_position-_x) * 100 / _w; }

        UiHslider(UiPanel *parent, int x, int y, int w, int h, int color, String label="") : 
            UiButton(parent, x, y, w, h, "", label), _color(color)
            {_value = (_position-_x) * 100 / _w; }

        UiHslider(UiPanel *parent, int x, int y, int w, int h, String label="") : 
            UiButton(parent, x, y, w, h, "", label)
            {_value = (_position-_x) * 100 / _w; }

        void draw();
        void slideToPosition(int x);
        void slideToValue(int v);
        void slideToValue(double v);
        void addValueField(UiButton *btn);
        bool hasValueField();
        UiButton *getValueField();
        void setRange(int min, int max);
        void setRange(double min, double max);
        
    private:
        int _color=TFT_LIGHTGREY;
        int _d = 10; // distance to label
        int _r = 4;  // radius of rounded rectangle
        int _rb= 3*_h/4;  // radius of slider knob
        int _position = _x+_w/2;
        UiButton *_pValueField = nullptr; // ponter to linked value field
};

// Numeric keypad for entering numbers
class UiKeypad : public UiPanel
{
    public:
        UiKeypad(LGFX &lcd, int x, int y, int bgColor, bool hidden) : 
            UiPanel(lcd, x, y, _wp, _hp, bgColor, hidden)
        { if (! hidden) show(); }

        void show();
        void handleKeys(int x, int y);
        void addValueField(UiButton *btn);
        void addOkCallback(Callback cb);

    private:
        int __x = _x + _gap; // origin x of the top left button (screen coords)
        int __y = _y + _gap; // origin y of the top left button (screen coords)

        static const int _rows = 5;  // number of key rows
        static const int _cols = 4;  // number of key columns
        static const int _wb   = 40; // width of the keys
        static const int _hb   = 25; // height of the keys
        static const int _gap  = 4;  // gap between keys 
        static const int _wp   = _cols*(_wb+_gap) + _gap; // width of the underlying panel
        static const int _hp   = _rows*(_hb+_gap) + _gap; // height of the underlying panel

        UiButton *_targetValueField;
        Callback _okCallback = nullptr;

        UiButton *_btnEntry = new UiButton(this, __x, __y, (_wb + _gap)*_cols - _gap, _hb, "");
        UiButton *_btn1     = new UiButton(this, __x,               __y+(_hb+_gap), _wb, _hb, "1");
        UiButton *_btn2     = new UiButton(this, __x+1*(_wb + _gap), __y+(_hb+_gap), _wb, _hb, "2");
        UiButton *_btn3     = new UiButton(this, __x+2*(_wb + _gap), __y+(_hb+_gap), _wb, _hb, "3");
        UiButton *_btnC     = new UiButton(this, __x+3*(_wb + _gap), __y+(_hb+_gap), _wb, _hb, "C");

        UiButton *_btn4   = new UiButton(this, __x,               __y+2*(_hb+_gap), _wb, _hb, "4");
        UiButton *_btn5   = new UiButton(this, __x+1*(_wb + _gap), __y+2*(_hb+_gap), _wb, _hb, "5");
        UiButton *_btn6   = new UiButton(this, __x+2*(_wb + _gap), __y+2*(_hb+_gap), _wb, _hb, "6"); 
        UiButton *_btnClr = new UiButton(this, __x+3*(_wb + _gap), __y+2*(_hb+_gap), _wb, _hb, "Clr");

        UiButton *_btn7      = new UiButton(this, __x,               __y+3*(_hb+_gap), _wb, _hb, "7");
        UiButton *_btn8      = new UiButton(this, __x+1*(_wb + _gap), __y+3*(_hb+_gap), _wb, _hb, "8");
        UiButton *_btn9      = new UiButton(this, __x+2*(_wb + _gap), __y+3*(_hb+_gap), _wb, _hb, "9");
        UiButton *_btnCancel = new UiButton(this, __x+3*(_wb + _gap), __y+3*(_hb+_gap), _wb, _hb, "X");

        UiButton *_btnSign = new UiButton(this, __x,               __y+4*(_hb+_gap), _wb, _hb, "+/-");
        UiButton *_btn0    = new UiButton(this, __x+1*(_wb + _gap), __y+4*(_hb+_gap), _wb, _hb, "0");
        UiButton *_btnDot  = new UiButton(this, __x+2*(_wb + _gap), __y+4*(_hb+_gap), _wb, _hb, ".");
        UiButton *_btnOk   = new UiButton(this, __x+3*(_wb + _gap), __y+4*(_hb+_gap), _wb, _hb, "OK");

        std::vector<UiButton *> _btns = {_btnEntry, _btn1, _btn2, _btn3, _btn4, _btn5, _btn6, 
                                         _btn7, _btn8, _btn9, _btn0, _btnDot, _btnC, _btnClr, 
                                         _btnCancel, _btnSign, _btnOk};
};
