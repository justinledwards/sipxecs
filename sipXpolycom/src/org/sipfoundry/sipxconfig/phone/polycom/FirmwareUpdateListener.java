/**
 *
 *
 * Copyright (c) 2012 eZuce, Inc. All rights reserved.
 * Contributed to SIPfoundry under a Contributor Agreement
 *
 * This software is free software; you can redistribute it and/or modify it under
 * the terms of the Affero General Public License (AGPL) as published by the
 * Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for more
 * details.
 */
package org.sipfoundry.sipxconfig.phone.polycom;

import java.util.ArrayList;
import java.util.List;

import org.apache.commons.lang.ArrayUtils;
import org.apache.commons.lang.StringUtils;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.sipfoundry.sipxconfig.common.event.DaoEventListener;
import org.sipfoundry.sipxconfig.device.DeviceVersion;
import org.sipfoundry.sipxconfig.phone.Line;
import org.sipfoundry.sipxconfig.phone.Phone;
import org.sipfoundry.sipxconfig.phone.PhoneContext;
import org.sipfoundry.sipxconfig.setting.Group;

public class FirmwareUpdateListener implements DaoEventListener {
    public static final String SETTING_TRANSPORT_VOIPPROT = "voIpProt/SIP.outboundProxy/transport";
    public static final String SETTING_TRANSPORT_REG_PROXY = "reg/outboundProxy.transport";
    public static final String SETTING_TRANSPORT_REG_SERV1 = "reg/server/1/transport";
    public static final String SETTING_TRANSPORT_REG_SERV2 = "reg/server/2/transport";
    public static final String TCP_ONLY_40 = "TCPOnly";
    public static final String TCP_ONLY_32 = "TCPonly";
    private static final Log LOG = LogFactory.getLog(FirmwareUpdateListener.class);
    private static final String GROUP_FW_VERSION = "group.version/firmware.version";

    private PhoneContext m_phoneContext;

    public void setPhoneContext(PhoneContext phoneContext) {
        m_phoneContext = phoneContext;
    }

    @Override
    public void onDelete(Object entity) {
    }

    @Override
    public void onSave(Object entity) {
        //when we change group order (weight) we also need to change version
        if (entity instanceof ArrayList<?>) {
            List<?> list = (ArrayList<?>) entity;
            if (list.isEmpty()) {
                return;
            }
            Object o = list.iterator().next();
            if (o instanceof Group) {
               setPhoneDeviceVersion((Group) o); 
            }
        } else if (entity instanceof Group) {
            Group g = (Group) entity;
            setPhoneDeviceVersion(g);
        } else if (entity instanceof PolycomPhone) {
            PolycomPhone phone = (PolycomPhone) entity;
            if (phone.getGroups().isEmpty()) {
                return;
            }
            Group groupWithHighestWeight = phone.getGroups().iterator().next(); 
            if (groupWithHighestWeight.getSettingValue(GROUP_FW_VERSION) == null) {
                return;
            }
            setPhoneDeviceVersion(phone, groupWithHighestWeight);
        }

    }
    
    private void setPhoneDeviceVersion(Group g) {
        if (Phone.GROUP_RESOURCE_ID.equals(g.getResource())) {
            if (g.getSettingValue(GROUP_FW_VERSION) != null
                    && StringUtils.isNotEmpty(g.getSettingValue(GROUP_FW_VERSION))) {
                for (Phone phone : m_phoneContext.getPhonesByGroupId(g.getId())) {
                    if (phone instanceof PolycomPhone) {
                        setPhoneDeviceVersion(phone, g);
                        m_phoneContext.storePhone(phone);
                    }
                }
            }
        } else if (entity instanceof PolycomPhone) {
            PolycomPhone phone = (PolycomPhone) entity;
            String transport = phone.getSettingValue(SETTING_TRANSPORT_VOIPPROT);
            DeviceVersion ver = phone.getDeviceVersion();
            if ((ver.equals(PolycomModel.VER_3_2_X) || ver.equals(PolycomModel.VER_3_1_X))
                    && transport.equals(TCP_ONLY_40)) {
                phone.setSettingValue(SETTING_TRANSPORT_VOIPPROT, TCP_ONLY_32);
            } else if ((ver.equals(PolycomModel.VER_4_0_X) || ver.equals(PolycomModel.VER_4_1_X))
                    && transport.equals(TCP_ONLY_32)) {
                phone.setSettingValue(SETTING_TRANSPORT_VOIPPROT, TCP_ONLY_40);
            }
            for (Line line : phone.getLines()) {
                String transportReg = line.getSettingValue(SETTING_TRANSPORT_REG_PROXY);
                if ((ver.equals(PolycomModel.VER_3_2_X) || ver.equals(PolycomModel.VER_3_1_X))
                        && transportReg.equals(TCP_ONLY_40)) {
                    line.setSettingValue(SETTING_TRANSPORT_REG_PROXY, TCP_ONLY_32);
                } else if ((ver.equals(PolycomModel.VER_4_0_X) || ver.equals(PolycomModel.VER_4_1_X))
                        && transportReg.equals(TCP_ONLY_32)) {
                    line.setSettingValue(SETTING_TRANSPORT_REG_PROXY, TCP_ONLY_40);
                }
                String transportRegServ1 = line.getSettingValue(SETTING_TRANSPORT_REG_SERV1);
                if ((ver.equals(PolycomModel.VER_3_2_X) || ver.equals(PolycomModel.VER_3_1_X))
                        && transportRegServ1.equals(TCP_ONLY_40)) {
                    line.setSettingValue(SETTING_TRANSPORT_REG_SERV1, TCP_ONLY_32);
                } else if ((ver.equals(PolycomModel.VER_4_0_X) || ver.equals(PolycomModel.VER_4_1_X))
                        && transportRegServ1.equals(TCP_ONLY_32)) {
                    line.setSettingValue(SETTING_TRANSPORT_REG_SERV1, TCP_ONLY_40);
                }
                String transportRegServ2 = line.getSettingValue(SETTING_TRANSPORT_REG_SERV2);
                if ((ver.equals(PolycomModel.VER_3_2_X) || ver.equals(PolycomModel.VER_3_1_X))
                        && transportRegServ2.equals(TCP_ONLY_40)) {
                    line.setSettingValue(SETTING_TRANSPORT_REG_SERV2, TCP_ONLY_32);
                } else if ((ver.equals(PolycomModel.VER_4_0_X) || ver.equals(PolycomModel.VER_4_1_X))
                        && transportRegServ2.equals(TCP_ONLY_32)) {
                    line.setSettingValue(SETTING_TRANSPORT_REG_SERV2, TCP_ONLY_40);
                }
            }
        }
    }
    
    private void setPhoneDeviceVersion(Phone phone, Group g) {
        DeviceVersion version = DeviceVersion.getDeviceVersion(PolycomPhone.BEAN_ID
                + g.getSettingValue(GROUP_FW_VERSION));
        if (ArrayUtils.contains(phone.getModel().getVersions(), version)) {
            LOG.info("Updating " + phone.getSerialNumber() + " to " + version.getVersionId());
            phone.setDeviceVersion(version);
            
        } else {
            LOG.info("Skipping " + phone.getSerialNumber() + " as it doesn't support "
                    + version.getVersionId() + "; model: " + phone.getModelId());
        }
    }
}
