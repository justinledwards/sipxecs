/*
 *
 *
 * Copyright (C) 2013 Sip2ser Srl., certain elements licensed under a Contributor Agreement.
 * Contributors retain copyright to elements licensed under a Contributor Agreement.
 * Licensed to the User under the LGPL license.
 *
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
