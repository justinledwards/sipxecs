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
    public static enum State {
        // states defined in http://docs.mongodb.org/manual/reference/replica-status/
        STARTUP1, PRIMARY, SECONDARY, RECOVERING, FATAL_ERROR,
        STARTUP2, UNKNOWN, ARBITER, DOWN, ROLLBACK, REMOVED,

        // states added my mongo-replication-admin
        UNAVAILABLE, NAME_MISMATCH, MISCONFIGURED, UNINITIALIZED
    }

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

    public State getState() {
        return State.valueOf((String) m_status.get("state"));
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
        Map<String, ? > status = getStatusMember();
        return status == null ? null : (String) status.get(key);
    }

    Map<String, ? > getStatusMember() {
        List<Map<String, ? >> l = getStatusMembers();
        if (l != null) {
            for (Map<String, ? > m : l) {
                if (m.get("self") != null) {
                    return m;
                }
            }
        }
        return null;
    }

    List<Map<String, ? >> getStatusMembers() {
        String s = getServerId();
        Map<String, ? > o = (Map<String, ? >) m_status.get("status");
        if (o != null) {
            List<Map<String, ? >> l = (List<Map<String, ? >>) o.get("members");
            return l;
        }
        return null;
    }
}
