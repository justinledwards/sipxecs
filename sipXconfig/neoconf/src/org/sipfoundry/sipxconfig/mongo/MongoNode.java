/**
 * Copyright (c) 2013 eZuce, Inc. All rights reserved.
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
package org.sipfoundry.sipxconfig.mongo;

import java.util.List;

public class MongoNode {
    private String m_id;
    private List<String> m_status;

    public MongoNode(String id, List<String> status) {
        m_id = id;
        m_status = status;
    }

    public String getId() {
        return m_id;
    }

    public List<String> getStatus() {
        return m_status;
    }

    public String getLabel() {
        return label(m_id);
    }

    public static String label(String id) {
        int colon = id.indexOf(':');
        return colon > 0 ? id.substring(0, colon) : id;
    }

    public static String arbiterId(String fqdn) {
        return fqdn + ':' + MongoSettings.ARBITER_PORT;
    }

    public static String databaseId(String fqdn) {
        return fqdn + ':' + MongoSettings.SERVER_PORT;
    }
}
