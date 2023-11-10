/*
 * Copyright (c) 2018, Arm Limited and affiliates.
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
#include "GEMALTO_CINTERION_CellularContext.h"
#include "GEMALTO_CINTERION_CellularStack.h"
#include "CellularLog.h"
#include "CellularDevice.h"

namespace mbed {

GEMALTO_CINTERION_CellularContext::GEMALTO_CINTERION_CellularContext(ATHandler &at, CellularDevice *device,
                                                                     const char *apn, bool cp_req, bool nonip_req) : AT_CellularContext(at, device, apn, cp_req, nonip_req)
{
}

GEMALTO_CINTERION_CellularContext::~GEMALTO_CINTERION_CellularContext()
{
}

nsapi_error_t GEMALTO_CINTERION_CellularContext::connect(const char *sim_pin, const char *apn, const char *uname,
                                          const char *pwd)
{
    set_sim_pin(sim_pin);
    set_credentials(apn, uname, pwd);

    set_device_ready();

    _at.lock();
    bool valid_context = get_context();
    _at.unlock();

    if(!valid_context) {
        set_new_context(_cid);
    }

    do_user_authentication();

    enable_access_technology();

    return AT_CellularContext::connect();
}

bool GEMALTO_CINTERION_CellularContext::get_context()
{
    _at.cmd_start_stop("+CGDCONT", "?");
    _at.resp_start("+CGDCONT:");
    set_cid(-1);
    int cid_max = 0; // needed when creating new context
    char apn[MAX_ACCESSPOINT_NAME_LENGTH];
    int apn_len = 0;

    while (_at.info_resp()) {
        int cid = _at.read_int();
        if (cid > cid_max) {
            cid_max = cid;
        }
        char pdp_type_from_context[10];
        int pdp_type_len = _at.read_string(pdp_type_from_context, sizeof(pdp_type_from_context));
        if (pdp_type_len > 0) {
            apn_len = _at.read_string(apn, sizeof(apn));
            if (apn_len > 0 && (strcmp(apn, _apn) == 0)) {
                // APN matched -> Check PDP type
                pdp_type_t pdp_type = string_to_pdp_type(pdp_type_from_context);
                tr_debug("CID %d APN \"%s\" pdp_type %u", cid, apn, pdp_type);

                // Accept exact matching PDP context type or dual PDP context for modems that support both IPv4 and IPv6 stacks
                if (get_device()->get_property(pdp_type_t_to_cellular_property(pdp_type)) ||
                        ((pdp_type == IPV4V6_PDP_TYPE && (get_device()->get_property(AT_CellularDevice::PROPERTY_IPV4_PDP_TYPE) &&
                                                          get_device()->get_property(AT_CellularDevice::PROPERTY_IPV6_PDP_TYPE))) && !_nonip_req)) {
                    _pdp_type = pdp_type;
                    set_cid(cid);
                }
            }
             else {
                cid_max = 0;
            }
        }
    }

    _at.resp_stop();
    if (_cid == -1) { // no suitable context was found so create a new one
        if (!set_new_context(cid_max + 1)) {
            return false;
        }
    }

    // save the apn
    if (apn_len > 0 && !_apn) {
        memcpy(_found_apn, apn, apn_len + 1);
    }

    tr_info("Found PDP context %d", _cid);

    return true;
}


nsapi_error_t GEMALTO_CINTERION_CellularContext::do_user_authentication()
{
    // if user has defined user name and password we need to call CGAUTH before activating or modifying context
    if ((strcmp(_pwd, "") != 0) && (strcmp(_uname, "") != 0)) {
        if (!get_device()->get_property(AT_CellularDevice::PROPERTY_AT_CGAUTH)) {
            return NSAPI_ERROR_UNSUPPORTED;
        }

        _at.at_cmd_discard("^SGAUTH", "=", "%d%d%s%s", _cid, _authentication_type, _uname, _pwd);

        if (_at.get_last_error() != NSAPI_ERROR_OK) {
            return NSAPI_ERROR_AUTH_FAILURE;
        }
    } else {
        tr_info("Empty pwd and username fields: no need for authentication\n");
    }

    return NSAPI_ERROR_OK;
}


#if !NSAPI_PPP_AVAILABLE
NetworkStack *GEMALTO_CINTERION_CellularContext::get_stack()
{
    if (_pdp_type == NON_IP_PDP_TYPE || _cp_in_use) {
        tr_error("Requesting stack for NON-IP context! Should request control plane netif: get_cp_netif()");
        return NULL;
    }

    if (!_stack) {
        _stack = new GEMALTO_CINTERION_CellularStack(_at, _apn, _uname, _pwd, _cid, (nsapi_ip_stack_t)_pdp_type, *get_device());
        if (static_cast<GEMALTO_CINTERION_CellularStack *>(_stack)->socket_stack_init() != NSAPI_ERROR_OK) {
            delete _stack;
            _stack = NULL;
        }
    }
    return _stack;
}
#endif // NSAPI_PPP_AVAILABLE

void GEMALTO_CINTERION_CellularContext::enable_access_technology()
{
    char *buffer = new char [8];
    memset(buffer, 0, 8);
    sprintf(buffer,"%08X", _band);
    switch (_rat)
    {
    case CATM1:
        _at.at_cmd_discard("^SXRAT", "=","%d", _rat);
        _at.at_cmd_discard("^SCFG", "=","%s%s", "Radio/Band/CatM",buffer);
        _at.at_cmd_discard("^SCFG", "=","%s%d%d", "Radio/Band/CatNB",0,0);
        break;

    case CATNB:
        _at.at_cmd_discard("^SXRAT", "=","%d", _rat);
        _at.at_cmd_discard("^SCFG", "=","%s%s", "Radio/Band/CatNB",buffer);
        _at.at_cmd_discard("^SCFG", "=","%s%d%d", "Radio/Band/CatM",0,0);
        break;

    default:
        break;
    }

    _at.at_cmd_discard("^SCFG", "=", "%s%s", "Tcp/withURCs", "on");
    free(buffer);

}

} /* namespace mbed */
