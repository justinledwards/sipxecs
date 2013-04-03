/*
 *
 *
 * Copyright (C) 2013 Sip2ser Srl., certain elements licensed under a Contributor Agreement.
 * Contributors retain copyright to elements licensed under a Contributor Agreement.
 * Licensed to the User under the LGPL license.
 *
 */
package org.sipfoundry.sipxconfig.phone.snom;

import org.sipfoundry.sipxconfig.upload.Upload;

public class SnomUpload extends Upload {
    private static final String SNOMLANG_DIR = "/snomlang/";
    private static final String VERSION = "localization/version";

    @Override
    public void deploy() {
        super.setDestinationDirectory(getDestinationDirectory() + SNOMLANG_DIR + getSettingValue(VERSION));
        super.deploy();
    }

    @Override
    public void undeploy() {
        super.setDestinationDirectory(getDestinationDirectory() + SNOMLANG_DIR + getSettingValue(VERSION));
        super.undeploy();
    }

}
