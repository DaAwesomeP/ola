/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ArduinoDMXUSBWidget.cpp
 * The ArduinoDMXUSB Widget(based on DMX King Ultra DMX Pro code by Simon Newton).
 * Copyright (C) 2019 Perry Naseck (DaAwesomeP)
 */

#include <cstring>
#include "ola/Constants.h"
#include "ola/strings/Format.h"
#include "plugins/usbpro/ArduinoDMXUSBWidget.h"

namespace ola {
namespace plugin {
namespace usbpro {

/**
 * ArduinoDMXUSBWidget Constructor
 */
ArduinoDMXUSBWidget::ArduinoDMXUSBWidget(
  ola::io::ConnectedDescriptor *descriptor)
    : GenericUsbProWidget(descriptor) {
}


bool ArduinoDMXUSBWidget::SendDMX(const DmxBuffer &buffer) {
  return SendDMXWithLabel(DMX_START_PORT, buffer);
}


bool ArduinoDMXUSBWidget::SendDMXPort(unsigned int port, const DmxBuffer &buffer) {
  return SendDMXWithLabel(DMX_START_PORT + port, buffer);
}


bool ArduinoDMXUSBWidget::SendDMXWithLabel(uint8_t label,
                                         const DmxBuffer &data) {
  struct {
    uint8_t start_code;
    uint8_t dmx[DMX_UNIVERSE_SIZE];
  } widget_dmx;

  widget_dmx.start_code = DMX512_START_CODE;
  unsigned int length = DMX_UNIVERSE_SIZE;
  data.Get(widget_dmx.dmx, &length);
  return SendMessage(label,
                     reinterpret_cast<uint8_t*>(&widget_dmx),
                     length + 1);
}


void ArduinoDMXUSBWidget::GetExtendedParameters(
    arduinodmxusb_extended_params_callback *callback) {
  m_outstanding_extended_param_callbacks.push_back(callback);

  uint16_t user_size = 0;
  bool r = SendMessage(EXTENDED_PARAMETERS_LABEL,
                       reinterpret_cast<uint8_t*>(&user_size),
                       sizeof(user_size));

  if (!r) {
    // failed
    m_outstanding_extended_param_callbacks.pop_back();
    arduinodmxusb_extended_parameters params = {0, 0};
    callback->Run(false, params);
  }
}


void ArduinoDMXUSBWidget::SpecificStop() {
  // empty extedned_params struct
  arduinodmxusb_extended_parameters extedned_params;
  while (!m_outstanding_extended_param_callbacks.empty()) {
    arduinodmxusb_extended_params_callback *callback =
        m_outstanding_extended_param_callbacks.front();
    m_outstanding_extended_param_callbacks.pop_front();
    callback->Run(false, extedned_params);
  }
}


void ArduinoDMXUSBWidget::HandleMessage(uint8_t label,
                                 const uint8_t *data,
                                 unsigned int length) {
  if (label == EXTENDED_PARAMETERS_LABEL) {
    HandleExtendedParameters(data, length);
  } else {
    GenericUsbProWidget::HandleMessage(label, data, length);
  }
}


void ArduinoDMXUSBWidget::HandleExtendedParameters(const uint8_t *data,
                                           unsigned int length) {
  if (m_outstanding_extended_param_callbacks.empty()) {
    return;
  }

  if (length < sizeof(arduinodmxusb_extended_parameters)) {
    return;
  }

  arduinodmxusb_extended_parameters params;
  memcpy(&params, data, sizeof(arduinodmxusb_extended_parameters));

  arduinodmxusb_extended_params_callback *callback =
      m_outstanding_extended_param_callbacks.front();
  m_outstanding_extended_param_callbacks.pop_front();

  callback->Run(true, params);
}
}  // namespace usbpro
}  // namespace plugin
}  // namespace ola