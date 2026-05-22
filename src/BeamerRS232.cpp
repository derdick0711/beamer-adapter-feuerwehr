#include "BeamerRS232.h"

BeamerRS232 gRS232;

void BeamerRS232::begin(const Rs232Config& cfg) {
    // UART1 on configurable pins
    _serial = &Serial1;
    _serial->begin(cfg.baud, SERIAL_8N1, cfg.rxPin, cfg.txPin);
    delay(100);
}

const char* BeamerRS232::_cmdStr(BeamerCmd cmd) {
    switch (cmd) {
        case BeamerCmd::POWER_ON:         return "*0 IR 001\r";
        case BeamerCmd::POWER_OFF:        return "*0 IR 002\r";
        case BeamerCmd::BLANK_ON:         return "*0 IR 055\r";
        case BeamerCmd::BLANK_OFF:        return "*0 IR 055\r";
        case BeamerCmd::INPUT_HDMI:       return "C36\r";
        case BeamerCmd::INPUT_VGA:        return "C05\r";
        case BeamerCmd::INPUT_COMPONENT:  return "C33\r";
        case BeamerCmd::INPUT_SVIDEO:     return "C34\r";
        case BeamerCmd::INPUT_COMPOSITE:  return "C35\r";
        default:                          return nullptr;
    }
}

bool BeamerRS232::sendCommand(BeamerCmd cmd) {
    if (!_serial) return false;
    _flushRx();

    const char* str = _cmdStr(cmd);
    if (!str) return false;

    _serial->print(str);

    bool ack = _waitAck(500);

    // Update global status
    if (ack) {
        gBeamerStatus.markReachable();
        switch (cmd) {
            case BeamerCmd::POWER_ON:
                gBeamerStatus.power = PowerState::ON;
                break;
            case BeamerCmd::POWER_OFF:
                gBeamerStatus.power = PowerState::OFF;
                break;
            case BeamerCmd::BLANK_ON:
                gBeamerStatus.blank = true;
                break;
            case BeamerCmd::BLANK_OFF:
                gBeamerStatus.blank = false;
                break;
            case BeamerCmd::INPUT_HDMI:
                gBeamerStatus.input = InputSource::HDMI;
                break;
            case BeamerCmd::INPUT_VGA:
                gBeamerStatus.input = InputSource::VGA;
                break;
            case BeamerCmd::INPUT_COMPONENT:
                gBeamerStatus.input = InputSource::COMPONENT;
                break;
            case BeamerCmd::INPUT_SVIDEO:
                gBeamerStatus.input = InputSource::SVIDEO;
                break;
            case BeamerCmd::INPUT_COMPOSITE:
                gBeamerStatus.input = InputSource::COMPOSITE;
                break;
            default: break;
        }
    } else {
        gBeamerStatus.markUnreachable();
    }

    return ack;
}

bool BeamerRS232::_waitAck(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (millis() - start < timeoutMs) {
        if (_serial->available() >= 1) {
            uint8_t b = _serial->read();
            if (b == 0x06) { // ACK
                // Consume trailing CR if present
                delay(5);
                while (_serial->available()) _serial->read();
                return true;
            }
            if (b == 0x15) { // NAK
                return false;
            }
        }
        delay(5);
    }
    return false; // timeout
}

void BeamerRS232::_flushRx() {
    while (_serial && _serial->available()) _serial->read();
}

const char* BeamerRS232::inputName(InputSource src) {
    switch (src) {
        case InputSource::HDMI:      return "hdmi";
        case InputSource::VGA:       return "vga";
        case InputSource::COMPONENT: return "component";
        case InputSource::SVIDEO:    return "svideo";
        case InputSource::COMPOSITE: return "composite";
        default:                     return "unknown";
    }
}

InputSource BeamerRS232::inputFromString(const String& s) {
    if (s == "hdmi")      return InputSource::HDMI;
    if (s == "vga")       return InputSource::VGA;
    if (s == "component") return InputSource::COMPONENT;
    if (s == "svideo")    return InputSource::SVIDEO;
    if (s == "composite") return InputSource::COMPOSITE;
    return InputSource::UNKNOWN;
}
