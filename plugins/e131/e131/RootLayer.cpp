/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * RootLayer.cpp
 * The RootLayer
 * Copyright (C) 2007 Simon Newton
 */

#include <ola/Logging.h>
#include "RootLayer.h"

namespace ola {
namespace e131 {

/*
 * Create a new RootLayer
 * @param parser the parser to use
 * @param ip_address the desired IP address to use
 * @param loop set to true to loop multicast packets
 */
RootLayer::RootLayer(UDPTransport *transport, const CID &cid):
  m_transport(transport),
  m_cid(cid),
  m_root_pdu(0) {
    m_transport->SetInflator(&m_root_inflator);
    m_root_pdu.Cid(cid);
}


/*
 * Add an inflator to the root level
 */
bool RootLayer::AddInflator(BaseInflator *inflator) {
  return m_root_inflator.AddInflator(inflator);
}


/*
 * Encapsulate this PDU in a RootPDU and send it to the destination.
 * @param addr where to send the PDU
 * @param vector the vector to use at the root level
 * @param pdu the pdu to send.
 */
bool RootLayer::SendPDU(struct in_addr &addr,
                       unsigned int vector,
                       const PDU &pdu) {
  m_working_block.Clear();
  m_working_block.AddPDU(&pdu);
  return SendPDUBlock(addr, vector, m_working_block);
}


/*
 * Encapsulate this PDUBlock in a RootPDU and send it to the destination.
 * @param addr where to send the PDU
 * @param vector the vector to use at the root level
 * @param block the PDUBlock to send.
 */
bool RootLayer::SendPDUBlock(struct in_addr &addr,
                            unsigned int vector,
                            const PDUBlock<PDU> &block) {

  if (!m_transport) {
    OLA_WARN << "transport is null";
    return false;
  }

  m_root_pdu.SetVector(vector);
  m_root_pdu.SetBlock(&block);
  m_root_block.Clear();
  m_root_block.AddPDU(&m_root_pdu);
  OLA_WARN << "size: " << block.Size();
  OLA_WARN << "size: " << m_root_pdu.Size();
  OLA_WARN << "size: " << m_root_block.Size();

  struct sockaddr_in destination;
  destination.sin_family = AF_INET;
  destination.sin_port = htons(UDPTransport::ACN_PORT);
  destination.sin_addr = addr;
  return m_transport->Send(m_root_block, destination);
}

} //e131
} //ola
