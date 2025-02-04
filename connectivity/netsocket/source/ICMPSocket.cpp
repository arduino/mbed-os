/* Socket
 * Copyright (c) 2015 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ICMPSocket.h"
#if MBED_CONF_LWIP_RAW_SOCKET_ENABLED
#include "drivers/Timer.h"
#include "lwip/prot/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"
#endif

ICMPSocket::ICMPSocket()
{
    _socket_stats.stats_update_proto(this, NSAPI_ICMP);
}

#if MBED_CONF_LWIP_RAW_SOCKET_ENABLED
int ICMPSocket::ping(SocketAddress &socketAddress, uint32_t timeout)
{
    struct __attribute__((__packed__)) {
        struct icmp_echo_hdr header;
        uint8_t data[32];
    } request;

    ICMPH_TYPE_SET(&request.header, ICMP_ECHO);
    ICMPH_CODE_SET(&request.header, 0);
    request.header.chksum = 0;
    request.header.id = 0xAFAF;
    request.header.seqno = random();

    for (size_t i = 0; i < sizeof(request.data); i++) {
      request.data[i] = i;
    }

    request.header.chksum = inet_chksum(&request, sizeof(request));

    int res = sendto(socketAddress, &request, sizeof(request));
    if (res <= 0){
        return -1;
    }

    mbed::Timer timer;
    timer.start();
    int elapsed = -1;
    do {
        struct __attribute__((__packed__)) {
            struct ip_hdr ipHeader;
            struct icmp_echo_hdr header;
        } response;

        int rxSize = recvfrom(&socketAddress, &response, sizeof(response));
        if (rxSize < 0) {
            // time out
            break;
        }

        if (rxSize < sizeof(response)) {
            // too short
            continue;
        }

        if ((response.header.id == request.header.id) && (response.header.seqno == request.header.seqno)) {
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timer.elapsed_time()).count();
            timer.stop();
        }
  } while (elapsed == -1 && std::chrono::duration_cast<std::chrono::milliseconds>(timer.elapsed_time()).count() < timeout);

  return elapsed;
}
#endif

nsapi_protocol_t ICMPSocket::get_proto()
{
    return NSAPI_ICMP;
}
