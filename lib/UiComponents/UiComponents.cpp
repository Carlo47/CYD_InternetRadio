#include "UiComponents.h"


double fmap(double x, double in_min, double in_max, double out_min, double out_max)
{
  double v = (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  return fabs(v) < 1.0e-12 ? 0.0 : v;  // prevents exponential representation like -4.0957e-16
}

//                Text       Background  Border  Shadow  Font
UiTheme blueTheme(TFT_BLACK, 0x07df,     0x03df, 0x01ca, &fonts::DejaVu12);
UiTheme defaultTheme;


void UiButton::draw()
{
    _lcd.drawRoundRect(_x+2, _y+2, _w, _h, _r, _theme._shadowColor);
    _lcd.drawRoundRect(_x+1, _y+1, _w, _h, _r, _theme._shadowColor);
    _lcd.fillRoundRect(_x, _y, _w, _h, _r, _theme._borderColor);
    _lcd.fillRoundRect(_x+2, _y+2, _w-4, _h-4, _r, _theme._bodyColor);
    _lcd.setTextDatum(textdatum_t::middle_center);
    _lcd.setTextColor(_theme._textColor, _theme._bodyColor);
    _lcd.setFont(_theme._font);
    _lcd.drawString(_value, _x+_w/2, _y+2+_h/2);
    _lcd.setTextDatum(textdatum_t::middle_left);
    _lcd.setTextColor(_theme._textColor, _parent->getPanelColor());
    _lcd.drawString(_label, _x+_w+_d, _y+2+_h/2);
    
}

bool UiButton::touched(int x, int y)
{
    return (x > _x && x < _x+_w && y > _y && y < _y+_h);
}

void UiButton::clearValue()
{
    _value= "";
    draw();
}

String UiButton::getValue() 
{
    return _value;
}

void UiButton::getValue(String &value) 
{ 
    value = _value; 
}

void UiButton::getValue(int &value) 
{ 
    value = _value.toInt(); 
}

void UiButton::getValue(double &value) 
{ 
    value = _value.toDouble(); 
}

String UiButton::getLabel() 
{
    return(_label);
}

bool UiButton::rangeIsInteger() 
{ 
    return _rangeIsInteger; 
}

bool UiButton::hasSlider() 
{ 
    return _pSlider != nullptr; 
}

UiButton *UiButton::getSlider() 
{ 
    return _pSlider; 
}


void UiButton::updateValue(String value)
{
    _value = value;
    draw();
}

void UiButton::updateValue(int value)
{
    char buf[24];
    if (_minInt!=0 || _maxInt!=0) // a range is set
    {
        if (value < _minInt) value = _minInt;  // limit the value to the range
        if (value > _maxInt) value = _maxInt;
    }
    snprintf(buf, sizeof(buf), "%d", value);
    _value = buf;
    //log_i("Int value in %d, value out %s", value, _value);
    draw();
}

void UiButton::updateValue(double value)
{
    char buf[24];
    if (_minDouble!=0.0 || _maxDouble!=0.0) // a range is set
    {
        if (value < _minDouble) value = _minDouble; // limit the value to the range
        if (value > _maxDouble) value = _maxDouble;
    }
    snprintf(buf, sizeof(buf), "%.10g", value);
    _value = buf;
    //log_i("Double value in %.4g, value out %s", value, _value);
    draw();
}

void UiButton::setLabel(String label)
{
    _label = label;
    draw(); //_lcd.drawString(_label, _x+_w+_d, _y+2+_h/2);
}

void UiButton::clearLabel()
{
    _lcd.setTextColor(_parent->getPanelColor());
    _lcd.drawString(_label, _x+_w+_d, _y+2+_h/2);
    _lcd.setTextColor(_theme._textColor); 
}

void UiButton:: setRange(int min, int max)
{
    //log_i("Button setRange int %p", this);
    _minInt = min;
    _maxInt = max;
    _rangeIsInteger = true;
}

void UiButton:: setRange(double min, double max)
{
    //log_i("Button setRange double %p", this);
    _minDouble = min;
    _maxDouble = max;
    _rangeIsInteger = false;
}

void UiButton::addSlider(UiButton *btn)
{
    _pSlider = btn;
}
// --- UiButton ---


void UiLed::draw()
{
    _lcd.fillCircle(_x+2, _y+2, _radius, _theme._shadowColor);
    _lcd.fillCircle(_x, _y, _radius, _theme._borderColor);
    _isOn ? _lcd.fillCircle(_x, _y, _radius-2, _color) : _lcd.fillCircle(_x, _y, _radius-2, _theme._bodyColor);
    _lcd.setTextDatum(textdatum_t::middle_left);
    _lcd.setTextColor(_theme._textColor);
    _lcd.setFont(_theme._font);
    _lcd.drawString(_label, _x+_radius*2+_d, _y);
}

bool UiLed::touched(int x, int y)
{
    return (x > _x-_radius && x < _x+_radius && y > _y-_radius && y < _y+_radius);
}

void UiLed::setLabel(String txt)
{
    _label = txt;
    draw();     
}

bool UiLed::isOn()
{
    return _isOn;
}

void UiLed::on()
{
    if (! _isOn)
    {
        _lcd.fillCircle(_x, _y, _radius-2, _color);
        _isOn = true;
    }
}  

void UiLed::off()
{
    if (_isOn)
    {
        _lcd.fillCircle(_x, _y, _radius-2, _theme._bodyColor);
        _isOn = false;
    }
} 
    
void UiLed::toggle()
{
    if (_isOn)
    {
        _lcd.fillCircle(_x, _y, _radius-2, _theme._bodyColor);
        _isOn = false;
    }
    else
    {
        _lcd.fillCircle(_x, _y, _radius-2, _color);
        _isOn = true;
    }
}
// --- UiLed ---


void UiHslider::draw()
{
    _lcd.drawRoundRect(_x+2, _y+2, _w, _h, _r, _theme._shadowColor);
    _lcd.drawRoundRect(_x+1, _y+1, _w, _h, _r, _theme._shadowColor);
    _lcd.fillRoundRect(_x, _y, _w, _h, _r, _theme._borderColor);
    _lcd.fillRoundRect(_x+2, _y+2, _w-4, _h-4, _r, _theme._bodyColor);
    _lcd.fillCircle(_position, _y+_h/2, _rb, _color);
    _lcd.drawCircle(_position, _y+_h/2, _rb, _theme._borderColor);
    _lcd.setTextDatum(textdatum_t::middle_left);
    _lcd.setTextColor(_theme._textColor, _parent->getPanelColor());
    _lcd.setFont(_theme._font);
    _lcd.drawString(_label, _x+_w+_d, _y+2+_h/2);    
}

void UiHslider::slideToPosition(int x)
{
    _lcd.fillCircle(_position, _y+_h/2, _h, _parent->getPanelColor());
    _position = x;
    if (rangeIsInteger())
    {
        int v = map(_position-_x, 0, _w-2*_r, _minInt, _maxInt);
        if (_pValueField) getValueField()->updateValue(v);
        //log_i("Int x=%d, _w=%d, v=%d, min=%d, max=%d", x, _w, v, _minInt, _maxInt);
        _value = String(v);
    }
    else
    {
        double v = fmap(_position-_x, 0.0, _w-2*_r, _minDouble, _maxDouble);
        if (_pValueField) getValueField()->updateValue(v);
        //log_i("Double x=%d, w=%d, v=%.10g, min=%.10g, max=%.10g", x, _w, v, _minDouble, _maxDouble);
        _value = String(v);
    }
    
    draw(); 
}

void UiHslider::slideToValue(int v)
{
    _lcd.fillCircle(_position, _y+_h/2, _h, _parent->getPanelColor());
    _position = map(v, _minInt, _maxInt, 0, _w-2*_r) + _x;
    if (_pValueField) _pValueField->updateValue(v);
    _value = String(v);
    draw(); 
}

void UiHslider::slideToValue(double v)
{
    _lcd.fillCircle(_position, _y+_h/2, _h, _parent->getPanelColor());
    _position = fmap(v, _minDouble, _maxDouble, 0, _w) + _x;
    char buf[24];
    snprintf(buf,sizeof(buf), "%.4g", v);
    if (_pValueField) _pValueField->updateValue(buf);
    _value = buf;
    draw(); 
}

void UiHslider::addValueField(UiButton *btn)
{
    _pValueField = btn;
    _pValueField->addSlider(this);
    //log_i("valueField=%p, slider=%p", _pValueField, this);
}

bool UiHslider::hasValueField() 
{ 
    return _pValueField != nullptr; 
}

UiButton *UiHslider::getValueField() 
{ 
    return _pValueField ? _pValueField : nullptr; 
}

void UiHslider::setRange(int min, int max)
{
    //log_i("Slider setRange int vf %p", _pValueField);
    if (_pValueField) _pValueField->setRange(min, max);
    UiButton::setRange(min, max);
}
void UiHslider::setRange(double min, double max)
{
    //log_i("Slider setRange double vf %p", _pValueField);
    if (_pValueField) _pValueField->setRange(min, max);
    UiButton::setRange(min, max);
}
// ---UiHslider ---


void UiPanel::show()
{
    _lcd.fillRect(_x,_y,_w,_h,_bgColor);
    _hidden = false;    
}

void UiPanel::hide(UiPanel *pCaller)
{
    _hidden = true; 
    pCaller == nullptr ? _lcd.fillRect(_x,_y,_w,_h,_lcd.getBaseColor()) : pCaller->show();
}

bool UiPanel::isHidden()
{
    return _hidden;
}

void UiPanel::addKeypad(UiKeypad *pKeypad)
{
    _pKeypad = pKeypad;
}

int UiPanel::getPanelColor() 
{ 
    return _bgColor; 
}

LGFX &UiPanel::getScreen() 
{ 
    return _lcd; 
}

void UiPanel::panelText(int x, int y, String text, int textColor, GFXfont font)
{
    LGFX lcd = getScreen();
    lcd.setFont(&font);
    lcd.setTextColor(textColor);
    lcd.drawString(text, _x+x, _y+y);
}
// --- UiPanel ---


void UiKeypad::show()
{
    UiPanel::show();
    _btnEntry->clearValue();
    for (int i = 0; i < _btns.size(); i++) { _btns.at(i)->draw(); }
}

void UiKeypad::handleKeys(int x, int y)
{
    int msKeyDelay = 300;
    for (int i = 1; i < _btns.size(); i++)
    {
        if (_btns.at(i)->touched(x, y))
        {
            String keyValue = _btns.at(i)->getValue();
            Serial.printf("Key pressed: %s\n", keyValue);
            if (i > 0 && i < 12) // handle digits and decimal point
            {
                if (_btns.at(i)->getValue() == "." && _btnEntry->getValue().indexOf('.') > 0) return;
                String newValue = _btnEntry->getValue() + _btns.at(i)->getValue();
                _btnEntry->updateValue(newValue);
                delay(msKeyDelay);
                return;
            }

            if (keyValue == "Clr") 
                { 
                    _btnEntry->updateValue(""); 
                    return; 
                }
            if (keyValue == "C") 
                { 
                    _btnEntry->updateValue(_btnEntry->getValue().substring(0, _btnEntry->getValue().length()-1));
                    delay(msKeyDelay); 
                    return; 
                }
            if (keyValue =="+/-")
                { 
                    if (_btnEntry->getValue().length() > 0)
                    {
                        if (_btnEntry->getValue().indexOf('.') > 0) // it's a float
                        {
                            double v = _btnEntry->getValue().toDouble();
                            if (v != 0) v = -v;
                            _btnEntry->updateValue(v);
                        }
                        else
                        {
                            int v = _btnEntry->getValue().toInt(); // it's an integer
                            v = -v;
                            //log_i("int %d", v);
                            _btnEntry->updateValue(v);
                        }
                        delay(msKeyDelay);
                        return; 
                    }  
                }
            if (keyValue == "X")
            {
                delay(msKeyDelay);
                //log_e("==> done X");
                hide();
                UiPanel::redrawPanels(); // Called to restore underlying panels 
                return;
            }
            if (keyValue == "OK")
            {
                delay(msKeyDelay);
                String e = _btnEntry->getValue();
                if (!_targetValueField->rangeIsInteger()) // The assigned value field contains floats
                {
                    double v = e.toDouble();
                    _targetValueField->updateValue(v);
                    _targetValueField->getValue(v);
                    //log_e("==> done OK: targetValueField=%p, slider=%p", _targetValueField, this);
                    if (_targetValueField->hasSlider()) reinterpret_cast<UiHslider *>(_targetValueField->getSlider())->slideToValue(v);
                }
                else
                {
                    int v = e.toInt();                    // The assigned value field contains integers
                    _targetValueField->updateValue(v);
                    _targetValueField->getValue(v);
                    //log_e("==> done OK: targetValueField=%p, slider=%p", _targetValueField, this);
                    if (_targetValueField->hasSlider()) reinterpret_cast<UiHslider *>(_targetValueField->getSlider())->slideToValue(v);
                } 
                       
                hide();
                _okCallback(_targetValueField);
                UiPanel::redrawPanels(); // Called to restore underlying panels
                return;                
            }
        }
    }    
}

void UiKeypad::addValueField(UiButton *btn) 
{
    _targetValueField = btn;
}

void UiKeypad::addOkCallback(Callback cb)
{
    _okCallback = cb;
}
// --- UiKeypad ---