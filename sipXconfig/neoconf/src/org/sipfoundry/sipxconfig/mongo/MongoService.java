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
import java.util.Map;

public class MongoService {
    public static final String PRIMARY = "PRIMARY";
    public static final String SECONDARY = "SECONDARY";

    private Map<String, ? > m_status;

    public MongoService(Map<String, ? > status) {
        m_status = status;
    }

    public String getServerId() {
        return (String) m_status.get("server");
    }

    public String getServerLabel() {
        return label(getServerId());
    }

    public static String label(String id) {
        int colon = id.indexOf(':');
        return colon > 0 ? id.substring(0, colon) : id;
    }

    public Map<String, ? > entries() {
        return m_status;
    }

    public String getState() {
        return getStatusValue("stateStr");
    }

    public String getErrMsg() {
        return null;
    }

    public String getHealth() {
        return "UP";
    }

    @Override
    public String toString() {
        return m_status == null ? "" : m_status.toString();
    }

    public String getStatusValue(String key) {
        Map<String, ?> status = getStatus();
        return status == null ? null : (String) status.get(key);
    }

    Map<String, ?> getStatus() {
        String s = getServerId();
        Map<String, ?> o = (Map<String, ?>) m_status.get("status");
        if (o != null) {
            List<Map<String, ? >> l = (List<Map<String, ?>>) o.get("members");
            if (l != null) {
                for (Map<String, ? > m : l) {
                    if (m.get("self") != null) {
                        return m;
                    }

                }
            }

        }
        return null;
    }
}
