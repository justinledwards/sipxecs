/**
 *
 *
 * Copyright (c) 2013 Sip2ser Srl.. All rights reserved.
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

package org.sipfoundry.sipxconfig.phone.snom;

import org.sipfoundry.sipxconfig.device.DeviceVersion;
import org.sipfoundry.sipxconfig.phone.PhoneModel;


public final class SnomModel extends PhoneModel {

    public static final DeviceVersion VER_7_3_X = new DeviceVersion(SnomPhone.BEAN_ID, "7.3.X");
    public static final DeviceVersion VER_8_4_X = new DeviceVersion(SnomPhone.BEAN_ID, "8.4.X");
    public static final DeviceVersion VER_8_7_X = new DeviceVersion(SnomPhone.BEAN_ID, "8.7.X");
    public static final DeviceVersion[] SUPPORTED_VERSIONS = new DeviceVersion[]{VER_7_3_X, VER_8_4_X, VER_8_7_X};

    public SnomModel() {
        super(SnomPhone.BEAN_ID);
        setEmergencyConfigurable(true);
    }

    public static DeviceVersion getPhoneDeviceVersion(String version) {
        for (DeviceVersion  deviceVersion : SUPPORTED_VERSIONS) {
            if (deviceVersion.getName().contains(version)) {
                return deviceVersion;
            }
        }
        return VER_7_3_X;
    }
}
