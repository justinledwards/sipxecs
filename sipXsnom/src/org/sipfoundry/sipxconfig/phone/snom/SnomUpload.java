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
